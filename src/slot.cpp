#include "slot.h"

#include <glog/logging.h>

namespace Sight {

Slot::Slot(size_t streamId, const std::string& streamName, size_t stageCount) :
	mStreamId(streamId),
	mStreamName(streamName),
	mStageCount(stageCount) {
	mSource = av_frame_alloc();
	if (!mSource) {
		LOG(ERROR) << "Failed to allocate memory for AVFrame";
	}
}

Slot::Slot(const Slot& other) :
	mStreamId(other.mStreamId),
	mStreamName(other.mStreamName),
	mStageCount(other.mStageCount),
	mMeta(other.mMeta),
	mFresh(other.mFresh) {
	mSource = av_frame_alloc();
	if (!mSource) {
		LOG(ERROR) << "Failed to allocate memory for AVFrame";
		return;
	}

	mSource->format = other.mSource->format;
	mSource->width = other.mSource->width;
	mSource->height = other.mSource->height;

	if (av_frame_get_buffer(mSource, 0) < 0) {
		LOG(ERROR) << "Failed to allocate buffer";
		return;
	}

	if (av_frame_copy(mSource, other.mSource) < 0) {
		LOG(ERROR) << "Failed to copy frame";
		return;
	}

	if (av_frame_copy_props(mSource, other.mSource) < 0) {
		LOG(ERROR) << "Failed to copy props";
		return;
	}
}

Slot::Slot(Slot&& other) noexcept :
	mStreamId(std::exchange(other.mStreamId, 0)),
	mStreamName(std::move(other.mStreamName)),
	mStageCount(std::exchange(other.mStageCount, 0)),
	mMeta(std::move(other.mMeta)),
	mFresh(std::exchange(other.mFresh, false)),
	mSource(std::exchange(other.mSource, NULL)),
	mFrame(std::move(other.mFrame)) {
}

Slot::~Slot() {
	clear();
	av_frame_free(&mSource);
}

void Slot::clear() {
	std::lock_guard<std::mutex> lg(mLockFrame);
	for (auto& f : mFrame) {
		av_freep(&f.mFrame->data[0]);
		av_frame_free(&f.mFrame);
		sws_freeContext(f.mSwsContext);
	}
	mFrame.clear();
}

bool Slot::ready() const {
	return !mReady.test();
}

bool Slot::fresh() const {
	return mFresh;
}

void Slot::wait() const {
	mReady.wait(true);
}

void Slot::reset() {
	if (mSource->width != mWidth || mSource->height != mHeight ||
	    mSource->pts <= mPts || mSource->pkt_dts <= mDts) {
		mWidth = mSource->width;
		mHeight = mSource->height;
		mFresh = true;
		clear();
	}
	mPts = mSource->pts;
	mDts = mSource->pkt_dts;
	mReference = mStageCount;
	mReady.test_and_set();
}

void Slot::unref() {
	if (--mReference == 0) {
		mFresh = false;
		mMeta.clear();
		mReady.clear();
		mReady.notify_one();
	}
}

size_t Slot::streamId() const {
	return mStreamId;
}

const std::string& Slot::streamName() const {
	return mStreamName;
}

AVFrame* Slot::source() {
	return mSource;
}

const AVFrame* Slot::frame(AVPixelFormat format, int width, int height, int scale) {
	std::lock_guard<std::mutex> lg(mLockFrame);

	// Default to original frame
	if ((format == AV_PIX_FMT_NONE) ||
	    (format == mSource->format  && ((width == 0 && height == 0) ||
	     (width == mSource->width && height == mSource->height)))) {
		return mSource;
	}

	// Tune dimensions to preserve aspect ratio
	if (width == 0 && height != 0) {
		double factor = (double)mSource->width / (double)mSource->height;
		width = (int)((double)height * factor);
	} else if (height == 0 && width != 0) {
		double factor = (double)mSource->height / (double)mSource->width;
		height = (int)((double)width * factor);
	} else if (width == 0 && height == 0) {
		width = mSource->width;
		height = mSource->height;
	}

	// Frame with the same format already exists
	for (auto& f : mFrame) {
		if (f.mFrame->format == format && f.mFrame->width == width && f.mFrame->height == height) {
			if (f.mFrame->coded_picture_number != mSource->coded_picture_number) {
				sws_scale(f.mSwsContext, (const uint8_t* const*)mSource->data, mSource->linesize, 0,
				          f.mFrame->height, f.mFrame->data, f.mFrame->linesize);
				f.mFrame->coded_picture_number = mSource->coded_picture_number;
			}
			return f.mFrame;
		}
	}

	// Create a new frame and scaler context
	Frame frame;
	AVPixelFormat pixFormat = (AVPixelFormat)mSource->format;
	bool correctRange = false;
	switch (mSource->format)	{
		case AV_PIX_FMT_YUVJ420P:
			pixFormat = AV_PIX_FMT_YUV420P;
			correctRange = true;
			break;
		case AV_PIX_FMT_YUVJ422P:
			pixFormat = AV_PIX_FMT_YUV422P;
			correctRange = true;
			break;
		case AV_PIX_FMT_YUVJ444P:
			pixFormat = AV_PIX_FMT_YUV444P;
			correctRange = true;
			break;
		case AV_PIX_FMT_YUVJ440P:
			pixFormat = AV_PIX_FMT_YUV440P;
			correctRange = true;
			break;
	}

	frame.mSwsContext = sws_getContext(mSource->width, mSource->height, pixFormat,
	                       width, height, format,
	                       scale, NULL, NULL, NULL);
	if (!frame.mSwsContext) {
		LOG(ERROR) << "Failed to create SwsContext";
		return nullptr;
	}

	if (correctRange) {
		int dummy[4];
		int srcRange, dstRange;
		int brightness, contrast, saturation;
		sws_getColorspaceDetails(frame.mSwsContext, (int**)&dummy, &srcRange, (int**)&dummy,
		                         &dstRange, &brightness, &contrast, &saturation);
		const int* coefs = sws_getCoefficients(SWS_CS_DEFAULT);
		srcRange = 1;
		sws_setColorspaceDetails(frame.mSwsContext, coefs, srcRange, coefs,
		                         dstRange, brightness, contrast, saturation);
	}

	frame.mFrame = av_frame_alloc();
	frame.mFrame->format = format;
	frame.mFrame->width = width;
	frame.mFrame->height = height;

	int ret = av_image_alloc(frame.mFrame->data, frame.mFrame->linesize,
	                         frame.mFrame->width, frame.mFrame->height,
	                         format, 32);
	if (ret < 0) {
		av_frame_free(&frame.mFrame);
		LOG(ERROR) << "Failed to allocate memory for AVFrame buffer";
		return nullptr;
	}

	sws_scale(frame.mSwsContext, (const uint8_t* const*)mSource->data, mSource->linesize, 0,
	          frame.mFrame->height, frame.mFrame->data, frame.mFrame->linesize);
	frame.mFrame->coded_picture_number = mSource->coded_picture_number;

	mFrame.push_back(frame);

	return mFrame.back().mFrame;
}

const json& Slot::meta() const {
	return mMeta;
}

json& Slot::meta(const std::string& type) {
	std::lock_guard<std::mutex> lg(mLockMeta);
	if (!mMeta.contains(type)) {
		mMeta[type] = json::object();
	}
	return mMeta[type];
}

}
