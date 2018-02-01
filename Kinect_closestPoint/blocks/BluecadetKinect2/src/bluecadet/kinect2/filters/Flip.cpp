#include "Flip.h"

using namespace ci;
using namespace std;
using namespace bluecadet::kinect2;

void Flip::apply(cv::Mat &depthMat) {
	
	cv::Mat destinationMat;
	cv::flip(depthMat, destinationMat, 1);
	depthMat = destinationMat;
	
}
