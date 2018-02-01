#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include <vector>
#include "Particle.h"
#include "ParticleSystem.h"
#include "FlowField.h"
#include "InteractiveField.h"
#include "cinder\params\Params.h"


using namespace ci;
using namespace ci::app;
using namespace std;

class Particle_perlinApp : public App {
  public:
	void setup() override;
	void mouseMove( MouseEvent event ) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void addParticleSystem(vec3 triggerLoc, int amt);
	void switchMode();
	void setDefaultCameraValues();
	void update() override;
	void draw() override;


	gl::TextureRef				mBgTexture;
	vector<ParticleSystem>		mParticleSystems;
	vector<Particle>			mParticles;
	Flowfield					mFlowField;
	InteractiveField			mInteractiveField;

	params::InterfaceGlRef		mParams;
	CameraPersp					mObjectCam;

	

	const int PARTICLE_AMT = 200;
	const float RADIUS_UNIT = 4.0f;
	const float IMG_WIDTH = 1080.0f;
	const float IMG_HEIGHT = 480.0f;
	vec3 mouseloc;
	vec3 mMouseDownLoc;
	bool mMouseDownMode = false;


/*adjustable in params*/
	// params for the main camera
	vec3 mEyePoint;
	vec3 mLookAt;
	float mFov;
	float mAspectRatio;
	float mNearPlane;
	float mFarPlane;
	vec2 mLensShift;

	// params for background texture position
	float mBgZPos = 10.0f;
	vec2 mBgScalor = vec2 (1.0f);

	// Different ways of Interaction
	bool mOnAddParticlesMode = false;

// PARTICLE RADIUS 
float mRadius = RADIUS_UNIT;	// initial radius
float mMaxRadius = RADIUS_UNIT*3.0f;	// initial largest sizing

// DISTRIBUTION
// restricted height
int mCuttingHeight = 50;
// weight for the central force
float mCentralForceWeight = 0.02f;	// best ranged bt 0.01-0.07

// OUT SIDE FORCES
// perlin flow
float mNoiseRange = 3.0f; // better in range of 7.0f-30.0f, 3.0  makes it has a tendency to move towards right
float mPerlinWeight = 0.01f; // range between 0.0f to 4.0f, the smaller the slower

// mouse
float mInteractiveWeight = 0.5f;// range between 0.3 to 3.0f
int mAttractNum = 6;

// INTERNAL FORCE
// separate 
float desiredseparation = 50.0f;
float mSeparateWeight = 0.2f;

};

void Particle_perlinApp::setDefaultCameraValues() {
	
	mEyePoint = vec3(getWindowWidth()/2.0f, getWindowHeight()/2.0f, 50.0f);
	mLookAt = vec3(getWindowWidth() / 2.0f, getWindowHeight() / 2.0f,0.0f);
	mFov = 160;
	mAspectRatio = getWindowAspectRatio();
	mNearPlane = 0.5f;
	mFarPlane = 1500.0f;

}

