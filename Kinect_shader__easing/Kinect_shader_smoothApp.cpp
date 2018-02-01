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

class Kinect_shader_smoothApp : public App {
  public:
	void setup() override;
	void mouseMove(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;

	KinectInput mKinectInput;
	KinectData mKinectData;
	bool kinectMode = true;
	vec2 mMousePos;
	bool mouseDragging = false;
	float draggingTime = 0.0;

	RemoveBackgroundRef mRemoveBackgroundFilter;
	int mDepthImageScale = 8;

	bool mHorizontalFlip = true;
	bool mVerticalFlip = false;
	float closestDepth = 8000;
	vec2 closestPointPos = vec2(0, 0);
	vec2 averagedClosestPoingPos = vec2(0, 0);
	float averagedClosestDepth = 10.0;

	const int RECENT_CLOSEST_BUFFER_SIZE = 10;
	std::vector<vec2> recentClosestPoints;
	std::vector<float>::size_type sz;
	std::vector<float> recentClosestDepths;
	std::vector<float>::size_type depthzSZ;


	const int DEPTH_FRAME_WIDTH = 512;
	const int DEPTH_FRAME_HEIGHT = 424;
	float scalerX = 1.0;
	float scalerY = 1.0;

	const float maxRange = 4.0f;
	const float minRange = 0.0f;

	vec2 mappedClosestPoint = vec2 (0, 0);

protected:
	gl::BatchRef mBatch;
	gl::Texture2dRef mTextureA;
	gl::Texture2dRef mTextureB;
};
////////////////////////////
////////////////////////////


void Kinect_shader_smoothApp::setup()
{	
	//! Set up windows relative parameters
	setWindowSize(1920, 1080);
	//setWindowSize(960, 540);
	setWindowPos(0, 0);
	scalerX = (float)getWindowWidth() / DEPTH_FRAME_WIDTH;
	scalerY = (float)getWindowHeight() / DEPTH_FRAME_HEIGHT;

	//! Set up Kinect V2
	mKinectInput.setup(mKinectInput.getDeviceSerialNumber(0), PacketPipelineType::OPEN_CL);
	mRemoveBackgroundFilter = RemoveBackgroundRef(new RemoveBackground(minRange, true, true, maxRange));
	mKinectInput.addFilter(mRemoveBackgroundFilter);
	
	//! DepthToImage
	// RemoveBackfoundFilter needs to go before DepthToImage
	mKinectInput.addFilter(BaseFilterRef(new DepthToImage()));
	mRemoveBackgroundFilter->setMinDistanceFromBackground(0.1);
	mRemoveBackgroundFilter->updateBackground();

	//Set up / input Shader
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
	uniform float ciElapsedSeconds;
	uniform vec2 uSize;
	uniform vec2 uClosestPoint;
	uniform float uClosestDepth;
	in vec2 vTexCoord0;
	out vec4 oColor;

	vec2 center = vec2((uClosestPoint.x / uSize.x), (1.0 - uClosestPoint.y / uSize.y));
	float smoothRadius = 0.5;
	float shadowOffset = 0.03;

	float circle(in vec2 _st, in float _radius, vec2 _center) {
		//vec2 dist = _st - vec2((uClosestPoint.x / uSize.x), (1.0 - uClosestPoint.y / uSize.y));
		vec2 dist = _st - _center;
		return 1. - smoothstep(_radius*0.2 - (_radius*0.15),
			_radius*0.2 + (_radius*0.15),
			dot(dist,dist)*uClosestDepth*uClosestDepth /2000.0);
	}

	float circleSDF(vec2 st, vec2 placement, float density) {
		return length(st - placement) * density;
	}


	void main(void) {
		vec2 st = gl_FragCoord.xy / uSize.xy;

		// FOR THE ASPECT RATIO
		if (uSize.y > uSize.x) {
			st.y *= uSize.y / uSize.x;
			st.y -= (uSize.y*.5 - uSize.x*.5) / uSize.x;

			center.y *= uSize.y / uSize.x;
			center.y -= (uSize.y*.5 - uSize.x*.5) / uSize.x;
		}
		else {
			st.x *= uSize.x / uSize.y;
			st.x -= (uSize.x*.5 - uSize.y*.5) / uSize.y;
			center.x *= uSize.x / uSize.y;
			center.x -= (uSize.x*.5 - uSize.y*.5) / uSize.y;
		}
		vec4 colorA = texture(uTex0, vTexCoord0);
		vec4 colorB = texture(uTex1, vTexCoord0);
		oColor = vec4(vec3(circle(st, smoothRadius-shadowOffset,center)), 1.0)*colorA + vec4(1.0-vec3(circle(st, smoothRadius,center)), 1.0)*colorB;
	}
	)
		); // shader ends

		   // initial a batch
	mBatch = gl::Batch::create(geometry, shaders);

	mTextureA = gl::Texture2d::create(loadImage(loadAsset("bus01.jpg")));
	mTextureB = gl::Texture2d::create(loadImage(loadAsset("bus02.jpg")));
	mTextureA->bind(0);
	mTextureB->bind(1);

	// Pass in uniform
	mBatch->getGlslProg()->uniform("uTex0", 0);
	mBatch->getGlslProg()->uniform("uTex1", 1);
	mBatch->getGlslProg()->uniform("uSize", vec2(getWindowSize()));
}



