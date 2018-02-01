#include "CinderLogger.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::kinect2;

bluecadet::kinect2::CinderLogger::CinderLogger() {
}

void bluecadet::kinect2::CinderLogger::log(Level level, const std::string & message) {
	
	switch (level) {
	case None:
		break;

	case Error:
		CI_LOG_E(message);
		break;

	case Warning:
		CI_LOG_W(message);
		break;

	case Info:
		CI_LOG_I(message);
		break;

	case Debug:
		CI_LOG_D(message);
		break;
	}

}
