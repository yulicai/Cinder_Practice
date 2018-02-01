
#include "cinder\app\App.h"
#include "InteractiveField.h"
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

void InteractiveField::setup(int particleAmt) {
	// Array to store the closest certain particles based on the sorting result
	sortedParticleDis.assign(particleAmt, vec2(0.0));
}

void InteractiveField::applyForces(vector<Particle> & particles, vec3 triggerLoc, float attractNum, float interactiveWeight,float eyePointZ) {
	/*
	for (auto &p : particles) {
		vec3 pos = p.mLoc;
		vec3 vel = p.mVel;
		float dis = length(pos - triggerLoc);
		if (dis < attractRange) {
			vec3 interactiveForce = interactiveWeight*interactiveInterfare(triggerLoc, pos, vel);
			p.applyForce(interactiveForce);
		}
	}
	*/

	/* GET THE ASCENDING SORTED VALUES OF DISTANCE TO TRIGGER LOCATION */
	int current;
	for (int i = 0; i < particles.size(); i++) {
		auto &p = particles.at(i);
		p.activeMode = false;
		vec3 pos = particles.at(i).mLoc;
		vec3 vel = particles.at(i).mVel;
		//float nextDis = length(pos - triggerLoc);
		float nextDis = length(vec2(pos.x,pos.y) - vec2(triggerLoc.x,triggerLoc.y));
		current = 0;
		while (current < i && nextDis >= sortedParticleDis.at(current).y) current++;
		for (int j = i; j > current; j--) {
			sortedParticleDis.at(j) = sortedParticleDis.at(j - 1);
		}
		sortedParticleDis.at(current).x = i;
		sortedParticleDis.at(current).y = nextDis;
	}
	
	activatedParticles.clear();
	for (int i = 0; i < attractNum; i++) {
		int oriIndex = (int)sortedParticleDis.at(i).x;
		auto &p = particles.at(oriIndex);
		activatedParticles.push_back(p);
		p.activeMode = true;
		vec3 pos = p.mLoc;
		vec3 vel = p.mVel;
		vec3 interactiveForce = interactiveWeight*interactiveInterfare(triggerLoc, pos, vel);
		float disToBound = (eyePointZ*0.9f - pos.z)/ eyePointZ*0.9f;
		vec3 pullOutForce = disToBound* pullOutInterfare(eyePointZ*0.9f, triggerLoc, pos, vel); // the weight of this should be tied to the weight for z bounding repel force in flow field
		vec3 zRepelForce = (1.0f-disToBound)*zBoundingRepelForce(eyePointZ*0.9f, pos, vel); // the weight for bounding repel force and mouse interactive force should be tied together and negative to one another
		vec3 repelBorderForce = zBoundingRepelForce(eyePointZ, pos, vel); //the first param should be eyepointZ
		//console() << "[pull out force]-" << pullOutForce << "   [repel force]-" << zRepelForce<<endl;
		vec3 groupSeparationForce = 3.5f* p.separate(activatedParticles, 150.0f);
		p.applyForce(interactiveForce);
		if (p.activeMode) {
			p.applyForce(pullOutForce);
			//p.applyForce(zRepelForce);
			
		}
		else { 
			p.applyForce(2.0f*zRepelForce);
			p.applyForce(repelBorderForce);
		}
		p.applyForce(groupSeparationForce); 

	}
	
}


// FORCE APPROACH
// Main interactive interfare affect the x-y space location
vec3 InteractiveField::interactiveInterfare(vec3 triggerLoc, vec3 pos, vec3 vel) {
	vec3 desired = normalize(triggerLoc - pos);
	float disWeight = length(triggerLoc - pos) / 200.0f;

	desired *= disWeight;
	vec3 interactiveSteer = desired - vel;

	return interactiveSteer;
}

// Side force that applies to the activated particles to bring the z position front 
vec3 InteractiveField::pullOutInterfare(const float boundingDis, vec3 triggerLoc, vec3 pos, vec3 vel) {
	vec3 pullLoc = vec3(triggerLoc.x, triggerLoc.y, boundingDis);
	vec3 desired = normalize(pullLoc - pos);
	vec3 pullOutSteer = desired - vel;
	return pullOutSteer;
}


// Z space bounding repelling force, the close to the bounding boundary, the slower the speed
vec3 InteractiveField::zBoundingRepelForce(const float boundingDis, const vec3 pos, const vec3 vel) {
	vec3 target = vec3(pos.x, pos.y, boundingDis);
	vec3 desire = normalize(target - pos);
	vec3 repelSteer = vel - desire; // the opposite direction, negative force
	return repelSteer;
}
