#include "KinectInput.h"
#include "CinderLogger.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::kinect2;

KinectInput::KinectInput() {

	libfreenect2::setGlobalLogger(new CinderLogger());
	
	if(mFreenect2.enumerateDevices() == 0) {
		CI_LOG_E("No Kinect sensor connected!");
	}
	
}

KinectInput::~KinectInput() {
	
}

string KinectInput::getDeviceSerialNumber(int index) {	
	
	return mFreenect2.getDeviceSerialNumber(index);
	
}

bool KinectInput::setup(const std::string serial, 
						const PacketPipelineType packetPipelineType, 
						const bool requestColorFrame, 
						const bool requestDepthFrame, 
						const bool requestPointCloud, 
						const float minDepth, 
						const float maxDepth,
						const bool autoReconnection,
						const float reconnctionInterval) {

	mSerial = serial;
	mPacketPipelineType = packetPipelineType;
	mRequestColorFrame = requestColorFrame;
	mRequestDepthFrame = requestDepthFrame;
	mRequestPointCloud = requestPointCloud;
	mMinDepth = minDepth;
	mMaxDepth = maxDepth;
	mAutoReconnection = autoReconnection;
	mReconnectionInterval = reconnctionInterval;

	return setup();
	
}

bool KinectInput::setup() {

	// Setup packet pipeline:
	libfreenect2::PacketPipeline *packetPipeline;

	switch (mPacketPipelineType) {
	case PacketPipelineType::CPU:
		packetPipeline = new libfreenect2::CpuPacketPipeline();
		break;
	case PacketPipelineType::OPEN_GL:
		packetPipeline = new libfreenect2::OpenGLPacketPipeline();
		break;
	case PacketPipelineType::OPEN_CL:
		packetPipeline = new libfreenect2::OpenCLPacketPipeline();
		break;
	}

	// Setup device control:
	mDevice = shared_ptr<libfreenect2::Freenect2Device>(mFreenect2.openDevice(mSerial, packetPipeline));

	if (!mDevice) {
		CI_LOG_E("Failure opening Kinect: " << mSerial);
		return false;
	}

	// Setup frame listner:
	int frameListenerType = 0;

	if (mRequestColorFrame | mRequestPointCloud) frameListenerType |= libfreenect2::Frame::Color;
	if (mRequestDepthFrame | mRequestPointCloud) frameListenerType |= libfreenect2::Frame::Depth;

	mListener = shared_ptr<libfreenect2::SyncMultiFrameListener>(new libfreenect2::SyncMultiFrameListener(frameListenerType));

	if (mRequestColorFrame | mRequestPointCloud) mDevice->setColorFrameListener(mListener.get());
	if (mRequestDepthFrame | mRequestPointCloud) mDevice->setIrAndDepthFrameListener(mListener.get());

	// Configuration of depth processing:
	if (mRequestDepthFrame | mRequestPointCloud) {

		libfreenect2::Freenect2Device::Config config;
		config.MinDepth = mMinDepth;
		config.MaxDepth = mMaxDepth;

		// Setting these to true removes a lot of noise:
		config.EnableBilateralFilter = true;
		config.EnableEdgeAwareFilter = true;

		mDevice->setConfiguration(config); // Must be called after setting up frame listners.

	}

	// Start Kinect sensor:
	if (!mDevice->start()) {
		CI_LOG_E("Faild to start Kinect:" << mSerial);
		return false;
	}

	mIsConnected = true;

	// Setup registration:
	if (mRequestPointCloud) {
		mRegistration = shared_ptr<libfreenect2::Registration>(new libfreenect2::Registration(mDevice->getIrCameraParams(), mDevice->getColorCameraParams()));
		// Have to call these mDevice member functions after calling mDevice->start(). Otherwise they will return wrong parameters.
		mUndistortedDepthFrame = shared_ptr<libfreenect2::Frame>(new libfreenect2::Frame(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT, 4));
		mRegisteredColorFrame = shared_ptr<libfreenect2::Frame>(new libfreenect2::Frame(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT, 4));
		mPointCloud = PointCloudRef(new PointCloud());
		mPointCloud->reserve(POINT_CLOUD_SIZE);
	}

	// Setup worker thread:
	mWorkerThread = thread(bind(&KinectInput::collectAndConvertData, this));

	return true;

}

void bluecadet::kinect2::KinectInput::update() {

	if (!mIsConnected && mAutoReconnection) {
		if (!mReconnectionScheduled) {
			// Terminate worker thread:
			if (mWorkerThread.joinable()) {
				mWorkerThread.join();
			}

			CI_LOG_I("Try to reconnect to Kinect sensor...");
			if (mDevice) {
				mDevice->close();
				mDevice = nullptr;
			}
			setup();

			if (mIsConnected) {
				CI_LOG_I("Success!");
				mReconnectionTimer.stop();
				mReconnectionScheduled = false;
			} else {
				CI_LOG_E("Failure! Will try again in " << mReconnectionInterval << " seconds.");
				mReconnectionTimer.start();
				mReconnectionScheduled = true;
			}
		} else {
			if (mReconnectionTimer.getSeconds() >= mReconnectionInterval) {
				CI_LOG_I("Try to reconnect to Kinect sensor...");
				setup();

				if (mIsConnected) {
					CI_LOG_I("Success!");
					mReconnectionTimer.stop();
					mReconnectionScheduled = false;
				} else {
					CI_LOG_E("Failure! Will try again in " << mReconnectionInterval << " seconds.");
					mReconnectionTimer.start();
					mReconnectionScheduled = true;
				}
			}
		}
	}
}

