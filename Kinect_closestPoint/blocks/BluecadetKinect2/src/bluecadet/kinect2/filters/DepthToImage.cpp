#include "DepthToImage.h"

using namespace ci;
using namespace std;
using namespace bluecadet::kinect2;



DepthToImage::DepthToImage(float maxDistance) : mMaxDistance(maxDistance * 1000) {
	
}

void DepthToImage::apply(cv::Mat &depthMat) {
	
	depthMat /= mMaxDistance;
	
}
