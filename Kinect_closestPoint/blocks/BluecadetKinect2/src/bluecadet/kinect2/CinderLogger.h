#pragma once

#include "cinder/app/App.h"
#include "cinder/Log.h"

#include "libfreenect2/logger.h"

namespace bluecadet {

namespace kinect2 {

class CinderLogger : public libfreenect2::Logger {
public:
	CinderLogger();
	virtual void log(Level level, const std::string &message) override;
};

}

}