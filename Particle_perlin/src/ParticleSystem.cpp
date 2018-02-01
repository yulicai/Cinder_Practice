#include "cinder/app/App.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include <vector>
#include "ParticleSystem.h"
#include "Particle.h"

using namespace ci;
using namespace ci::app;
using namespace std;


void ParticleSystem::setup() {
	mParticles.assign(mAmt, Particle(mOriLoc,mRadius));

	for (int i = 0; i < mAmt; ++i) {
		auto &p = mParticles.at(i);
		float x = i*(Rand::randFloat() - 0.5f)*4.0f*mRadius;
		float y = i*(Rand::randFloat() - 0.5f)*4.0f*mRadius;
		p.mLoc = vec3(x, y, 0.0f) + mOriLoc;

		p.mVel = 0.1f *vec3((Rand::randFloat() + 3.0), Rand::randFloat(), 0.0f);
		p.activeMode = true;
	}
}