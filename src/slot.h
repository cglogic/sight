#pragma once

#include <atomic>
#include <map>
#include <string>
#include <vector>
#include <mutex>

extern "C" {
#	include <libavcodec/avcodec.h>
#	include <libavformat/avformat.h>
#	include <libavutil/imgutils.h>
#	include <libswscale/swscale.h>
}

#include <nlohmann/json.hpp>

namespace Sight {

using json = nlohmann::json;

class Slot {
public:
	Slot(size_t streamId,
	     size_t stageCount);
	Slot(const Slot& slot);
	Slot(Slot&& slot);
	~Slot();

	bool ready() const;
	void wait() const;
	void reset();
	void unref();
	size_t streamId() const;
	bool fresh() const;

	AVFrame* source();
	const AVFrame& frame(AVPixelFormat format = AV_PIX_FMT_NONE, int width = 0, int height = 0, int scale = SWS_BICUBIC);

	const json& meta() const;
	json& meta(const std::string& type);

private:
	size_t mStreamId = 0;
	size_t mStageCount = 0;

	void clearConverted();

	struct Frame {
		AVFrame* mFrame = NULL;
		SwsContext* mSwsContext = NULL;
	};

	int mWidth = 0;
	int mHeight = 0;
	int64_t mDts = 0;
	int64_t mPts = 0;

	json mMeta;
	mutable std::mutex mLockMeta;

	bool mFresh = true;
	std::atomic_ushort mReference = 0;
	mutable std::atomic_flag mReady = ATOMIC_FLAG_INIT;

	AVFrame* mSource = NULL;
	std::vector<Frame> mFrame;
	mutable std::mutex mLockFrame;
};

}
