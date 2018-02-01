#pragma once

#include "BaseFilter.h"

namespace bluecadet {
    
    namespace kinect2 {
		
		typedef std::shared_ptr<class Resize> ResizeRef;
        
		class Resize : public BaseFilter {
		public:
			Resize(int width, int height);
			void apply(cv::Mat &depthMat) override;
			
		private:
			cv::Size mSize;
		};
		
    }
    
}

