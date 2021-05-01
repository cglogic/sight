#pragma once

#include "dummy.h"

namespace Sight::Input {

class Stream
	: public Dummy {
public:
	Stream(const json& config,
	       size_t id,
	       std::vector<Slot>& slot,
	       std::vector<Queue<uint32_t>>& queue,
	       std::vector<size_t>& queueId);
	Stream(const Stream& other) = delete;
	Stream(Stream&& other);
	virtual ~Stream();

	static bool validate(const json& config);

protected:
	virtual bool start();
	virtual void stop();
	virtual Result read(AVFrame* frame);

private:
	AVDictionary* mOptions = NULL;
	AVFormatContext* mFormatContext = NULL;
	AVCodec* mCodec = NULL;
	AVCodecParameters* mCodecParameters = NULL;
	AVCodecContext* mCodecContext = NULL;
	int mVideoStream = -1;
	AVPacket* mPacket = NULL;

	std::string mUrl = "";
	std::string mProtocol = "";
	int64_t mTimeout = -1;

};

}
