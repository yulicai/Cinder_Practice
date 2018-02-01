#pragma once

#include <thread>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/Timer.h"

#include "libfreenect2/libfreenect2.hpp"
#include "libfreenect2/frame_listener_impl.h"
#include "libfreenect2/registration.h"
#include "libfreenect2/packet_pipeline.h"
#include "libfreenect2/logger.h"

#include "filters/BaseFilter.h"

namespace bluecadet {
    
    namespace kinect2 {
        
        enum class PacketPipelineType {
            CPU,
            OPEN_GL,
            OPEN_CL
        };
        
        enum class FrameListenerType {
            COLOR,
            DEPTH,
			POINTCLOUD,
			COLOR_DEPTH
        };
		
		struct Point {
			ci::vec3 position;
			ci::ColorA color;
			
			Point() : position(ci::vec3(0)), color(ci::ColorA(0.0f, 0.0f, 0.0f, 0.0f)) {};
			Point(ci::vec3 _position, ci::ColorA _color) : position(_position), color(_color){};
		};
		
		typedef std::vector<Point> PointCloud;
		typedef std::shared_ptr<PointCloud> PointCloudRef;
		
		struct KinectData {
			ci::Surface8uRef colorFrame;
			ci::Channel32fRef rawDepthFrame;
			ci::Channel32fRef depthFrame;
			PointCloudRef pointCloud;
		};
		
		const int DEPTH_FRAME_WIDTH = 512;
		const int DEPTH_FRAME_HEIGHT = 424;
        const int POINT_CLOUD_SIZE = DEPTH_FRAME_WIDTH * DEPTH_FRAME_HEIGHT;
        
        typedef std::shared_ptr<class KinectInput> KinectInputRef;
        
        //! Each KinectInput instance only handles one Kinect sensor. In order to use multiple Kinect sensors, use multiple KinectInput instances.
        class KinectInput {
            
        public:
            
            KinectInput();
            ~KinectInput();
            
            //! Return device serial number, or empty if the index is invalid.
            std::string getDeviceSerialNumber(const int index);
			
			//! Return true if succeeds; false if failed.
			//! minDepth can be as small as 0.5 meter (setting it smaller won't help).
			//! maxDepth can be as large as 8.0 meter (or larger).
			bool setup(const std::string serial, 
					   const PacketPipelineType packetPipelineType = PacketPipelineType::CPU, 
					   const bool requestColorFrame = true, 
					   const bool requestDepthFrame = true, 
					   const bool requestPointCloud = false, 
					   const float minDepth = 0.5, 
					   const float maxDepth = 8, 
					   const bool autoReconnection = true, 
					   const float reconnctionInterval = 60);

			void update();
            void destory();

			bool isConnected();
            
            bool hasNewData();
			KinectData getData();
			
			//! Add a post-processing filter to the depth image.
			void addFilter(const BaseFilterRef filter);
			
			//! Remove all filters.
			void clearPostProcessingPipeline();
			
        private:
			const float WAIT_FOR_NEW_FRAME_TIMEOUT = 1000; // milliseconds
			const int NUM_TIMEOUTS_TO_CONFIRM_DISCONNECTION = 10;
			
			// libfreenect2 utilities:
            libfreenect2::Freenect2 mFreenect2; // library context to find and open Kinect sensors
            std::shared_ptr<libfreenect2::Freenect2Device> mDevice; // device control
            std::shared_ptr<libfreenect2::SyncMultiFrameListener> mListener; // collects raw data from Kinect
            libfreenect2::FrameMap mFrames; // storage of raw data
            std::shared_ptr<libfreenect2::Registration> mRegistration; // undistorts depth data
            std::shared_ptr<libfreenect2::Frame> mUndistortedDepthFrame; // storage of undistorted depth data
			std::shared_ptr<libfreenect2::Frame> mRegisteredColorFrame; // storage of registered color data

			// Connection utilities:
			std::string mSerial;
			PacketPipelineType mPacketPipelineType; 
			bool mRequestColorFrame;
			bool mRequestDepthFrame;
			bool mRequestPointCloud;
			float mMinDepth;
			float mMaxDepth;
			bool setup(); // Start Kinect sensor and worker thread; returns true if succeed

			std::atomic<bool> mIsConnected {false};
			bool mAutoReconnection;
			float mReconnectionInterval;
			ci::Timer mReconnectionTimer;
			bool mReconnectionScheduled = false;
			
			// Worker thread which does the actual work of collect and process data
			std::thread mWorkerThread; // worker thread to collect raw data from Kinect and convert data to Cinder formats in the background
			std::atomic<bool> mAboutToDestoryWorkerThread {false}; // indicates whether worker thread should terminate
			void collectAndConvertData(); // worker thread routine
			
			// Data that can be accessd by both main and worker threads
			std::mutex mDataMutex; // protects shared data
            ci::Surface8uRef mColorFrame; // color frame in Cinder format
			ci::Channel32fRef mRawDepthFrame; // raw depth frame in Cinder format
            ci::Channel32fRef mDepthFrame; // depth frame (after post-processing if any) in Cinder format
			PointCloudRef mPointCloud; // storage of point cloud
            
			std::atomic<bool> mHasNewData = {false}; // whether there are new frames for user; will be set to false once user calls getData().
			
			// These shared member variables will only be set by setup() once, so they don't need to be protected by mutex:
			
			// Post-procesing pipeline:
			std::vector<BaseFilterRef> mPostProcessingPipeline;
        };
        
    }
    
}

