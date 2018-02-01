#include "cinder/app/App.h"
#include "Particle.h"
#include "Globals.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "Conversions.h"
#include <Box2D/Box2d.h>

using namespace ci;
using namespace ci::app;
using namespace std;

namespace particles {
	
	Particle::Particle() {
	}
	
	Particle::~Particle() {
		//body->GetWorld()->DestroyBody( body );  // this ruins everything
	}
	
	
	void Particle::setup(vec2 boxSize, gl::TextureRef texture) {

		size = boxSize;
		mTexture = texture;

		if (global::COLOR_SCHEME == 0)
			color = ci::ColorA(1, ci::Rand::randFloat(0,.8), 0, 1);  // red to yellow
		else
			color = ci::ColorA(ci::Rand::randFloat(0,.8), 0, 1, 1);  // blue to violet
	}
	
	void Particle::update() {

	}

	void Particle::draw() {

		//gl::color(color);

		vec2 pos = Conversions::toScreen( body->GetPosition() );
		float t = Conversions::radiansToDegrees( body->GetAngle() );
	
		gl::pushMatrices();
		gl::translate( pos.x, pos.y);
		gl::rotate( t );
	

		//Rectf rect(-size.x, -size.y, size.x, size.y);
		//gl::drawSolidRect(rect);

		//gl::TextureRef mTexture = gl::Texture::create(loadImage(loadAsset("modelTBig.png")), gl::Texture::Format());
		gl::draw(mTexture,vec2(-size.x,-size.y));

		gl::popMatrices();
	}

}

