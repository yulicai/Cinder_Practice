#include "HoverZoneManager.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::kinect2;

HoverZoneManager::~HoverZoneManager() {

}

bool HoverZoneManager::setup(const ivec2 appSize) {

	if (!mKinectInput.setup(mKinectInput.getDeviceSerialNumber(0), PacketPipelineType::OPEN_CL, false, true, false)) {
		CI_LOG_E("Failed to initialize Kinect!");
		return false;
	}

	mKinectInput.addFilter(DepthToImageRef(new DepthToImage()));

	mAppSize = appSize;

	// Create a new gl::Context just worker thread
	gl::ContextRef workerContext = gl::Context::create(gl::context());

	// Setup worker thread:
	mWorkerThread = thread(bind(&HoverZoneManager::detectTouch, this, workerContext));

	return true;
}

void HoverZoneManager::update() {
	{
		lock_guard<mutex> scopedLock(mHoverZonesMutex);

		for (auto &hoverZone : mHoverZonesToDisable) {
			hoverZone->disable();
		}

		mHoverZonesToDisable.clear();

		for (auto &hoverZone : mHoverZonesToEnable) {
			hoverZone->enable();
		}

		mHoverZonesToEnable.clear();
	}

	mKinectInput.update();

	if (isConnected()) {
		if (mHasUpdatedDepthData) {
			// Emit signal from the most intersected hot zone:
			bool hasDetectedTouch = false;
			int mostIntersectedHoverZoneIndex = 0;
			int maxIntersectionCount = 0;

			{
				lock_guard<mutex> scopedLock(mHoverZonesMutex);

				for (int i = 0; i < mHoverZones.size(); i++) {
					auto &hoverZone = mHoverZones[i];

					if (hoverZone->enabled &&
						((hoverZone->detectionFrameCount < mDetectionFrameThreshold &&
						  hoverZone->intersectionCount > mIntersectionInitialThreshold) ||
						  (hoverZone->detectionFrameCount >= mDetectionFrameThreshold &&
						   hoverZone->intersectionCount > mIntersectionOngoingThreshold))) {

						hasDetectedTouch = true;
						if (hoverZone->intersectionCount > maxIntersectionCount) {
							mostIntersectedHoverZoneIndex = i;
							maxIntersectionCount = hoverZone->intersectionCount;
						}
					}
				}

				if (hasDetectedTouch) {
					for (int i = 0; i < mHoverZones.size(); i++) {
						if (mHoverZones[i]->enabled) {
							if (i == mostIntersectedHoverZoneIndex) {
								mHoverZones[i]->detectionFrameCount++;
								if (mHoverZones[i]->detectionFrameCount >= mDetectionFrameThreshold) {
									mHoverZones[i]->trigger();
								}
							} else {
								if (mHoverZones[i]->detectionFrameCount >= mDetectionFrameThreshold) {
									mHoverZones[i]->cancel();
								}
								mHoverZones[i]->detectionFrameCount = 0;
							}
						}
					}
				} else {
					for (int i = 0; i < mHoverZones.size(); i++) {
						if (mHoverZones[i]->enabled) {
							if (mHoverZones[i]->detectionFrameCount >= mDetectionFrameThreshold) {
								mHoverZones[i]->cancel();
							}
							mHoverZones[i]->detectionFrameCount = 0;
						}
					}
				}
			}

			mHasUpdatedDepthData = false;
		}
	} else {
		lock_guard<mutex> scopedLock(mHoverZonesMutex);

		for (int i = 0; i < mHoverZones.size(); i++) {
			mHoverZones[i]->intersectionCount = -1;
			mHoverZones[i]->detectionFrameCount = 0;
		}
	}
}

void HoverZoneManager::draw(const vec2 offset) {

	gl::ScopedMatrices scopedMatrices;
	gl::translate(offset);

	{
		lock_guard<mutex> scopedLock(mDepthTextureMutex);

		if (mDepthTexture) {
			gl::ScopedColor color(0, 1, 0);
			gl::draw(mDepthTexture);
		}
	}

	{
		gl::ScopedMatrices scopedMatrices;
		gl::translate(mSensingAreaCenter);
		gl::rotate(toRadians(mSensingAreaRotation));
		gl::scale(mSensingAreaSize.load() / vec2(mAppSize.load()));
		gl::translate(-vec2(mAppSize.load()) / vec2(2));

		Rectf sensingArea(vec2(0), vec2(mAppSize.load()));

		gl::ScopedColor transparentWhite(1, 1, 1, 0.2);
		gl::drawSolidRect(sensingArea);

		gl::ScopedColor opageWhite(1, 1, 1, 1);
		gl::drawStrokedRect(sensingArea);

		{
			lock_guard<mutex> scopedLock(mHoverZonesMutex);

			for (auto &hoverZone : mHoverZones) {
				if (hoverZone->enabled) {
					gl::ScopedColor transparentYellow(1, 1, 0, 0.5);
					gl::drawSolidRect(hoverZone->zone);
				}
			}
		}
	}
}

