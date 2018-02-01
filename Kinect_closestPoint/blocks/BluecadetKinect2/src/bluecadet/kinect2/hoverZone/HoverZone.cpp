#include "HoverZone.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::kinect2;

void HoverZone::enable() {
	enabled = true;
}

void HoverZone::disable() {
	enabled = false;
	intersectionCount = -1;
	detectionFrameCount = 0;
}

Rectf HoverZone::getZone() const {
	return zone;
}

int HoverZone::getIntersectionCount() {
	return intersectionCount;
}
