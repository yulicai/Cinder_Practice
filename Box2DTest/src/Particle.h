#pragma once
#include "cinder/gl/gl.h"
#include "cinder/app/App.h"
#include "cinder/Color.h"
#include <Box2D/Box2d.h>

using namespace ci;
using namespace ci::app;

namespace particles {
	
    class Particle {
	public:
		Particle();
		~Particle();
		
		// pass in a pointer to the particle
		void setup(ci::vec2 boxSize,gl::TextureRef texture);
		void update();
		void draw();
		
	//private:
		// store a pointer to the particle in the physics world from the main app
		b2Body*				body;
		ci::Color			color;
		ci::vec2			size;
		gl::TextureRef	    mTexture;
	};
	
}

