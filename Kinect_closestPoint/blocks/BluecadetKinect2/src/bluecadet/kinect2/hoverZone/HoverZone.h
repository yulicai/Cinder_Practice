#pragma once

#include "cinder/app/App.h"

namespace bluecadet {
namespace kinect2 {

typedef std::shared_ptr<struct HoverZone> HoverZoneRef;

struct HoverZone {
	friend class HoverZoneManager;

public:
	ci::Rectf getZone() const;
	int getIntersectionCount();

private:
	void enable();
	void disable();

	ci::Rectf zone;
	float averageDepth;
	bool hasAverageDepth = false;
	bool enabled = false;
	int intersectionCount = -1;
	unsigned int detectionFrameCount = 0;

	std::function<void()> trigger;
	std::function<void()> cancel;
};

}
}