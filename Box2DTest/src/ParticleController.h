#pragma once
#include "Particle.h"
#include "cinder/app/App.h"
#include <Box2D/Box2d.h>
#include <list>


namespace particles {
	
	class ParticleController {
	public:
		ParticleController();
		// pass the physics world from the main app by reference
		void setup(b2World &w);
		void update();
		void draw();
		void addParticle(const ci::vec2 &mousePos, gl::TextureRef texture);
		void removeAll();
	
	private:
		// store a pointer to the physics world fro the main app
		b2World *world;
		std::list<Particle>	particles;
};

}