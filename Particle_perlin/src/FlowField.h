#pragma once
#include "cinder/app/App.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "Particle.h"
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

class Flowfield {
public:
	

	Flowfield::Flowfield() :
	mCentralWeight(0.005f),
	mPerlinWeight(0.01f)
	{};

	void applyForces(  vector<Particle> & particles, float perlinRange);
	vec3 centralInterfare(const vec3 pos, const vec3 vel);
	vec3 perlinInterfare(const vec3 pos, float noiseRange);
	float mCentralWeight;
	float mPerlinWeight;

private:
	Perlin mPerlin;
	vector <vec3> field;
};