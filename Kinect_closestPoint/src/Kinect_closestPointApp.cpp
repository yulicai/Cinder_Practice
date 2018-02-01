#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "bluecadet/kinect2/KinectInput.h"
#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::kinect2;

class Kinect_closestPointApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	void cleanup() override;

	const int DEPTH_FRAME_WIDTH = 512;
	const int DEPTH_FRAME_HEIGHT = 424;

	bool isConnected();
	bool mHorizontalFlip = true;
	bool mVerticalFlip = false;
	vec2 closestPointPos = vec2(0, 0);
	vec2 averagedClosestPoingPos = vec2(0, 0);

	KinectInput mKinectInput;
	KinectData mKinectData;
};

void Kinect_closestPointApp::setup()
{
	setWindowSize(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT);
	setWindowPos(0, 0);

	mKinectInput.setup(mKinectInput.getDeviceSerialNumber(0), PacketPipelineType::OPEN_CL, false, true, false);
}

void Kinect_closestPointApp::mouseDown( MouseEvent event )
{
}

void Kinect_closestPointApp::update()
{
	mKinectInput.update();

	if (mKinectInput.isConnected() && mKinectInput.hasNewData()) {
		mKinectData = mKinectInput.getData();

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

		cv::Mat depthMatNormalized;
		depthMat.convertTo(depthMatNormalized, CV_8U, 1.0 / 8000 * 255);

		vector<cv::Point> nonZeroLocations;
		
		// Mat data need to be converted to range between 0.0-1.0 before feeding into findNonZero()
		cv::findNonZero(depthMatNormalized, nonZeroLocations);
	
		float closestDepth = 8000;
		for (const auto &location : nonZeroLocations) {
			float currentPointDepth = depthMat.at<float>(location.y, location.x);
			if (currentPointDepth < closestDepth) {
				closestDepth = currentPointDepth;
				closestPointPos.x = location.x;
				closestPointPos.y = location.y;
			}
		}
	
		console() << closestDepth << endl;

	}
	averagedClosestPoingPos.x = 0.8 * closestPointPos.x + 0.2 * averagedClosestPoingPos.x;
	averagedClosestPoingPos.y = 0.8 * closestPointPos.x + 0.2 * averagedClosestPoingPos.y;


}

void Kinect_closestPointApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 

	if (mKinectInput.isConnected()) {

		gl::color(1.0, 1.0, 1.0);

		if (mKinectData.depthFrame) gl::draw(gl::Texture2d::create(*(mKinectData.depthFrame)));
		gl::color(Color(1, 0, 0)); 
		gl::drawSolidCircle(averagedClosestPoingPos, 10);
	}
}

bool Kinect_closestPointApp::isConnected()
{
	return mKinectInput.isConnected();
}

void Kinect_closestPointApp::cleanup()
{
	mKinectInput.destory();
}

CINDER_APP( Kinect_closestPointApp, RendererGl )
