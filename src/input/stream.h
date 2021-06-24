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
	Stream(Stream&& other) noexcept;
	~Stream();

	static bool validate(const json& config);

protected:
	bool start() override;
	void stop() override;
	Result read(AVFrame* frame) override;

private:
	AVDictionary* mOptions = NULL;
	AVFormatContext* mFormatContext = NULL;
	AVCodec* mCodec = NULL;
	AVCodecParameters* mCodecParameters = NULL;
	AVCodecContext* mCodecContext = NULL;
	int mVideoStream = -1;
	AVPacket* mPacket = NULL;

	std::string mUrl = "";

};

}
