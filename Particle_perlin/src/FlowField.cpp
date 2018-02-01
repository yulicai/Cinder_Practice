#include "FlowField.h"
#include <vector>
#include "cinder/Perlin.h"

using namespace ci;
using namespace ci::app;
using namespace std;



void Flowfield::applyForces( vector<Particle> & particles, float perlinRange) {
	for (auto &p : particles) {
		vec3 pos = p.mLoc;
		vec3 vel = p.mVel;

		vec3 centralForce = mCentralWeight*centralInterfare(pos, vel)/100.0f;	
		vec3 perlinForce = mPerlinWeight * perlinInterfare(pos, perlinRange);
	
		//if (!p.activeMode) {
			p.applyForce(perlinForce);
		//}
			p.applyForce(centralForce);

	}
}


vec3 Flowfield::centralInterfare(const vec3 pos, const vec3 vel) {
	vec3 target = vec3(pos.x, getWindowHeight() / 2.0f, 0.0f);
	vec3 off = target - pos;
	//vec3 desire =target - pos;
	vec3 centralSteer = off - vel;
	return centralSteer;
}


vec3 Flowfield::perlinInterfare(const vec3 pos, float noiseRange) {
	float nX = pos.x * 0.005f;
	float nY = pos.y * 0.005f;
	float nZ = getElapsedSeconds() * 0.1f;
	vec3 v = vec3(nX, nY, nZ);
	float noise = mPerlin.fBm(v);
	float angle = noise *noiseRange;
	vec3 perlinNoiseSteer = vec3(cos(angle), sin(angle), 0.0f);
	return perlinNoiseSteer;
}

