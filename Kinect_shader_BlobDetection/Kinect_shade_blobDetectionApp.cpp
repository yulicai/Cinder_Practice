#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "bluecadet/kinect2/KinectInput.h"
#include "bluecadet/kinect2/filters/Filters.h"
#include "CinderOpenCV.h"
#include "Resources.h"


using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::kinect2;

class Kinect_blob_opencvApp : public App {
public:
	void setup() override;
	void mouseMove(MouseEvent event) override;
	void update() override;
	void draw() override;
	float map(float val, float inMin, float inMax, float outMin, float outMax);
	void cleanup() override;

	KinectInput mKinectInput;
	KinectData mKinectData;
	const float DEPTH_FRAME_WIDTH = 512.0f;
	const float DEPTH_FRAME_HEIGHT = 424.0f;

	RemoveBackgroundRef mRemoveBackgroundFilter;
	int mDepthImageScale = 8;

	const float maxRange = 2.0f;
	const float minRange = 0.0f;



	// FOR OPENCV
	float mThreshold, mBlobMin, mBlobMax;
	int blurAmount = 33; // need to be an odd number
	float radiusToShader = 0.0;
	vec2 mTargetPosition = vec2(0.0);
	int currentBlobNum = 0;
	const int RADIUS_BUFFER_SIZE = 16;
	std::vector<float> recentRadius;
	float averagedRecentRadius = 0.0;
	std::vector<vec2> recentCenters;
	vec2 averagedRecentCenter = vec2(0.0);

	struct Blob {
		float radius = 0.0;
		vec2 loc = vec2(0.0);
	};

	float tempR = 0.2;
	vec2 mMousePos = vec2(0);

protected:
	gl::BatchRef		mBatch;
	gl::Texture2dRef	mTexture;
	gl::Texture2dRef	mTextureA;
	gl::Texture2dRef	mTextureB;
	 
};

void Kinect_blob_opencvApp::setup()
{
	//! Set up windows relative parameters
	setWindowSize(1920, 1080);
	//setWindowSize(960, 540);

	//! Set up Kinect V2
	mKinectInput.setup(mKinectInput.getDeviceSerialNumber(0), PacketPipelineType::OPEN_CL);
	mRemoveBackgroundFilter = RemoveBackgroundRef(new RemoveBackground(minRange, true, true, maxRange));
	mKinectInput.addFilter(mRemoveBackgroundFilter);

	//! DepthToImage
	// RemoveBackfoundFilter needs to go before DepthToImage
	mKinectInput.addFilter(BaseFilterRef(new DepthToImage()));
	mRemoveBackgroundFilter->setMinDistanceFromBackground(0.1);
	mRemoveBackgroundFilter->updateBackground();

	//! Blob Detection params
	mThreshold = 30.0f;
	//! Size threshold for each blob
	mBlobMin = 20.0f;
	mBlobMax = DEPTH_FRAME_WIDTH;

	//! Set up shader
	auto rect = Rectf(ivec2(0), getWindowSize());
	auto geometry = geom::Rect().rect(rect);
	auto shaders = gl::GlslProg::create(
		//vertex shader
		CI_GLSL(150,
			uniform mat4 ciModelViewProjection;
	in vec4 ciPosition;
	in vec2 ciTexCoord0;
	out vec2 vTexCoord0;

	void main(void) {
		vTexCoord0 = ciTexCoord0;
		gl_Position = ciModelViewProjection * ciPosition;
	}
	),
		//fragment shader
		CI_GLSL(150,
			const float M_PI = 3.14159265359;
	uniform sampler2D uTex0;
	uniform sampler2D uTex1;
	uniform vec2 uSize;
	uniform vec2 uBlobCenter;
	uniform float uBlobRadius;
	uniform sampler2D uDepthTex;
	in vec2 vTexCoord0;
	out vec4 oColor;

	vec2 convertedBlogCenter = vec2((uBlobCenter.x / uSize.x), (1.0 - uBlobCenter.y / uSize.y));
	float smoothRadius = 0.5;
	float shadowOffset = 0.07;

	float circle(in vec2 _st, in float _radius, vec2 _center) {
		vec2 dist = _st - _center;
		return 1. - smoothstep(_radius - (_radius*0.01),
			_radius + (_radius*0.01),
			dot(dist, dist)*4.0);
	}



	void main(void) {
		vec2 st = gl_FragCoord.xy / uSize.xy;


// FOR THE ASPECT RATIO
if (uSize.y > uSize.x) {
	st.y *= uSize.y / uSize.x;
	st.y -= (uSize.y*.5 - uSize.x*.5) / uSize.x;

	convertedBlogCenter.y *= uSize.y / uSize.x;
	convertedBlogCenter.y -= (uSize.y*.5 - uSize.x*.5) / uSize.x;
}
else {
	st.x *= uSize.x / uSize.y;
	st.x -= (uSize.x*.5 - uSize.y*.5) / uSize.y;
	convertedBlogCenter.x *= uSize.x / uSize.y;
	convertedBlogCenter.x -= (uSize.x*.5 - uSize.y*.5) / uSize.y;
}


vec4 colorA = texture(uTex0, vTexCoord0);
vec4 colorB = texture(uTex1, vTexCoord0);
//oColor = vec4(vec3(circle(st, uBlobRadius, convertedBlogCenter)), 1.0)*colorA;
oColor = vec4(vec3(circle(st, uBlobRadius-shadowOffset, convertedBlogCenter)), 1.0)*colorA + vec4(1.0 - vec3(circle(st, uBlobRadius, convertedBlogCenter)), 1.0)*colorB;

	}

	)


	); // shader ends

// Initial a batch
mBatch = gl::Batch::create(geometry, shaders);
mBatch->getGlslProg()->uniform("uSize", vec2(getWindowSize()));
mTextureA = gl::Texture2d::create(loadImage(loadAsset("bus01.jpg")));
mTextureB = gl::Texture2d::create(loadImage(loadAsset("bus02.jpg")));
mTextureA->bind(1);
mTextureB->bind(2);

// Pass in uniform
mBatch->getGlslProg()->uniform("uTex0", 1);
mBatch->getGlslProg()->uniform("uTex1", 2);


}

