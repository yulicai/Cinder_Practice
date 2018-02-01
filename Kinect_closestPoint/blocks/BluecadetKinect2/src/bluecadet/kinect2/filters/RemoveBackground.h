#pragma once

#include  <atomic>
#include "BaseFilter.h"

namespace bluecadet {
    
    namespace kinect2 {
		
		typedef std::shared_ptr<class RemoveBackground> RemoveBackgroundRef;
        
		class RemoveBackground : public BaseFilter {
		public:
			//! Remove all pixels within +-distanceThreshold distance from the background.
			//! distanceThreshold and maxDepth in meters.
			//! Reduce edge noise will also reduce the maximum detection range to 0.7x maxDepth.
			//! Must be place before DepthToImage filter.
			RemoveBackground(float minDistanceFromBackground = 0.15f, bool reduceNoise = true, bool reduceEdgeNoise = true, float maxDepth = 8.0f);
			void setMinDistanceFromBackground(float minDistanceFromBackground);
			float getMinDistanceFromBackground() const;
			
			void updateBackground();
			void apply(cv::Mat &depthMat) override;
			
		private:
			cv::Mat mBackgroundMat;
			std::atomic_bool mNeedtoUpdateBackground;
			std::atomic<float> mMinDistanceFromBackground;
			bool mReduceNoise;
			bool mReduceEdgeNoise;
			float mMaxDepth;

		};
		
    }
    
}