void Particle_perlinApp::setup()
{
	//hideCursor();
	setWindowSize(1080, 480);
	setWindowPos(0, 0);

	gl::enableDepthRead();
	gl::enableDepthWrite();
	mBgTexture = gl::Texture::create(loadImage(loadAsset("summerSmall.jpg")),gl::Texture::Format().loadTopDown(true));

	setDefaultCameraValues();

	/*PARAMETERS SETTINGS*/
	///////////////////////
	mParams = params::InterfaceGl::create("SETTINGS", vec2(250, 460));

	// PARTICLE INTERACT MODE
	mParams->addText("text", "label='Attract and Enlarge Mode'");
	mParams->addButton("Switch Interaction Mode Toggle", bind(&Particle_perlinApp::switchMode, this));
	
	// PARTICLE SIZING
	mParams->addParam("Largest Radius Scale", &mMaxRadius).min(RADIUS_UNIT).max(12 * RADIUS_UNIT).updateFn([this] {
		for (auto &p : mParticles) {
			p.mMaxRadius = mMaxRadius;
		}
	});

	// GROUP -- DISTRIBUTION
	mParams->addParam("Cutting Height", &mCuttingHeight).min(0).max(getWindowHeight() / 4).group("DISTRIBUTION").updateFn([this] {
		for (auto &p : mParticles) {
			p.mCuttingHeight = mCuttingHeight;
		}
	});
	mParams->addParam("Central Force Weight", &mCentralForceWeight).group("DISTRIBUTION").step(0.001).updateFn([this] {
		mFlowField.mCentralWeight = mCentralForceWeight;
	});

	// GROUP -- OUT SIDE FORCE
	mParams->addParam("Perlin Weight", &mPerlinWeight).group("OUT SIDE FORCE").step(0.01).updateFn([this] {
		mFlowField.mPerlinWeight = mPerlinWeight;
	});
	mParams->addParam("Perlin Range", &mNoiseRange).group("OUT SIDE FORCE").updateFn([this] {
		mFlowField.applyForces(mParticles, mNoiseRange);
	});
	mParams->addParam("Interactive Weight", &mInteractiveWeight).step(0.1).group("OUT SIDE FORCE").updateFn([this] {
		mInteractiveField.applyForces(mParticles, mouseloc, mAttractNum, mInteractiveWeight,mEyePoint.z);
	});
	mParams->addParam("Interactive Attract Range", &mAttractNum).group("OUT SIDE FORCE").min(1).max(50);

	// GROUP -- INTERNAL FORCE
	mParams->addParam("Desired Separation", &desiredseparation).group("INTERNAL FORCE").updateFn([this] {
		for (auto &p : mParticles) {
			p.update(mParticles, desiredseparation, mSeparateWeight,mEyePoint.z);
		}
	});
	mParams->addParam("Separation Weight", &mSeparateWeight).group("INTERNAL FORCE").updateFn([this] {
		for (auto &p : mParticles) {
			p.update(mParticles, desiredseparation, mSeparateWeight, mEyePoint.z);
		}
	});

	// GROUP -- CAMERA SETTINGS
	mParams->addParam("Eye Point X", &mEyePoint.x).group("CAMERA SETTINGS").step(1.0f);
	mParams->addParam("Eye Point Y", &mEyePoint.y).group("CAMERA SETTINGS").step(1.0f);
	mParams->addParam("Eye Point Z", &mEyePoint.z).group("CAMERA SETTINGS").step(1.0f).updateFn([this] {
		mInteractiveField.applyForces(mParticles, mouseloc, mAttractNum, mInteractiveWeight, mEyePoint.z);
			for (auto &p : mParticles) {
				p.update(mParticles, desiredseparation, mSeparateWeight, mEyePoint.z);
			}
	});
	mParams->addParam("Look At X", &mLookAt.x).group("CAMERA SETTINGS").step(0.5f);
	mParams->addParam("Look At Y", &mLookAt.y).group("CAMERA SETTINGS").step(0.5f);
	mParams->addParam("Look At Z", &mLookAt.z).group("CAMERA SETTINGS").step(0.5f);
	mParams->addParam("FOV", &mFov).group("CAMERA SETTINGS").min(1.0f).max(179.0f);
	mParams->addParam("Near Plane", &mNearPlane).group("CAMERA SETTINGS").step(0.2f).min(0.1f);
	mParams->addParam("Far Plane", &mFarPlane).group("CAMERA SETTINGS").step(0.2f).min(0.1f);
	mParams->addButton("Reset Camera Defaults", bind(&Particle_perlinApp::setDefaultCameraValues, this));

	// GROUP -- BG POSITION
	mParams->addParam("BG Z Position", &mBgZPos).step(0.5f).min(0.5f).max(200.0f);
	mParams->addParam("BG scale X", &mBgScalor.x).step(0.1f).min(1.0f).max(10.0f);
	mParams->addParam("BG scale Y", &mBgScalor.y).step(0.1f).min(1.0f).max(10.0f);

	////////////////////

	mouseloc = vec3(0.0f);

	mParticles.assign(PARTICLE_AMT, Particle(vec3(getWindowCenter(), 100.0f), mRadius));
	for (int i = 0; i < PARTICLE_AMT; ++i) {
		auto &p = mParticles.at(i);
		p.mLoc = vec3(Rand::randFloat()*getWindowWidth(), (Rand::randFloat() - 0.5f)*(getWindowHeight() - 2.0*mCuttingHeight) / 2.0f + getWindowHeight() / 2.0f, 0.0f);
		p.mVel = 0.1f *vec3((Rand::randFloat() + 3.0), Rand::randFloat(), 0.0f);
	}

	mInteractiveField.setup(PARTICLE_AMT);
}

void Particle_perlinApp::mouseMove(MouseEvent event)
{
	mouseloc = vec3(event.getPos(), 0.0f);
}