void HoverZoneManager::cleanup() {

	// Terminate worker thread:
	mAboutToDestoryWorkerThread = true;

	if (mWorkerThread.joinable()) {
		mWorkerThread.join();
	}

	// Disconnect from Kinect:
	mKinectInput.destory();
}

bool HoverZoneManager::isConnected() {
	return mKinectInput.isConnected();
}

HoverZoneRef HoverZoneManager::registerHoverZone(const Rectf &zone,
												 const function<void()>& trigger,
												 const function<void()>& cancel) {
	HoverZoneRef newHoverZone = HoverZoneRef(new HoverZone());

	newHoverZone->zone = zone;
	newHoverZone->trigger = trigger;
	newHoverZone->cancel = cancel;

	{
		lock_guard<mutex> scopedLock(mHoverZonesMutex);

		mHoverZones.push_back(newHoverZone);
		return newHoverZone;
	}
}

void HoverZoneManager::unregisterHoverZone(const HoverZoneRef &zone) {
	{
		lock_guard<mutex> scopedLock(mHoverZonesMutex);

		for (auto iter = mHoverZones.begin(); iter != mHoverZones.end(); iter++) {
			if (*iter == zone) {
				mHoverZones.erase(iter);
				return;
			}
		}
	}
}

void HoverZoneManager::enableHoverZoneInNextFrame(const HoverZoneRef & zone) {
	mHoverZonesToEnable.push_back(zone);
}

void HoverZoneManager::disableHoverZoneInNextFrame(const HoverZoneRef & zone) {
	mHoverZonesToDisable.push_back(zone);
}

void HoverZoneManager::loadConfiguration(const fs::path & jsonPath) {
	JsonTree configuration;

	try {
		configuration = JsonTree(loadAsset(jsonPath)).getChild("kinectConfiguration");
	} catch (std::exception &exc) {
		CI_LOG_F("Failed to load kinect configuration! Exception type: " << System::demangleTypeName(typeid(exc).name()) << ", what: " << exc.what());
		throw;
	}

	mAppSize = ivec2(configuration["appSize"]["x"].getValue<int>(),
					 configuration["appSize"]["y"].getValue<int>());

	mHorizontalFlip = configuration["horizontalFlip"].getValue<bool>();
	mVerticalFlip = configuration["verticalFlip"].getValue<bool>();

	mSensingAreaCenter = vec2(configuration["sensingAreaCenter"]["x"].getValue<float>(),
							  configuration["sensingAreaCenter"]["y"].getValue<float>());

	mSensingAreaSize = vec2(configuration["sensingAreaSize"]["x"].getValue<float>(),
							configuration["sensingAreaSize"]["y"].getValue<float>());

	mSensingAreaRotation = configuration["sensingAreaRotation"].getValue<float>();

	mDepthDetectionLowerThreshold = configuration["depthDetectionLowerThreshold"].getValue<float>();
	mDepthDetectionHigherThreshold = configuration["depthDetectionHigherThreshold"].getValue<float>();

	mIntersectionInitialThreshold = configuration["intersectionInitialThreshold"].getValue<unsigned int>();
	mIntersectionOngoingThreshold = configuration["intersectionOngoingThreshold"].getValue<unsigned int>();

	mDetectionFrameThreshold = configuration["detectionFrameThreshold"].getValue<unsigned int>();

	{
		lock_guard<mutex> scopedLock(mHoverZonesMutex);

		for (auto &hoverZone : mHoverZones) {
			hoverZone->hasAverageDepth = false;
		}
	}
}

