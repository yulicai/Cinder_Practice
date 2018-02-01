#pragma once

#include "cinder/app/App.h"
#include "cinder/Json.h"
#include "cinder/Timeline.h"

#include "../KinectInput.h"
#include "../filters/Filters.h"
#include "CinderOpenCV.h"

#include "HoverZone.h"

namespace bluecadet {
namespace kinect2 {

typedef std::shared_ptr<class HoverZoneManager> HoverZoneManagerRef;

class HoverZoneManager {
public:
	//! Singleton
	static HoverZoneManagerRef getInstance() {
		static HoverZoneManagerRef instance = nullptr;
		if (!instance) instance = HoverZoneManagerRef(new HoverZoneManager());
		return instance;
	}

	HoverZoneManager(HoverZoneManager const&) = delete;
	void operator=(HoverZoneManager const&) = delete;
	~HoverZoneManager();

	bool setup(const ci::ivec2 appSize);
	void update();
	void draw(const ci::vec2 offset = ci::vec2(0, 0));
	void cleanup();

	bool isConnected();

	HoverZoneRef registerHoverZone(const ci::Rectf &zone,
								   const std::function<void()>& trigger,
								   const std::function<void()>& cancel);
	void unregisterHoverZone(const HoverZoneRef &zone);

	void enableHoverZoneInNextFrame(const HoverZoneRef &zone);
	void disableHoverZoneInNextFrame(const HoverZoneRef &zone);

	void loadConfiguration(const ci::fs::path& jsonPath);
	void saveConfiguration(const ci::fs::path& jsonPath) const;

	ci::ivec2 getAppSize();
	void setAppSize(const ci::ivec2 size);

	ci::ivec2 getSize();

	bool getHorizontalFlip();
	void setHorizontalFlip(bool flip);

	bool getVerticalFlip();
	void setVerticalFlip(bool flip);

	ci::vec2 getSensingAreaCenter();
	void setSensingAreaCenter(const ci::vec2 center);

	ci::vec2 getSensingAreaSize();
	void setSensingAreaSize(const ci::vec2 size);

	float getSensingAreaRotation();
	void setSensingAreaRotation(const float rotation);

	float getDepthDetectionLowerThreshold();
	void setDepthDetectionLowerThreshold(const float threshold);

	float getDepthDetectionHigherThreshold();
	void setDepthDetectionHigherThreshold(const float threshold);

	unsigned int getIntersectionInitialThreshold();
	void setIntersectionInitialThreshold(const unsigned int threshold);

	unsigned int getIntersectionOngoingThreshold();
	void setIntersectionOngoingThreshold(const unsigned int threshold);

	unsigned int getDetectionFrameThreshold();
	void setDetectionFrameThreshold(const unsigned int threshold);

private:
	HoverZoneManager();

	// Worker thread which does the actual work of touch detection:
	std::thread mWorkerThread;
	std::atomic<bool> mAboutToDestoryWorkerThread { false }; // indicates whether worker thread should terminate
	void detectTouch(ci::gl::ContextRef context); // worker thread routine

	// Setup and destoried by main thread; used by worker thread:
	bluecadet::kinect2::KinectInput mKinectInput;

	// Shared by main and worker thread; protected by mutexes:
	ci::gl::TextureRef mDepthTexture;
	std::mutex mDepthTextureMutex;

	std::vector<HoverZoneRef> mHoverZones;
	std::mutex mHoverZonesMutex;

	// Used by main thread only, but contains reference to shared data:
	std::vector<HoverZoneRef> mHoverZonesToEnable;
	std::vector<HoverZoneRef> mHoverZonesToDisable;

	// Shared by main and worker thread; atomic:
	std::atomic<ci::ivec2> mAppSize;

	std::atomic<bool> mHorizontalFlip { true };
	std::atomic<bool> mVerticalFlip { false };

	std::atomic<ci::vec2> mSensingAreaCenter { ci::vec2(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT) / ci::vec2(2) };
	std::atomic<ci::vec2> mSensingAreaSize { ci::vec2(DEPTH_FRAME_WIDTH, DEPTH_FRAME_HEIGHT) };
	std::atomic<float> mSensingAreaRotation { 0 };

	std::atomic<float> mDepthDetectionLowerThreshold { 10 };
	std::atomic<float> mDepthDetectionHigherThreshold { 50 };

	std::atomic<unsigned int> mIntersectionInitialThreshold { 1000 };
	std::atomic<unsigned int> mIntersectionOngoingThreshold { 1000 };

	std::atomic<unsigned int> mDetectionFrameThreshold { 3 };

	std::atomic<bool> mHasUpdatedDepthData { false };
};

}
}