void Kinect_shader_smoothApp::mouseDown( MouseEvent event )
{

	
	mRemoveBackgroundFilter->setMinDistanceFromBackground(maxRange);
	mRemoveBackgroundFilter->updateBackground();
}

void Kinect_shader_smoothApp::mouseDrag(MouseEvent event)
{
	mouseMove(event);
	mouseDragging = true;
}

void Kinect_shader_smoothApp::mouseMove(MouseEvent event)
{
	mMousePos.x = (float)event.getX();
	mMousePos.y = (float)event.getY();
}

void Kinect_shader_smoothApp::mouseUp(MouseEvent event)
{
	mouseDragging = false;
	//draggingTime = 0.0;
}

void Kinect_shader_smoothApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'k') kinectMode = true;
	if (event.getChar() == 'm') kinectMode = false;
	if (event.getChar() == 'c') draggingTime = 0.0;

}



void Kinect_shader_smoothApp::update()
{
	mKinectInput.update();

	if (mKinectInput.isConnected() && mKinectInput.hasNewData()) {
		mKinectData = mKinectInput.getData();

		// Use filted data to feed into openCV
		cv::Mat depthMat(toOcv(*mKinectData.depthFrame));
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
		depthMat.convertTo(depthMatNormalized, CV_8U, 255);
		  
		vector<cv::Point> nonZeroLocations;
		// Mat data need to be converted to range between 0.0-255.0 before feeding into findNonZero()
		cv::findNonZero(depthMatNormalized, nonZeroLocations);
		closestDepth = 8000;
		for (const auto &location : nonZeroLocations) {
			float currentPointDepth = depthMat.at<float>(location.y, location.x);
			if (currentPointDepth < closestDepth) {
				closestDepth = currentPointDepth;
				closestPointPos.x = location.x;
				closestPointPos.y = location.y;
			}
		}

		// A cluster of recent point location data
		recentClosestPoints.push_back(closestPointPos);
		if (recentClosestPoints.capacity()>RECENT_CLOSEST_BUFFER_SIZE) {
			recentClosestPoints.erase(recentClosestPoints.begin());
		}
		recentClosestDepths.push_back(closestDepth);
		if (recentClosestDepths.capacity() > RECENT_CLOSEST_BUFFER_SIZE) {
			recentClosestDepths.erase(recentClosestDepths.begin());
		}

		// DEBUGGING PURPOSE -- Print out the recentPoints vector(array)
		//for (const auto &recentPoint : recentClosestPoints) {
			//console() << recentPoint << endl;
		//}

		// Get the average data out of the cluster points we got before
		vec2 sumClosestPoints = vec2(0, 0);
		for (int i = 0; i < recentClosestPoints.size(); i++) {
			sumClosestPoints += recentClosestPoints[i];
		}
		averagedClosestPoingPos = vec2(sumClosestPoints.x / recentClosestPoints.size(), sumClosestPoints.y / recentClosestPoints.size());

		float sumClosestDepths = 0.0;
		for (int i = 0; i < recentClosestDepths.size(); i++) {
			sumClosestDepths += recentClosestDepths[i];
		}
		averagedClosestDepth = sumClosestDepths / recentClosestDepths.size();
		//averagedClosestDepth *= 1000.0;

		// Scale data to current canvas size
		mappedClosestPoint.x = getWindowSize().x - averagedClosestPoingPos.x*scalerX;
		mappedClosestPoint.y = averagedClosestPoingPos.y*scalerY;

	
	}
	if (mouseDragging) draggingTime+=1.0;

	//put changing data here so it gets to update
	if (kinectMode) {
		CI_LOG_D("kinecttttttt!");
		mBatch->getGlslProg()->uniform("uClosestPoint", mappedClosestPoint);
		if (averagedClosestDepth < maxRange * 1000.0) {
			float toShaderDepth = 1000.0 * averagedClosestDepth;
			//CI_LOG_D(toShaderDepth);
			mBatch->getGlslProg()->uniform("uClosestDepth", toShaderDepth);
		}
	}
	else {
		mBatch->getGlslProg()->uniform("uClosestPoint", mMousePos);
		float mouseSimulationDepth = 200.0 / (draggingTime/50.0);
		mBatch->getGlslProg()->uniform("uClosestDepth", mouseSimulationDepth);
	}
}

void Kinect_shader_smoothApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	mBatch->draw();
	/*
	if (mKinectInput.isConnected()) {

		gl::color(1.0, 1.0, 1.0);

		if (mKinectData.depthFrame) gl::draw(gl::Texture2d::create(*(mKinectData.depthFrame)), Rectf(ivec2(0), ivec2(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT)));
		gl::color(1.0,0.0, 0.0);
		gl::drawSolidCircle(mappedClosestPoint, 10);

	}
	*/

	// sgl::drawString();
}


void Kinect_shader_smoothApp::cleanup() {

	mKinectInput.destory();

}

CINDER_APP(Kinect_shader_smoothApp, RendererGl, [](ci::app::App::Settings * settings) {
	//settings->setConsoleWindowEnabled(true);
});