void KinectInput::destory() {
	
	if (mIsConnected) {

		// Terminate worker thread:
		mAboutToDestoryWorkerThread = true;

		if (mWorkerThread.joinable()) {
			mWorkerThread.join();
		}

		// Disconnect from Kinect:
		if (mDevice) {
			mDevice->stop();
			mDevice->close();
		}

	}
	
}

bool KinectInput::isConnected() {

	return mIsConnected;

}

bool KinectInput::hasNewData() {

	return mHasNewData;
	
}

KinectData KinectInput::getData() {
	KinectData data;
	
	{ // scoped lock
		
		lock_guard<mutex> scopedLock(mDataMutex);
		
		// We don't want user to access our member data, so make a copy of it to return.
		if (mRequestColorFrame) {
			data.colorFrame = Surface8u::create(*mColorFrame);
		}
		
		if (mRequestDepthFrame) {
			data.rawDepthFrame = Channel32f::create(*mRawDepthFrame);
			data.depthFrame = Channel32f::create(*mDepthFrame);
		}
		
		if (mRequestPointCloud) {
			data.pointCloud = PointCloudRef(new PointCloud(*mPointCloud));
		}
		
	} // scoped lock

	mHasNewData = false;

	return data;
	
}

void KinectInput::addFilter(BaseFilterRef filter) {
	
	mPostProcessingPipeline.push_back(filter);
	
}

void KinectInput::clearPostProcessingPipeline() {
	
	mPostProcessingPipeline.clear();
	
}