float Kinect_blob_opencvApp::map(float val, float inMin, float inMax, float outMin, float outMax) {
	return outMin + (outMax - outMin) * ((val - inMin) / (inMax - inMin));
}

void Kinect_blob_opencvApp::mouseMove(MouseEvent event)
{
	mMousePos.x = (float)event.getX();
	mMousePos.y = (float)event.getY();
}

void Kinect_blob_opencvApp::update()
{
	mKinectInput.update();

	//! When Kinect is ready and new data comes in
	if (mKinectInput.isConnected() && mKinectInput.hasNewData()) {
		mKinectData = mKinectInput.getData();

		// OpenCV likes 8u, the original depth frame from kinect is 16u
		// Make a surface for openCV
		ci::Surface8u convertedDepthFrame = *mKinectData.depthFrame;
		cv::Mat input = (toOcv(convertedDepthFrame));

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
			//float scaledX = center.x * getWindowSize().x/ DEPTH_FRAME_WIDTH;
			float scaledX = ((boundingBox.br().x - boundingBox.tl().x)/2.0) * getWindowSize().x / DEPTH_FRAME_WIDTH;
			float scaledY = center.y *  getWindowSize().y / DEPTH_FRAME_HEIGHT;
			recentCenters.push_back(vec2(scaledX, scaledY));
			if (recentCenters.capacity() > RADIUS_BUFFER_SIZE) {
				recentCenters.erase(recentCenters.begin());
			}
		
			// DEBUGGING VISIUAL - CURRENT BLOBS
			if (radius > mBlobMin && radius < mBlobMax) {
				// draw the blob if it's in range
				cv::circle(output, center, radius, color);
			}
		}

		// SMOOTH RADIUS
		recentRadius.push_back(maxRectRadius);
		if (recentRadius.capacity() > RADIUS_BUFFER_SIZE) {
			recentRadius.erase(recentRadius.begin());
		}
		float sumRecentRadius = 0.0;
		for (int i = 0; i < recentRadius.size(); i++) {
			sumRecentRadius += recentRadius[i];
		}
		averagedRecentRadius = sumRecentRadius / recentRadius.size();
		radiusToShader = map(averagedRecentRadius, 0.0, 400.0, 0.0, 2.0);
		mBatch->getGlslProg()->uniform("uBlobRadius", radiusToShader);

		// SMOOTH CENTER LOCATION
		for (int i = 0; i < recentCenters.size(); i++) {
			sumRecentCenters += recentCenters[i];
		}
		averagedRecentCenter = vec2(sumRecentCenters.x / recentCenters.size(), sumRecentCenters.y / recentCenters.size());
		mBatch->getGlslProg()->uniform("uBlobCenter", averagedRecentCenter);


		// DEBUGGING PURPOSE
		//for (const auto &currentBlob : currentBlobs) {
		//	console() << currentBlob << endl;
		//}
		

		mTexture = gl::Texture2d::create(fromOcv(output));
	}
	mBatch->getGlslProg()->uniform("uDepthTex", 0);


}

void Kinect_blob_opencvApp::draw()
{
	//gl::clear( Color( 0, 0, 0 ) ); 
	gl::clear();
	
	//gl::draw(mTexture);
	/*
	if (mTexture) {
		mTexture->bind(0);
		mBatch->draw();
	}
	*/
	mBatch->draw();
	if(mTexture) gl::draw(mTexture, Rectf(ivec2(0), ivec2(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT)));

}


void Kinect_blob_opencvApp::cleanup() {

	mKinectInput.destory();

}

CINDER_APP(Kinect_blob_opencvApp, RendererGl)
