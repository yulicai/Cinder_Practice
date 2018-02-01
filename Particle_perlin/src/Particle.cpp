#include "Particle.h"
#include "cinder\app\App.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/Perlin.h"
#include "cinder/CinderMath.h"
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

void Particle::applyForce(vec3 force) {
	mAcc += force;
}


vec3 Particle::separate(const vector<Particle> & particles, float desiredseparation) {
	vec3 steer = vec3(0.0f);
	vec3 separateSteer = steer;
	int count = 0;
	for (const auto &b : particles) {
		float d = length(mLoc - b.mLoc);
		if ((d > 0) && d < desiredseparation) {
			vec3 diff = normalize(mLoc - b.mLoc);
			diff /= d; // weight by distance
			steer += diff;
			count++;
		}
	}
	// Average -- devide by how many 
	if (count > 0) {
		steer /= count;
	}
	if (length(steer) > 0) {
		separateSteer = normalize(steer);
		separateSteer *= mMaxSpeed;
		separateSteer -= mVel;
		steer = separateSteer;
		auto l = length(separateSteer);
		if (l > mMaxForce) separateSteer = mMaxForce * normalize(steer);
	}
	return separateSteer;
}



void Particle::borderInterfare(float damping,int cuttingHeight, float eyePointZ) {
	int width = getWindowWidth();
	int height = getWindowHeight()-cuttingHeight;
	
	//if ((mLoc.x < -mRadius) || (mLoc.x > width + mRadius)) mVel.x *= -damping;
	//if ((mLoc.y < -mRadius) || (mLoc.y > height + mRadius)) mVel.y *= -damping;	

	if (mLoc.x > width + mRadius) {
		mVel.x *= 0.9f;
		mLoc.x = -mRadius;		
	}
	if (mLoc.x < -mRadius) mLoc.x = width;
	if ((mLoc.y < cuttingHeight-mRadius) || (mLoc.y > height - mRadius)) mVel.y *= -damping;

	// Add z space border, should be tied to eyepoint z
	if(!activeMode){
	if (mLoc.z >= eyePointZ*0.7f || mLoc.z <= 0) mVel.z *= -damping;
	}
	/*
	if (mLoc.z > 50.0f) {
		mVel.z *= 0.9f;
		mLoc.z = 0.5f;
	}
	if (mLoc.z < 0.0f) {
		mLoc.z = 0.9f;
	}
	*/

}



/* RADIUS RELATED FUNCTIONS*/
/////////////////////////////
void Particle::mouseInterfare(vec3 mouseloc) {
	// change the radius of each circle
	float maxDis = 2.0f*length(getWindowCenter() - vec2(0.0f));
	float dis = length(mouseloc - mLoc);
	float mappedDis = map(dis, 0.0f, maxDis, 20.0f, 2.0f, 0.5);
	mRadius = mappedDis;
}

void Particle::centralRadiusInterfare(vec3 mLoc, float radiusScalor) {
	// the closer it is to the central line in y direction, the bigger it is
	// it is happening all the time, a linear distribution
	float maxDis = getWindowHeight() / 2.0f;
	float disToCenterLine = abs(mLoc.y - (getWindowHeight() / 2.0f));
	float mappedDis = 1.5f*centralMap(disToCenterLine, mMaxRadius) ;
	if (mappedDis < 0.0f) mappedDis = 2.0f;
	mRadius = radiusScalor* mappedDis;
}

float Particle::map(float val, float inMin, float inMax, float outMin, float outMax, float mapFactor) {
	return outMin + (outMax - outMin) * pow(((val - inMin)/  (inMax - inMin)), mapFactor);
}


float Particle::centralMap(float val,float mMaxRadius) {
	// returns something between 10.0 to 0.0
	float out = (-pow(val, 0.5) + mMaxRadius);
	return out;
}


void Particle::update(const vector<Particle> &particles, float desiredseparation, float separateWeight,float eyePointZ) {
	// forces 
	applyForce(separateWeight*separate(particles, desiredseparation));
	borderInterfare(0.70f, mCuttingHeight,eyePointZ);

	if (activeMode) {
		float largestScalor = 1.5f;
		if(radiusCounter<= largestScalor)  radiusCounter += 0.02f;
		mRadiusScalor = sqrt(pow(radiusCounter, 3.0f) + pow(radiusCounter, 2.0f*radiusCounter));
	}
	else {
		radiusCounter = 1.0f;
		mRadiusScalor = 1.0f;
	}

	// radius / size
	centralRadiusInterfare(mLoc, mRadiusScalor);
	mVel += mAcc;
	mLoc += mVel;

	// TESTING Z INDEX
	//mLoc += vec3(0.0f, 0.0f, (Rand::randFloat()-0.5f)*50.0f);
}

void Particle::render() {
	 float r = mRadius / 2.0f;
	 gl::ScopedModelMatrix scpModelMatrix;
	 gl::ScopedColor scpColor;
	 gl::translate(mLoc);
	 float heigthOff = abs(mLoc.y - getWindowHeight() / 2.0);
	 gl::enableAlphaBlending();
	 ColorA color = Color(CM_HSV, lmap<float>(heigthOff, 0.0f, getWindowHeight() / 2.0,0.18f, 0.07f), 1.0f, lmap<float>(heigthOff, 0.0f, getWindowHeight() / 2.0, 1.00f, 0.87f));
	 //ColorA color = Color(CM_HSV, lmap<float>(mRadius, 0.0f, mMaxRadius, 0.06f, 0.15f), 1.0f, lmap<float>(mRadius, 0.0f, mMaxRadius, 0.87f, 1.00f));

	 
	 if (activeMode) {
		 color.a = 0.7f;
		 gl::color(color);
		 gl::drawSolidRect(Rectf(-r, -r, r, r));
		 //gl::drawSolidCircle(vec2(0), r);
	 }
	 else {
		 color.a = 1.0f;
		 gl::color(color);
		 gl::drawStrokedRect(Rectf(-r, -r, r, r));
		 //gl::drawStrokedCircle(vec2(0), r);
	 }
		

}


