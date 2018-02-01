#pragma once

#include "BaseFilter.h"

namespace bluecadet {
    
    namespace kinect2 {
		
		typedef std::shared_ptr<class DepthToImage> DepthToImageRef;
        
		class DepthToImage : public BaseFilter {
		public:
			//! Convert from meters to brightness (0.0f-1.0f).
			//! maxDinstance in meters.
			//! Must be placed after RemoveBackground filter.
			DepthToImage(float maxDistance = 8.0f);
			
			void apply(cv::Mat &depthMat) override;
			
		private:
			float mMaxDistance; // in millimeter
		};
		
    }
    
}

