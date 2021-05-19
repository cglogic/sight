#include "stream.h"

#include <glog/logging.h>

namespace Sight::Input {

Stream::Stream(const json& config,
               size_t id,
               std::vector<Slot>& slot,
               std::vector<Queue<uint32_t>>& queue,
               std::vector<size_t>& queueId) :
	Dummy(config, id, slot, queue, queueId) {
	mUrl = config["url"];

	if (config.contains("options")) {
		for (auto& el : config["options"].items()) {
			std::string value;
			if (el.value().is_string()) {
				value = el.value();
			} else if (el.value().is_number_float()) {
				double number = el.value();
				value = std::to_string(number);
			} else if (el.value().is_number()) {
				int64_t number = el.value();
				value = std::to_string(number);
			}
			av_dict_set(&mOptions, el.key().c_str(), value.c_str(), 0);
		}
	}
}

Stream::Stream(Stream&& other) :
	Dummy(std::move(other)),
	mUrl(std::move(other.mUrl)) {
}

Stream::~Stream() {
	av_dict_free(&mOptions);
}

bool Stream::validate(const json& config) {
	if (!Dummy::validate(config)) {
		return false;
	}
	if (!config.contains("url") || !config["url"].is_string() || config["url"].empty()) {
		LOG(ERROR) << "Url is not exists, not string or empty";
		return false;
	}
	if (config.contains("options")) {
		if (config["options"].is_object()) {
			for (auto& el : config["options"].items()) {
				if (!(el.value().is_string() || el.value().is_number_float() || el.value().is_number())) {
					LOG(ERROR) << "Unsupported option type";
					return false;
				}
			}
		} else {
			LOG(ERROR) << "Options is not an object";
			return false;
		}
	}
	return true;
}

bool Stream::start() {
	Dummy::start();

	mFormatContext = avformat_alloc_context();
	if (!mFormatContext) {
		LOG(ERROR) << mName << ": Could not allocate memory for Format Context";
		return false;
	}

	int response = avformat_open_input(&mFormatContext, mUrl.c_str(), NULL, &mOptions);
	if (response < 0) {
		LOG(ERROR) << mName
		           << ": Could not open stream, error = " << response
		           << ", text = " << std::string(av_err2str(response));
		return false;
	}

	response = avformat_find_stream_info(mFormatContext, NULL);
	if (response < 0) {
		LOG(ERROR) << mName
		           << ": Could not find stream info, error = " << response
		           << ", text = " << std::string(av_err2str(response));
		return false;
	}

	for (size_t i = 0; i < mFormatContext->nb_streams; ++i) {
		AVCodecParameters* localCodecParameters = mFormatContext->streams[i]->codecpar;

		// LOG(INFO) << "AVStream->time_base before open coded "
		//           << mFormatContext->streams[i]->time_base.num << "/"
		//           << mFormatContext->streams[i]->time_base.den;
		// LOG(INFO) << "AVStream->r_frame_rate before open coded "
		//           << mFormatContext->streams[i]->r_frame_rate.num << "/"
		//           << mFormatContext->streams[i]->r_frame_rate.den;
		// LOG(INFO) << "AVStream->start_time " << mFormatContext->streams[i]->start_time;
		// LOG(INFO) << "AVStream->duration " << mFormatContext->streams[i]->duration;

		AVCodec* localCodec = NULL;
		localCodec = avcodec_find_decoder(localCodecParameters->codec_id);

		if (localCodec == NULL) {
			LOG(ERROR) << mName << ": Unsupported codec";
			continue;
		}

		if (localCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (mVideoStream == -1) {
				mVideoStream = i;
				mCodec = localCodec;
				mCodecParameters = localCodecParameters;
			} else {
				LOG(WARNING) << mName << ": Found another video stream";
			}

			// LOG(INFO) << "Video Codec: resolution "
			//           << localCodecParameters->width << "x"
			//           << localCodecParameters->height;
		}

		// LOG(INFO) << "Codec " << localCodec->name
		//           << " ID " << localCodec->id
		//           << " bit_rate " << localCodecParameters->bit_rate;
	}

	if (mVideoStream == -1) {
		LOG(ERROR) << mName << ": Could not find video stream";
		return false;
	}

	mCodecContext = avcodec_alloc_context3(mCodec);
	if (!mCodecContext) {
		LOG(ERROR) << mName << ": Failed to allocated memory for AVCodecContext";
		return false;
	}

	if (avcodec_parameters_to_context(mCodecContext, mCodecParameters) < 0) {
		LOG(ERROR) << mName << ": Failed to copy codec params to codec context";
		return false;
	}

	if (avcodec_open2(mCodecContext, mCodec, NULL) < 0) {
		LOG(ERROR) << mName << ": Failed to open codec through avcodec_open2";
		return false;
	}

	LOG(INFO) << mName
	          << ": Codec id: " << mCodecContext->codec_id
	          << ", bit rate: " << mCodecContext->bit_rate
	          << ", time base: " << mCodecContext->time_base.num << "/" << mCodecContext->time_base.den
	          << ", frame size: " << mCodecContext->width << "x" << mCodecContext->height
	          << ", pixel format: " << mCodecContext->pix_fmt;

	mPacket = av_packet_alloc();
	if (!mPacket) {
		LOG(ERROR) << mName << ": Failed to allocated memory for AVPacket";
		return false;
	}

	return true;
}

void Stream::stop() {
	if (mCodecContext) {
		avcodec_send_packet(mCodecContext, NULL);
	}

	avformat_close_input(&mFormatContext);
	avcodec_free_context(&mCodecContext);
	av_packet_free(&mPacket);

	mCodec = NULL;
	mCodecParameters = NULL;
	mVideoStream = -1;
}

Dummy::Result Stream::read(AVFrame* frame) {
	av_packet_unref(mPacket);

	int response = av_read_frame(mFormatContext, mPacket);
	if (response >= 0) {
		if (mPacket->stream_index == mVideoStream) {
			// LOG(INFO) << "AVPacket->pts " << mPacket->pts;
			response = avcodec_send_packet(mCodecContext, mPacket);
			if (response >= 0) {
				response = avcodec_receive_frame(mCodecContext, frame);
				if (response >= 0) {
					// LOG(INFO) << "Context pix_fmt " << mCodecContext->pix_fmt;
					// LOG(INFO) << "Frame " << mCodecContext->frame_number
					//           << " (type=" << av_get_picture_type_char(frame->pict_type)
					//           << ", size=" << frame->pkt_size
					//           << " bytes, format=" << frame->format
					//           << ") pts " << frame->pts
					//           << " key_frame " << frame->key_frame
					//           << " [DTS " << frame->coded_picture_number << "]";
				} else if (response == AVERROR_EOF) {
					LOG(WARNING) << mName << ": Stream EOF reached by decoder";
					return Result::eof;
				} else if (response == AVERROR_INPUT_CHANGED) {
					LOG(WARNING) << mName << ": Stream input changed";
					return Result::changed;
				} else if (response == AVERROR(EAGAIN)) {
					return Result::again;
				} else {
					LOG(ERROR) << mName
					           << ": Error while receiving frame from the decoder, error = " << response
					           << ", text = " << std::string(av_err2str(response));
					return Result::error;
				}
			} else {
				LOG(ERROR) << mName
				           << ": Error while sending a packet to decoder, error = " << response
				           << ", text = " << std::string(av_err2str(response));
				return Result::error;
			}
		} else {
			return Result::unsupported;
		}
	} else if (response == AVERROR_EOF) {
		LOG(WARNING) << mName << ": Stream EOF reached by packet reader";
		return Result::eof;
	} else {
		LOG(ERROR) << mName
		           << ": Could not read frame, error = " << response
		           << ", text = " << std::string(av_err2str(response));
		return Result::error;
	}

	return Result::success;
}

}
