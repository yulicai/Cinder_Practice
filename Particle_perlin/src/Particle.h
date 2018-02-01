#pragma once

#include "cinder/app/App.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

class Particle {
public:
	vec3 mLoc;
	vec3 mVel;
	vec3 mAcc;
	float mRadius;
	float mMaxRadius;
	float mMaxForce;
	float mMaxSpeed;
	int mCuttingHeight;
	Perlin mPerlin;
	bool activeMode;
	float radiusCounter;
	float mRadiusScalor;
	

	Particle(const vec3 &loc, const float &r) :
		mLoc(loc),
		mVel(0.0f,0.0f, 0.0f),
		mAcc(vec3(0.0f)),
		mRadius(r),
		mMaxRadius(12.0f),
		mMaxForce(0.05f),
		mMaxSpeed(1.0f),
		mCuttingHeight(100),
		mPerlin(Perlin()),
		activeMode(false),
		radiusCounter(1.0f),
		mRadiusScalor(1.0f)
	{};

	void applyForce(vec3 force);
	void borderInterfare(float damping, int mCuttingHeight, float eyePointZ);
	vec3 separate(const vector<Particle> & particles, float desiredseparation);
	void mouseInterfare(vec3 mouseloc);
	void centralRadiusInterfare(vec3 mLoc, float radiusScalor);
	float map(float val, float inMin, float inMax, float outMin, float outMax,float mapFactor);
	float centralMap(float val, float maxRadius);
	void update(const vector<Particle> &particles, float desiredseparation, float separateWeight,float eyePointZ);
	void render();
	

};
