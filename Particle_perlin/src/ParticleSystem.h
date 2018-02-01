#pragma once
#include "cinder/app/App.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "Particle.h"
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

class ParticleSystem {
public:
	void setup();

	ParticleSystem(vec3 loc, int amt,float r) :
		mOriLoc(loc),
		mAmt(amt),
		mRadius(r)
	{};


	vec3	mOriLoc;
	int		mAmt;
	float	mRadius;
	vector<Particle> mParticles;
	
};

