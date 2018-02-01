#include "Resize.h"

using namespace ci;
using namespace std;
using namespace bluecadet::kinect2;

Resize::Resize(int width, int height) : mSize(max(width, 0), max(height, 0)) {
	
}

void Resize::apply(cv::Mat &depthMat) {
	
	cv::Mat destinationMat;
	cv::resize(depthMat, destinationMat, mSize);
	depthMat = destinationMat;
	
}