void KinectInput::collectAndConvertData() {
	
	int numTimeOuts = 0;
	
	while (!mAboutToDestoryWorkerThread) { // Work thread repeats until destroy() is called on the main thread.
		
		if (!mListener->waitForNewFrame(mFrames, WAIT_FOR_NEW_FRAME_TIMEOUT)) { // in milliseconds
			CI_LOG_E("Timed out when collecting new data from Kinect!");

			numTimeOuts++;
			if (numTimeOuts >= NUM_TIMEOUTS_TO_CONFIRM_DISCONNECTION) {
				mIsConnected = false;
				break;
			}

			continue;
		} // Has collected new data now.
		
		// References to local data:
		Surface8uRef newColorFrame;
		Channel32fRef newRawDepthFrame;
		Channel32fRef newDepthFrame;
		
		// Convert color frame to Cinder format:
		if (mRequestColorFrame) {
			
			libfreenect2::Frame *colorFrame = mFrames[libfreenect2::Frame::Color];
			
			cinder::SurfaceChannelOrder surfaceChannelOrder;
			switch (colorFrame->format) {
				case libfreenect2::Frame::RGBX:
					surfaceChannelOrder = cinder::SurfaceChannelOrder::RGBX;
					break;
				case libfreenect2::Frame::BGRX:
					surfaceChannelOrder = cinder::SurfaceChannelOrder::BGRX;
					break;
				default:
					CI_LOG_E("Color frame from Kinect is neither RGBX nor BGRX!");
					surfaceChannelOrder = cinder::SurfaceChannelOrder::BGRX;
					break;
			}
			
			newColorFrame = Surface8uRef(new Surface8u((uint8_t *)(colorFrame->data), colorFrame->width, colorFrame->height, colorFrame->width * colorFrame->bytes_per_pixel, surfaceChannelOrder)); // The Surface8u instance created from raw data doesn't own the data, so we need to make a copy of it in the next step.
			
		}
		
		// Convert depth frame to Cinder format, and apply filters if any:
		if (mRequestDepthFrame) {
			
			libfreenect2::Frame *rawDepthFrame = mFrames[libfreenect2::Frame::Depth];
			
			newRawDepthFrame = Channel32fRef(new Channel32f(rawDepthFrame->width, rawDepthFrame->height, rawDepthFrame->width * rawDepthFrame->bytes_per_pixel, 1, (float *)(rawDepthFrame->data))); // The Channel32f instance created from raw data doesn't own the data, so we need to make a copy of it.
			
			newDepthFrame = Channel32fRef(new Channel32f(*newRawDepthFrame)); // The Channel32f instance created from raw data doesn't own the data, so we need to make a copy of it.
			
			// If there are filters in post-processing pipeline, apply them:
			if (mPostProcessingPipeline.size() > 0) {
				
				cv::Mat depthMat(toOcvRef(*newDepthFrame));
				
				for (auto iter = mPostProcessingPipeline.begin(); iter != mPostProcessingPipeline.end(); iter++) {
					(*iter)->apply(depthMat);
				}
				
				newDepthFrame = Channel32f::create(fromOcv(depthMat));;
				
			}
			
		}
		
		// Undistort depth data:
		if (mRequestPointCloud) {
			
			libfreenect2::Frame *colorFrame = mFrames[libfreenect2::Frame::Color];
			libfreenect2::Frame *depthFrame = mFrames[libfreenect2::Frame::Depth];
			
			mRegistration->apply(colorFrame, depthFrame, mUndistortedDepthFrame.get(), mRegisteredColorFrame.get());
			
		}
		
		{ // scoped lock
			
			lock_guard<mutex> scopedLock(mDataMutex);
			
			if (mRequestColorFrame) {
    
				if (!mColorFrame) { // Allocate memory for the Cinder frame only once.
					
					mColorFrame = Surface8u::create(*newColorFrame);
					
					// mColorFrame = Surface8uRef(new Surface8u(newColorFrame));
					// mColorFrame = Surface8uRef(new Surface8u(Surface8u((uint8_t *)(colorFrame->data), colorFrame->width, colorFrame->height, colorFrame->width * colorFrame->bytes_per_pixel, surfaceChannelOrder)));
					
					/*	A Note on Copy and Move Constructors (Caution: Pitfalls!)
					 
					 Cinder has both copy and move constructors for its Surface (and Channel) class:
					 
					 SurfaceT (const SurfaceT &rhs)
					 // copy constructor which accepts a lvalue reference, and creates a clone of rhs
					 SurfaceT (SurfaceT &&rhs)
					 // move constructor which accepts an rvalue reference,
					 // and moves the contents of rhs to the newly created object
					 
					 Since the Surface constructor we used to create newColorFrame does not copy the data,
					 newColorFrame dos not own the data, and we don't want to pass it to a move constructor.
					 
					 The create() we called in the last line is fine,
					 because there is no version of create() which accepts a rvalue reference and move the data.
					 
					 We can also call Surface constructor here:
					 
					 mColorFrame = Surface8uRef(new Surface8u(newColorFrame));
					 
					 newColorFrame is a variable. A variable is a lvalue. We cann't bind a lvalue to a rvalue reference.
					 Therefore, only the copy constructor can be called.
					 
					 However, if we don't declare the local variable newColorFrame,
					 and instead pass the result of the first Surface constructor to the second one:
					 
					 mColorFrame = Surface8uRef(new Surface8u(Surface8u((uint8_t *)(colorFrame->data), colorFrame->width, colorFrame->height, colorFrame->width * colorFrame->bytes_per_pixel, surfaceChannelOrder)));
					 
					 There will be bad memory accesses in runtime.
					 
					 Because the first Surface constructor returns a nonreference type, which is an rvalue,
					 when the result is immediately passed to the second constructor call,
					 the compiler will prefer move constructor to copy constructor,
					 since the copy constructor requires implicit a conversion to const reference.
					 
					 The result will be: mColorFrame refers to the same shaky data storage
					 which we feeded the first Surface constructor with.
					 
					 */
					
				} else {
					
					mColorFrame->copyFrom(*newColorFrame, newColorFrame->getBounds());
					
				}
				
			}
			
			if (mRequestDepthFrame) {
				
				if (!mRawDepthFrame) { // Allocate memory for the Cinder frame only once.
					mRawDepthFrame = Channel32f::create(*newRawDepthFrame);
				} else {
					mRawDepthFrame->copyFrom(*newRawDepthFrame, newRawDepthFrame->getBounds());
				}
				
				if (!mDepthFrame) { // Allocate memory for the Cinder frame only once.
					mDepthFrame = Channel32f::create(*newDepthFrame);
				} else {
					mDepthFrame->copyFrom(*newDepthFrame, newDepthFrame->getBounds());
				}
				
			}
			
			if (mRequestPointCloud) {
				
				mPointCloud->clear();
				mPointCloud->reserve(POINT_CLOUD_SIZE);
				
				// Convert undistorted depth data to point cloud:
				for (int r = 0; r < DEPTH_FRAME_HEIGHT; r++) {
					for (int c = 0; c < DEPTH_FRAME_WIDTH; c++) {
						
						float x;
						float y;
						float z;
						float rgbFloat;
						
						mRegistration->getPointXYZRGB(mUndistortedDepthFrame.get(), mRegisteredColorFrame.get(), r, c, x, y, z, rgbFloat);
						
						const uint8_t *rgbInt = reinterpret_cast<uint8_t*>(&rgbFloat);
						uint8_t b = rgbInt[0];
						uint8_t g = rgbInt[1];
						uint8_t r = rgbInt[2];
						
						if (isnormal(z)) {
							mPointCloud->push_back(Point(vec3(x, y, z), ColorA(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f)));
						}
						
					}
				}
				
			}
			
			// Set new frame tag:
			mHasNewData = true;
			
		} // scoped lock
		
		// Have to release mFrames after processing raw data:
		mListener->release(mFrames);
		
	}
	
}