void HoverZoneManager::saveConfiguration(const fs::path & jsonPath) const {
	JsonTree configuration = JsonTree::makeObject("kinectConfiguration");

	JsonTree appSize = JsonTree::makeObject("appSize");
	appSize.addChild(JsonTree("x", mAppSize.load().x));
	appSize.addChild(JsonTree("y", mAppSize.load().y));
	configuration.addChild(appSize);

	configuration.addChild(JsonTree("horizontalFlip", mHorizontalFlip));
	configuration.addChild(JsonTree("verticalFlip", mVerticalFlip));

	JsonTree sensingAreaCenter = JsonTree::makeObject("sensingAreaCenter");
	sensingAreaCenter.addChild(JsonTree("x", mSensingAreaCenter.load().x));
	sensingAreaCenter.addChild(JsonTree("y", mSensingAreaCenter.load().y));
	configuration.addChild(sensingAreaCenter);

	JsonTree sensingAreaSize = JsonTree::makeObject("sensingAreaSize");
	sensingAreaSize.addChild(JsonTree("x", mSensingAreaSize.load().x));
	sensingAreaSize.addChild(JsonTree("y", mSensingAreaSize.load().y));
	configuration.addChild(sensingAreaSize);

	configuration.addChild(JsonTree("sensingAreaRotation", mSensingAreaRotation));

	configuration.addChild(JsonTree("depthDetectionLowerThreshold", mDepthDetectionLowerThreshold));
	configuration.addChild(JsonTree("depthDetectionHigherThreshold", mDepthDetectionHigherThreshold));

	configuration.addChild(JsonTree("intersectionInitialThreshold", mIntersectionInitialThreshold));
	configuration.addChild(JsonTree("intersectionOngoingThreshold", mIntersectionOngoingThreshold));

	configuration.addChild(JsonTree("detectionFrameThreshold", mDetectionFrameThreshold));

	JsonTree::WriteOptions writeOptions;
	writeOptions
		.createDocument(true)
		.indented(true);
	configuration.write(getAssetPath(jsonPath), writeOptions);
}

ivec2 HoverZoneManager::getAppSize() {
	return mAppSize;
}

void HoverZoneManager::setAppSize(const ivec2 size) {
	mAppSize = size;
}

ivec2 HoverZoneManager::getSize() {
	return ivec2(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT);
}

bool HoverZoneManager::getHorizontalFlip() {
	return mHorizontalFlip;
}

void HoverZoneManager::setHorizontalFlip(bool flip) {
	mHorizontalFlip = flip;
}

bool HoverZoneManager::getVerticalFlip() {
	return mVerticalFlip;
}

void HoverZoneManager::setVerticalFlip(bool flip) {
	mVerticalFlip = flip;
}

vec2 HoverZoneManager::getSensingAreaCenter() {
	return mSensingAreaCenter;
}

void HoverZoneManager::setSensingAreaCenter(const vec2 center) {
	mSensingAreaCenter = center;
}

vec2 HoverZoneManager::getSensingAreaSize() {
	return mSensingAreaSize;
}

void HoverZoneManager::setSensingAreaSize(const vec2 size) {
	mSensingAreaSize = size;
}

float HoverZoneManager::getSensingAreaRotation() {
	return mSensingAreaRotation;
}

void HoverZoneManager::setSensingAreaRotation(const float rotation) {
	mSensingAreaRotation = rotation;
}

float HoverZoneManager::getDepthDetectionLowerThreshold() {
	return mDepthDetectionLowerThreshold;
}

void HoverZoneManager::setDepthDetectionLowerThreshold(const float threshold) {
	mDepthDetectionLowerThreshold = threshold;
}

float HoverZoneManager::getDepthDetectionHigherThreshold() {
	return mDepthDetectionHigherThreshold;
}

void HoverZoneManager::setDepthDetectionHigherThreshold(const float threshold) {
	mDepthDetectionHigherThreshold = threshold;
}

unsigned int HoverZoneManager::getIntersectionInitialThreshold() {
	return mIntersectionInitialThreshold;
}

void HoverZoneManager::setIntersectionInitialThreshold(const unsigned int threshold) {
	mIntersectionInitialThreshold = threshold;
}

unsigned int HoverZoneManager::getIntersectionOngoingThreshold() {
	return mIntersectionOngoingThreshold;
}

void HoverZoneManager::setIntersectionOngoingThreshold(const unsigned int threshold) {
	mIntersectionOngoingThreshold = threshold;
}

unsigned int HoverZoneManager::getDetectionFrameThreshold() {
	return mDetectionFrameThreshold;
}

void HoverZoneManager::setDetectionFrameThreshold(const unsigned int threshold) {
	mDetectionFrameThreshold = threshold;
}

