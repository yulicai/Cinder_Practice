#pragma once
#include "cinder\app\App.h"
#include "Particle.h"
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

class InteractiveField {
public:
	InteractiveField(){};

	void setup(int particleAmt);
	void applyForces(vector<Particle> & particles, vec3 triggerLoc, float attractNum, float interactiveWeight,float eyePointZ);
	vec3 interactiveInterfare(vec3 triggerLoc, vec3 pos, vec3 vel);
	vec3 pullOutInterfare(const float boundingDis, vec3 triggerLoc, vec3 pos, vec3 vel);
	vec3 zBoundingRepelForce(const float boundingDis, const vec3 pos, const vec3 vel);

	vector<vec2> sortedParticleDis;
	vector<Particle> activatedParticles;
};