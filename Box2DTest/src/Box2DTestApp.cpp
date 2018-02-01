#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include <Box2D/Box2d.h>
#include "Conversions.h"
#include "ParticleController.h"
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace particles;

b2Vec2 gravity(0.0f, 10.0f);
b2World world(gravity);
ParticleController particleController;

const int TEXTURE_NUM = 15;

class Box2DTestApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp(MouseEvent event) override;
	void mouseMove(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void keyDown(KeyEvent event);
	void update() override;
	void draw() override;

	private:
		float width;
		float height;
		bool mousePressed;
		vec2 mousePos;
		vec2 mouseVel;
		vector<gl::TextureRef> mTextures;

};

void Box2DTestApp::setup()
{
	setWindowSize(1920, 1080);
	// prepare all the textures
	for (int i = 0; i < TEXTURE_NUM; i++) {
		gl::TextureRef tempTex = gl::Texture::create(loadImage(loadAsset(to_string(i) + ".png")));
		mTextures.push_back(tempTex);
	}

	mousePressed = false;
	mousePos = vec2(0.0f, 0.0f);

	// first define a ground box (no mass)
	// 1. define a body
	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(Conversions::toPhysics(getWindowWidth() / 2), Conversions::toPhysics(getWindowHeight())); // pos of ground

	// 2. use world to create body
	b2Body* groundBody = world.CreateBody(&groundBodyDef);

	// 3. define fixture
	b2PolygonShape groundBox;
	groundBox.SetAsBox(Conversions::toPhysics(app::getWindowWidth() / 2), Conversions::toPhysics(1.0f)); // size the ground

																										 // 4. create fixture on body
	groundBody->CreateFixture(&groundBox, 0.0f);

	// pass world to ParticleController
	particleController.setup(world);
}

void Box2DTestApp::mouseDown( MouseEvent event )
{
	mousePressed = true;
}

void Box2DTestApp::mouseUp(MouseEvent event)
{
	mousePressed = false;
}

void Box2DTestApp::mouseMove(MouseEvent event)
{
	mouseVel = (vec2(event.getPos()) - mousePos);
	mousePos = event.getPos();
}

void Box2DTestApp::mouseDrag(MouseEvent event)
{
	mouseMove(event);
}

void Box2DTestApp::keyDown(KeyEvent event)
{
	switch (event.getChar()) {
	case ' ': particleController.removeAll(); break;
	}
}
void Box2DTestApp::update()
{
	if (mousePressed) {
		int tempIndex = (int)Rand::randFloat(0.0f, (float)TEXTURE_NUM);
		console() << "index: " << endl;
		particleController.addParticle(mousePos,mTextures.at(tempIndex));
		//particleController.addParticle(mousePos, mTextures.at(2));
	}
	particleController.update();

	// step physics world
	float32 timeStep = 1.0f / 60.0f;
	int32 velocityIterations = 6;
	int32 positionIterations = 2;
	world.Step(timeStep, velocityIterations, positionIterations);
}

void Box2DTestApp::draw()
{
	gl::clear(Color(1.0f, 1.0f, 1.0f));
	gl::enableAlphaBlending();

	particleController.draw();
}

CINDER_APP( Box2DTestApp, RendererGl )
