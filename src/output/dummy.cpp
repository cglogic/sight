#include "dummy.h"

#include <chrono>

#include <glog/logging.h>

namespace Sight::Output {

Dummy::Dummy(const json& config,
             size_t id,
             std::vector<std::vector<Slot>>& slot,
             Queue<uint32_t>& queue) :
	Module(config, id),
	mSlot(slot),
	mQueue(queue) {
	if (config.contains("local_time")) {
		mLocalTime = config["local_time"];
	}
	if (config.contains("resend_interval")) {
		mResendInterval = config["resend_interval"];
	}
}

Dummy::Dummy(Dummy&& other) :
	Module(std::move(other)),
	mSlot(other.mSlot),
	mQueue(other.mQueue) {
}

Dummy::~Dummy() {
	for (auto& e : mEncoder) {
		// Flush the encoder
		if (e.second.context) {
			avcodec_send_frame(e.second.context, NULL);
		}
		avcodec_close(e.second.context);
		avcodec_free_context(&e.second.context);
	}
	for (auto& p : mPacket) {
		av_packet_unref(p.second.packet);
		av_packet_free(&p.second.packet);
	}
}

bool Dummy::validate(const json& config) {
	if (!Module::validate(config)) {
		return false;
	}
	if (config.contains("local_time") && !config["local_time"].is_boolean()) {
		LOG(ERROR) << "Local time is not boolean";
		return false;
	}
	if (config.contains("resend_interval") && !config["resend_interval"].is_number()) {
		LOG(ERROR) << "Resend interval is not number";
		return false;
	}
	return true;
}

std::string Dummy::timestampNow() {
	auto tp = std::chrono::system_clock::now();
	auto mks = std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch());
	std::time_t ts = std::chrono::system_clock::to_time_t(tp);
	char timestamp[100];
	if (mLocalTime) {
		std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S.", std::localtime(&ts));
	} else {
		std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S.", std::gmtime(&ts));
	}
	std::string smks = std::to_string(mks.count() % 1000000);
	while (smks.size() < 6) {
		smks.insert(smks.begin(), '0');
	}
	return std::string(timestamp) + smks;
	// LOG(INFO) << mName << ": time" << std::format("%Y-%m-%dT%H:%M:%S.s", tp);
}

const AVPacket& Dummy::packet(const Slot& slot) {
	return *mPacket[slot.streamId()].packet;
}

bool Dummy::start() {
	if (!mSend.test()) {
		mSend.test_and_set();
		mSender = std::thread(&Dummy::sender, this);
		return true;
	}
	return false;
}

void Dummy::stop() {
	if (mSend.test()) {
		mSend.clear();
		mSender.join();
	}
}

void Dummy::task() {
	if (mQueue.ready()) {
		uint16_t streamId = 0;
		uint8_t slotId = 0;
		bool send = false;
		unpack(mQueue.get(), streamId, slotId, send);
		auto& slot = mSlot[streamId][slotId];
		if (send) {
			mSendQueue.put(Slot(slot));
			mSendQueue.notify();
		}
		slot.unref();
	}
}

bool Dummy::send(Slot& slot) {
	const AVFrame& frame = slot.frame();

	if (!encode(slot, AV_CODEC_ID_MJPEG)) {
		LOG(ERROR) << mName << ": Error encodind frame";
		return false;
	}

	const AVPacket& picture = packet(slot);
	auto& meta = slot.meta();

	LOG(INFO) << mName
	          << ": Event stream id = " << slot.streamId()
	          << ", timestamp = " << timestampNow()
	          << ", frame number = " << frame.coded_picture_number
	          << ", frame dts = " << frame.pkt_dts
	          << ", frame pts = " << frame.pts
	          << ", frame size = " << frame.pkt_size
	          << ", frame width = " << frame.width
	          << ", frame height = " << frame.height
	          << ", packet pts: " << picture.pts
	          << ", packet dts: " << picture.dts
	          << ", packet size: " << picture.size
	          << ", stream index: " << picture.stream_index
	          << ", meta = " << meta.dump();

	return true;
}

bool Dummy::encode(Slot& slot, AVCodecID format) {
	const AVFrame& frame = slot.frame(AV_PIX_FMT_YUVJ420P);
	size_t streamId = slot.streamId();

	if (mPacket.contains(streamId)) {
		if (mPacket[streamId].frameId == frame.coded_picture_number) {
			return true;
		} else {
			av_packet_unref(mPacket[streamId].packet);
		}
	} else {
		Packet pkt = {.packet = av_packet_alloc()};
		if (!pkt.packet) {
			LOG(ERROR) << mName << ": Could not allocate packet";
			return false;
		}
		// av_init_packet(pkt.packet);
		mPacket[streamId] = pkt;
	}

	Encoder* encoder = nullptr;
	if (mEncoder.contains(streamId)) {
		auto& enc = mEncoder[streamId];
		if (!slot.fresh() && enc.context->codec_id == format && enc.context->pix_fmt == frame.format) {
		    encoder = &mEncoder[streamId];
		} else {
			// Flush the encoder
			if (enc.context) {
				avcodec_send_frame(enc.context, NULL);
			}
			avcodec_close(enc.context);
			avcodec_free_context(&enc.context);
			auto it = mEncoder.find(streamId);
			mEncoder.erase(it);
		}
	}

	if (encoder == nullptr) {
		Encoder enc;
		enc.codec = avcodec_find_encoder(format);
		if (!enc.codec) {
			LOG(ERROR) << mName << ": Could not find encoder";
			return false;
		}
		enc.context = avcodec_alloc_context3(enc.codec);
		if (!enc.context) {
			LOG(ERROR) << mName << ": Could not allocate encoder context";
			return false;
		}

		enc.context->pix_fmt = (AVPixelFormat)frame.format;
		enc.context->height = frame.height;
		enc.context->width = frame.width;
		// enc.context->global_quality = 5;
		// enc.context->compression_level = 12;

		// We need only one frame, so hardcode these
		enc.context->time_base = (AVRational){1, 25};
		// enc.context->framerate = (AVRational){25, 1};

		if (avcodec_open2(enc.context, enc.codec, NULL) < 0) {
			avcodec_close(enc.context);
			LOG(ERROR) << mName << ": Could not open context";
			return false;
		}

		mEncoder[streamId] = enc;
		encoder = &mEncoder[streamId];
	}

	int response = avcodec_send_frame(encoder->context, &frame);
	if (response >= 0) {
		response = avcodec_receive_packet(encoder->context, mPacket[streamId].packet);
		if (response >= 0) {
			mPacket[streamId].frameId = frame.coded_picture_number;
			return true;
		} else {
			LOG(ERROR) << mName
			           << ": Could not receive packet, error = " << response
			           << ", text = " << av_err2str(response);
		}
	} else {
		LOG(ERROR) << mName
		           << ": Could not send frame, error = " << response
		           << ", text = " << av_err2str(response);
	}

	LOG(ERROR) << mName << ": Could not encode packet";
	return false;
}

void Dummy::sender() {
	while (mSend.test()) {
		if (mSendQueue.ready()) {
			Slot& slot = mSendQueue.first();
			if (send(slot)) {
				mSendQueue.remove();
			} else {
				LOG(ERROR) << mName << ": Could not send event";
				if (mResendInterval > 0) {
					std::this_thread::sleep_for(std::chrono::seconds(mResendInterval));
				} else {
					mSendQueue.remove();
				}
			}
		}
	}
}

}