void Particle_perlinApp::mouseDown(MouseEvent event)
{	
	mMouseDownLoc = vec3(event.getPos(), 0.0f);
	mMouseDownMode = true;
	if (mOnAddParticlesMode) {
		addParticleSystem(mMouseDownLoc, mAttractNum);
	}
	
}

void Particle_perlinApp::mouseDrag(MouseEvent event) {
	mouseMove(event);
}

void Particle_perlinApp::mouseUp(MouseEvent event) {
	mMouseDownMode = false;
}

void Particle_perlinApp::switchMode() {
	if (mOnAddParticlesMode) {
		mOnAddParticlesMode = false;
		mParams->setOptions("text", "label='Attract and Enlarge Mode'");
	}
	else {
		mOnAddParticlesMode = true;
		mParams->setOptions("text", "label='Add Particles Mode'");
	}
}

void Particle_perlinApp::addParticleSystem(vec3 triggerLoc, int amt) {
	ParticleSystem newSystem(triggerLoc, amt, mRadius);
	newSystem.setup();
	mParticleSystems.push_back(newSystem);
}


void Particle_perlinApp::update()
{
	// CAMERA UPDATES
	//mObjectCam.setEyePoint(mEyePoint);
	mObjectCam.setPerspective(mFov, getWindowAspectRatio(), mNearPlane, mFarPlane);
	mObjectCam.lookAt(mEyePoint,mLookAt);

	mFlowField.applyForces(mParticles, mNoiseRange);
	if (!mOnAddParticlesMode) {
		mInteractiveField.applyForces(mParticles, mouseloc,mAttractNum, mInteractiveWeight, mEyePoint.z);
	}

	for (auto &p : mParticles) {
		p.update(mParticles,desiredseparation,mSeparateWeight, mEyePoint.z);
	}

		// If there is top layer particle systems
		if (mParticleSystems.size() > 0) {
			if (mOnAddParticlesMode) {
				int index = mParticleSystems.size();
				if (mMouseDownMode) {
					// apply interactive attract force to the new add on particles
					mInteractiveField.applyForces(mParticleSystems.at(index - 1).mParticles, mouseloc, mAttractNum, 1.5f, mEyePoint.z);
				}
				else {
					// release the new add-on particles from active mode
					for (auto &p : mParticleSystems.at(index - 1).mParticles) {
						p.activeMode = false;
					}
				}
			}

			// Apply all flow field forces(perlin and central) to all the top layer particles
			for (int i = 0; i < mParticleSystems.size(); i++) {
				mFlowField.applyForces(mParticleSystems.at(i).mParticles, mNoiseRange);
				for (auto &p : mParticleSystems.at(i).mParticles) {
					p.update(mParticles, desiredseparation, mSeparateWeight, mEyePoint.z);
				}
			}
	}
}


void Particle_perlinApp::draw()
{
	gl::clear( Color( 0.0, 0.0, 0.0 ) ); 
	gl::enableDepthRead();
	gl::enableDepthWrite();
	
	{
		// SCOPE TO FLIP THE Y COORDINATION FOR THE ENTIRE SPACE
		gl::ScopedMatrices scopedMatrices;
		gl::ScopedProjectionMatrix scpMatrices;
		gl::scale(vec3(1, -1, 1));
		gl::translate(vec3(0.0, -getWindowHeight(), 0.0));

		{
			gl::ScopedMatrices bgScpMatrices;
			gl::ScopedProjectionMatrix bgScpProjectionMatrices;
			gl::ScopedModelMatrix bgScpModelMatrix;
			gl::scale(vec3(mBgScalor, 1.0f));
			gl::translate(vec3(-(mBgScalor.x - 1.0f) * IMG_WIDTH / 4.0f, -(mBgScalor.y - 1.0f) * IMG_HEIGHT / 4.0f, -mBgZPos));
			gl::draw(mBgTexture);
		}

		for (auto &p : mParticles) {
			p.render();
			p.mAcc *= 0.0f;
		}

		if (mParticleSystems.size() > 0) {
			for (int i = 0; i < mParticleSystems.size(); i++) {
				for (auto &p : mParticleSystems.at(i).mParticles) {
					p.render();
					p.mAcc *= 0.0f;
				}
			}
		}
	}

	gl::setMatrices(mObjectCam);
	mParams->draw();
}

CINDER_APP( Particle_perlinApp, RendererGl )