HoverZoneManager::HoverZoneManager() {

}

void HoverZoneManager::detectTouch(gl::ContextRef context) {
	ci::ThreadSetup threadSetup;
	context->makeCurrent();

	while (!mAboutToDestoryWorkerThread) { // Work thread repeats until destroy() is called on the main thread.
		if (mKinectInput.hasNewData()) {

			// Get depth data:
			KinectData mKinectData = mKinectInput.getData();

			cv::Mat depthMat(toOcv(*mKinectData.rawDepthFrame));
			if (mHorizontalFlip || mVerticalFlip) {
				cv::Mat flippedMat;
				if (mHorizontalFlip && mVerticalFlip) {
					cv::flip(depthMat, flippedMat, -1);
				} else if (mHorizontalFlip) {
					cv::flip(depthMat, flippedMat, 1);
				} else {
					cv::flip(depthMat, flippedMat, 0);
				}
				depthMat = flippedMat;
			}

			// Get depth texture:
			cv::Mat textureMat(toOcv(*mKinectData.depthFrame));
			if (mHorizontalFlip || mVerticalFlip) {
				cv::Mat flippedMat;
				if (mHorizontalFlip && mVerticalFlip) {
					cv::flip(textureMat, flippedMat, -1);
				} else if (mHorizontalFlip) {
					cv::flip(textureMat, flippedMat, 1);
				} else {
					cv::flip(textureMat, flippedMat, 0);
				}
				textureMat = flippedMat;
			}

			{
				lock_guard<mutex> scopedLock(mDepthTextureMutex);

				mDepthTexture = gl::Texture::create(fromOcv(textureMat));
			}

			// Check hot zones:
			{
				lock_guard<mutex> scopedLock(mHoverZonesMutex);

				for (auto &hoverZone : mHoverZones) {
					if (hoverZone->enabled) {
						// Use hot zone to make a mask:
						auto maskFbo = gl::Fbo::create(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT);
						{
							gl::ScopedFramebuffer scopedFramebuffer(maskFbo);
							gl::clear(Color(0, 0, 0));
							gl::ScopedViewport scopedViewport(maskFbo->getSize());
							gl::ScopedMatrices scopedMatrices;
							gl::setMatricesWindowPersp(maskFbo->getSize());
							gl::translate(mSensingAreaCenter);
							gl::rotate(toRadians(mSensingAreaRotation));
							gl::scale(mSensingAreaSize.load() / vec2(mAppSize.load()));
							gl::translate(-vec2(mAppSize.load()) / vec2(2));

							gl::ScopedColor color(1, 1, 1);
							gl::drawSolidRect(hoverZone->zone);
						}
						Surface maskSurface = Surface::create(maskFbo->getColorTexture()->createSource())->clone();
						cv::Mat maskMat(toOcv(maskSurface, CV_8UC1));

						// Get overlapping part from depth image:
						cv::Mat depthMatNormalized;
						depthMat.convertTo(depthMatNormalized, CV_8U, 1.0 / 8000 * 255);

						cv::Mat overlapMat;
						depthMatNormalized.copyTo(overlapMat, maskMat);

						if (!hoverZone->hasAverageDepth) {
							// Calculate average depth of hot zones (one time, when setup):
							float averageDepth = 0;
							vector<cv::Point> nonZeroLocations;
							cv::findNonZero(overlapMat, nonZeroLocations);
							for (const auto &location : nonZeroLocations) {
								averageDepth += depthMat.at<float>(location.y, location.x);
							}
							averageDepth /= nonZeroLocations.size();

							hoverZone->averageDepth = averageDepth;
							hoverZone->hasAverageDepth = true;
							hoverZone->intersectionCount = -1;
							hoverZone->detectionFrameCount = 0;

						} else {
							// Touch detection:
							unsigned int intersectionCount = 0;
							vector<cv::Point> nonZeroLocations;
							cv::findNonZero(overlapMat, nonZeroLocations);
							for (const auto &location : nonZeroLocations) {
								float depth = hoverZone->averageDepth - depthMat.at<float>(location.y, location.x);
								if (depth >= mDepthDetectionLowerThreshold &&
									depth <= mDepthDetectionHigherThreshold) {
									intersectionCount++;
								}
							}

							hoverZone->intersectionCount = intersectionCount;
						}
					}
				}
			}

			mHasUpdatedDepthData = true;
		}
	}
}