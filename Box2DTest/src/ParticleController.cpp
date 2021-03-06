#pragma once
#include "cinder/app/App.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/Vector.h"
#include "ParticleController.h"
#include "Globals.h"
#include "Conversions.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using std::list;

namespace particles {

ParticleController::ParticleController()
{
}

void ParticleController::setup(b2World &w)
{
	world = &w;
}

void ParticleController::update()
{
	for( list<Particle>::iterator p = particles.begin(); p != particles.end(); p++) {
		p->update();
	}
}

void ParticleController::draw()
{
	for( list<Particle>::iterator p = particles.begin(); p != particles.end(); p++ ){
		p->draw();
	}
}

void ParticleController::addParticle(const vec2 &mousePos, gl::TextureRef texture)
{
	Particle p = Particle();

	// create a dynamic body
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(Conversions::toPhysics(mousePos.x), Conversions::toPhysics(mousePos.y));

	// instead of just creating body... 
	// b2Body* body = world->CreateBody(&bodyDef);
	// do the following to create it with a circular reference to it's corresponding particle
	bodyDef.userData = &p;
	p.body = world->CreateBody(&bodyDef);

	/* Testing to get image size*/
	//gl::TextureRef mTexture = gl::Texture::create(loadImage(loadAsset("modelTBig.png")), gl::Texture::Format());
	gl::TextureRef mTexture = texture; 
	float imageX = mTexture->getWidth()/2.0f;
	float imageY = mTexture->getHeight()/2.0f;

	b2PolygonShape dynamicBox;
	float boxSizeX = Rand::randFloat(global::BOX_X_MIN, global::BOX_X_MAX);
	float boxSizeY = Rand::randFloat(global::BOX_Y_MIN, global::BOX_Y_MAX);

	//dynamicBox.SetAsBox(Conversions::toPhysics(boxSizeX), Conversions::toPhysics(boxSizeY));
	dynamicBox.SetAsBox(Conversions::toPhysics(imageX), Conversions::toPhysics(imageY));

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &dynamicBox;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.2f;
	fixtureDef.restitution = 0.3f; // bounce

	p.body->CreateFixture(&fixtureDef);

	// rest of initialization particle can do for itself
	p.setup(vec2(imageX, imageY),texture);
	particles.push_back(p);

}

void ParticleController::removeAll()
{
	// not the right way, but an alternative
	/*
	b2Body* body = world->GetBodyList();
	while( body != NULL )
	{

		body = body->GetNext();
	}
	*/

	// right way
	for( list<Particle>::iterator p = particles.begin(); p != particles.end(); p++) {
		world->DestroyBody(p->body);
	}

	particles.clear();

	if (global::COLOR_SCHEME == 0)
		global::COLOR_SCHEME++;
	else
		global::COLOR_SCHEME--;
}

}