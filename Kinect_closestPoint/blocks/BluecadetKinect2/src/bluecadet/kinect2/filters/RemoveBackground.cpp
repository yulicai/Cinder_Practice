#include "RemoveBackground.h"

using namespace ci;
using namespace std;
using namespace bluecadet::kinect2;

RemoveBackground::RemoveBackground(float minDistanceFromBackground, bool reduceNoise, bool reduceEdgeNoise, float maxDepth) : mMinDistanceFromBackground(minDistanceFromBackground * 1000), mReduceNoise(reduceNoise), mMaxDepth(maxDepth) {
	
	mNeedtoUpdateBackground = false;

}

void RemoveBackground::setMinDistanceFromBackground(float minDistanceFromBackground) {
	
	mMinDistanceFromBackground = minDistanceFromBackground * 1000;
	
}

float RemoveBackground::getMinDistanceFromBackground() const {
	
	return mMinDistanceFromBackground / 1000;
	
}

void RemoveBackground::updateBackground() {
	
	mNeedtoUpdateBackground = true;
	
}

void RemoveBackground::apply(cv::Mat &depthMat) {
	
	if (mNeedtoUpdateBackground) {
		
		mBackgroundMat = depthMat.clone();
		mNeedtoUpdateBackground = false;
		
	}
	
	if (!mBackgroundMat.empty() && mBackgroundMat.size == depthMat.size) {
		
		cv::Mat differenceMat;
		cv::absdiff(depthMat, mBackgroundMat, differenceMat);
		cv::Mat maskMat;
		cv::threshold(differenceMat, maskMat, mMinDistanceFromBackground, 1.0f, cv::THRESH_BINARY);
		
		if (mReduceNoise) {
			
			cv::Mat maskMatDenoised;
			cv::morphologyEx(maskMat, maskMatDenoised, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(11, 11)));
			maskMat = maskMatDenoised;
			
		}

		cv::Mat destinationMat;
		cv::multiply(depthMat, maskMat, destinationMat);

		if (mReduceEdgeNoise) {

			cv::threshold(destinationMat, depthMat, mMaxDepth * 1000.0f * 0.7f, 1.0f, cv::THRESH_TOZERO_INV);

		} else {

			depthMat = destinationMat;

		}

	}
	
}
