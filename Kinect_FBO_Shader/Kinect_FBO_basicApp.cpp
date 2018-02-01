#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "bluecadet/kinect2/KinectInput.h"
#include "bluecadet/kinect2/filters/Filters.h"
#include "CinderOpenCV.h"


using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::kinect2;

class Kinect_FBO_basicApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	void keyDown(KeyEvent event) override;
	float map(float val, float inMin, float inMax, float outMin, float outMax); 
	void cleanup() override;

	KinectInput mKinectInput;
	KinectData mKinectData;

	bool mHorizontalFlip = true;
	bool mVerticalFlip = false;

private:
	const float WINDOW_WIDTH = 1920;
	const float WINDOW_HEIGHT = 1080;
	const float DEPTH_FRAME_WIDTH = 512;
	const float DEPTH_FRAME_HEIGHT = 424;

	

	// Kinect sensing range
	const float MIN_DISTANCE_FROM_BG = 0.1;
	const float maxRange = 4.0f;
	const float minRange = 1.0f;

	RemoveBackgroundRef mRemoveBackgroundFilter;

	// OpenCV part
	float mThreshold, mBlobMin, mBlobMax;
	int blurAmount = 25; // need to be an odd number
	float radiusToShader = 0.0;
	vec2 mTargetPosition = vec2(0.0);
	int currentBlobNum = 0;
	const int RADIUS_BUFFER_SIZE = 16;
	std::vector<float> recentRadius;
	float averagedRecentRadius = 0.0;
	std::vector<vec2> recentCenters;
	vec2 averagedRecentCenter = vec2(0.0);
	gl::Texture2dRef	mTexture; // texture to hold raw opencv result

	void	renderSceneToFbo(ivec2 pos, float radiusToShader);
	void	clearFbo();
	
	// Bus Batch
	gl::Texture2dRef mOldBusTex;
	gl::Texture2dRef mNewBusTex;
	gl::BatchRef mBusBatch;

	// Brush and main canvas
	gl::Texture2dRef mBrushTex;
	gl::BatchRef mBatch;
	gl::FboRef mFbo;
};

void Kinect_FBO_basicApp::setup()
{
	setWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	//setWindowPos(0, 0);
	//setFullScreen(true);
	//hideCursor();
	

	// Setup Kinect
	mKinectInput.setup(mKinectInput.getDeviceSerialNumber(0), PacketPipelineType::OPEN_CL);
	mRemoveBackgroundFilter = RemoveBackgroundRef(new RemoveBackground(minRange, true, true, maxRange));
	mKinectInput.addFilter(mRemoveBackgroundFilter);

	//! DepthToImage
	// RemoveBackfoundFilter needs to go before DepthToImage
	mKinectInput.addFilter(BaseFilterRef(new DepthToImage()));
	mRemoveBackgroundFilter->setMinDistanceFromBackground(MIN_DISTANCE_FROM_BG);
	mRemoveBackgroundFilter->updateBackground();

	//! Blob Detection params
	mThreshold = 30.0f;
	//! Size threshold for each blob
	mBlobMin = 50.0f;
	mBlobMax = DEPTH_FRAME_WIDTH;

	//! Setup bus batch
	mNewBusTex = gl::Texture2d::create(loadImage(loadAsset("textures/busNew.jpg")));
	mOldBusTex = gl::Texture2d::create(loadImage(loadAsset("textures/busOld.jpg")));
	auto rect = geom::Rect().rect(Rectf(ivec2(0), ivec2(WINDOW_WIDTH, WINDOW_HEIGHT)));
	auto busShowShader = gl::GlslProg::create(loadAsset("shaders/vertex.vert"), loadAsset("shaders/bus_combine_mask.frag"));
	mBusBatch = gl::Batch::create(rect, busShowShader);
	mBusBatch->getGlslProg()->uniform("uTex0", 0); // bus top texture
	mBusBatch-> getGlslProg()->uniform("uTex1", 1); // main canvas fbo texture

	//! Set up main/fbo masking batch, fbo result texture
	//mBrushTex = gl::Texture2d::create(loadImage(loadAsset("textures/brush.png"))); // currently not using brush texture
	gl::Fbo::Format canvasFboFormat;
	gl::Texture::Format canvasFboTextureFormat;
	canvasFboTextureFormat.setInternalFormat(GL_RGB32F);
	canvasFboFormat.setColorTextureFormat(canvasFboTextureFormat);
	mFbo = gl::Fbo::create(WINDOW_WIDTH, WINDOW_HEIGHT, canvasFboFormat);
	clearFbo(); // clear existing memory in graphic card
	auto canvasShader = gl::GlslProg::create(loadAsset("shaders/vertex.vert"), loadAsset("shaders/masking.frag"));
	mBatch = gl::Batch::create(rect, canvasShader);
	mBatch->getGlslProg()->uniform("uSize", vec2(getWindowSize())); // for the aspect ratio part in frag shader
	mBatch->getGlslProg()->uniform("uTex0", 0); // Fbo texture
	mBatch->getGlslProg()->uniform("uTex1", 1); // Brush texture, to print on fbo mask base

}



