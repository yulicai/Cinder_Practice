#pragma once

#include "BaseFilter.h"

namespace bluecadet {
    
    namespace kinect2 {
		
		typedef std::shared_ptr<class Flip> FlipRef;
        
		class Flip : public BaseFilter {
		public:
			void apply(cv::Mat &depthMat) override;
		};
		
    }
    
}

