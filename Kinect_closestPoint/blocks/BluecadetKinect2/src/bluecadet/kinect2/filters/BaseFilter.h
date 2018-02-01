#pragma once

#include "CinderOpenCV.h"

namespace bluecadet {
    
    namespace kinect2 {
		
		typedef std::shared_ptr<class BaseFilter> BaseFilterRef;
        
		class BaseFilter {
		public:
			virtual void apply(cv::Mat &depthMat) = 0;
		};
		
    }
    
}