void Kinect_FBO_basicApp::renderSceneToFbo(ivec2 pos, float radiusToShader)
{
	gl::ScopedFramebuffer fbScp(mFbo);

	//! Uncomment for the trail effect
	//gl::clear(Color(0.0, 0.0, 0.0));

	gl::ScopedViewport scopedViewport(mFbo->getSize());

	//! FBO TRANSLATION
	gl::ScopedMatrices scopedMatrices;
	gl::setMatricesWindowPersp(mFbo->getSize());
	gl::translate(-ivec2(WINDOW_WIDTH,WINDOW_HEIGHT)/2+mFbo->getSize()/2),

	// Bind FBO as a texture to 0 position
	// Bind brush as a second texture to 1 position
	mFbo->getColorTexture()->bind(0);
	//mBrushTex->bind(1); // temporarily use brush texture instead of cusmized shader
	mBatch->getGlslProg()->uniform("uBlobRadius", radiusToShader);
	mBatch->getGlslProg()->uniform("uPos", vec2(pos.x,pos.y));
	//mBatch->getGlslProg()->uniform("uPos", vec2(0.5, -0.5) + vec2(-pos.x, pos.y) / vec2(mBrushTex->getSize())); // if using brush texture
	mBatch->draw();

}


void Kinect_FBO_basicApp::clearFbo() {
	gl::ScopedFramebuffer sfb(mFbo);
	gl::ScopedViewport scopedViewport(mFbo->getSize());
	gl::ScopedMatrices scopedMatrices;
	gl::setMatricesWindowPersp(mFbo->getSize());
	gl::translate(-ivec2(WINDOW_WIDTH, WINDOW_HEIGHT) / 2 + mFbo->getSize() / 2);
	gl::clear();
}


void Kinect_FBO_basicApp::update()
{
	mKinectInput.update();

	//! When Kinect is ready and new data comes in
	if (mKinectInput.isConnected() && mKinectInput.hasNewData()) {
		mKinectData = mKinectInput.getData();
		
		// OpenCV likes 8u, the original depth frame from kinect is 16u
		// Make a surface for openCV
		ci::Surface8u convertedDepthFrame = *mKinectData.depthFrame;
		cv::Mat input = (toOcv(convertedDepthFrame));

		// Getting raw depth frame for the depth data
		cv::Mat depthMat(toOcv(*mKinectData.rawDepthFrame));
		if (mHorizontalFlip || mVerticalFlip) {
			cv::Mat flippedMat;
			if (mHorizontalFlip && mVerticalFlip) {
				cv::flip(depthMat, flippedMat, -1);
			}
			else if (mHorizontalFlip) {
				cv::flip(depthMat, flippedMat, 1);
			}
			else {
				cv::flip(depthMat, flippedMat, 0);
			}
			depthMat = flippedMat;
		}

		//! Initial some different img in different phases
		cv::Mat blurred, thresholded, thresholded2, output;

		//! Blur and filter the source input
		//the number for blur need to be an odd one
		cv::medianBlur(input, blurred, blurAmount);
		cv::threshold(blurred, thresholded, mThreshold, 255, CV_THRESH_BINARY);
		cv::threshold(blurred, thresholded2, mThreshold, 255, CV_THRESH_BINARY);

		//! FIND CONTOURS PART
		// 2d vector to store the found contours
		vector<vector<cv::Point>>contours;
		// findContours requires a monochrome image, so we need to convert the image to b&w before feeding into the function:
		cv::Mat bwImage;
		cv::cvtColor(thresholded, bwImage, CV_RGB2GRAY);
		cv::findContours(bwImage, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

		// convert theshold image to color for output
		// so we can draw blobs on it
		cv::cvtColor(bwImage, output, CV_GRAY2RGB);
		// loop through stored contours to get blob detection
		float maxRectRadius = 0.0; // for human tracking
		vec2 sumRecentCenters = vec2(0.0);

		// Remove the trail of previous frame on FBO
		clearFbo();

		for (vector<vector<cv::Point>>::iterator it = contours.begin(); it < contours.end(); it++) {
			// center radius for current blob
			cv::Point2f center;
			cv::Rect boundingBox;

			float radius = 0.0;
			// convert the contour points to a matrix
			vector<cv::Point> pts = *it;
			cv::Mat pointsMatrix = cv::Mat(pts);

			//! BOUNDING BOX
			boundingBox = cv::boundingRect(pointsMatrix);
			float rectRadius = boundingBox.br().x - boundingBox.tl().x;
			if (rectRadius > maxRectRadius) maxRectRadius = rectRadius;

			//! MAKE THE BLOB !
			cv::minEnclosingCircle(pointsMatrix, center, radius);
			cv::Scalar color(0, 255, 0);

			// SMOOTH LOCATION
			//float scaledX = ((boundingBox.br().x - boundingBox.tl().x) / 2.0) *( getWindowSize().x / DEPTH_FRAME_WIDTH); // bounding box loc off
			float centerY = center.y *  getWindowSize().y / DEPTH_FRAME_HEIGHT;
			float centerX = center.x * getWindowSize().x / DEPTH_FRAME_WIDTH;

			recentCenters.push_back(vec2(centerX, centerY));
			if (recentCenters.capacity() > RADIUS_BUFFER_SIZE) {
				recentCenters.erase(recentCenters.begin());
			}

			// Getting the depth data at the center location
			float currentPointDepth = depthMat.at<float>(center.y, center.x);
			float depthAsY = map(currentPointDepth, minRange*1000.0, maxRange*1000.0, 0.0, 400.0);

			// DEBUGGING VISIUAL - CURRENT BLOBS
			if (radius > mBlobMin && radius < mBlobMax) {
				// draw the blob if it's in range
				cv::circle(output, center, radius, color);
				float radiusToShader = map(rectRadius, 0.0, 400.0, 0.0, 1.0); // using bounding box width as radius of lens
				renderSceneToFbo(vec2(centerX, centerY), radiusToShader);
			}
		}
		

		//! Debugging visual output
		mTexture = gl::Texture2d::create(fromOcv(output));
		//mTexture = gl::Texture2d::create(convertedDepthFrame);
	}
	

}

void Kinect_FBO_basicApp::draw()
{
	gl::clear();
	gl::color(Color::white());
	gl::draw(mOldBusTex);
	mNewBusTex->bind(0);
	mFbo->getColorTexture()->bind(1);
	mBusBatch->draw(); // the final result of the combination of fbo as a mask and original bus textures

	//! Debugging visual output
	//if (mTexture) gl::draw(mTexture, Rectf(ivec2(0), ivec2(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT)));
}

void Kinect_FBO_basicApp::keyDown(KeyEvent event) {

	if (event.getChar() == ' ') mRemoveBackgroundFilter->updateBackground();
	if (event.getChar() == 'f') setFullScreen(true);

}

float Kinect_FBO_basicApp::map(float val, float inMin, float inMax, float outMin, float outMax) {
	return outMin + (outMax - outMin) * ((val - inMin) / (inMax - inMin));
}

void Kinect_FBO_basicApp::cleanup() {
	mKinectInput.destory();
}

//CINDER_APP( Kinect_FBO_basicApp, RendererGl )
CINDER_APP(Kinect_FBO_basicApp, RendererGl(RendererGl::Options().msaa(8)))