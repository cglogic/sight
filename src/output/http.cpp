#include "http.h"

#include <glog/logging.h>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

namespace Sight::Output {

Http::Http(const json& config,
           size_t id,
           std::vector<std::vector<Slot>>& slot,
           Queue<uint32_t>& queue) :
	Dummy(config, id, slot, queue) {
	mUrl = config["url"];
	mToken = config["token"];
	mApi = config["api"];
}

Http::Http(Http&& other) noexcept :
	Dummy(std::move(other)),
	mUrl(std::move(other.mUrl)),
	mToken(std::move(other.mToken)),
	mApi(std::move(other.mApi)) {
}

Http::~Http() {
}

bool Http::validate(const json& config) {
	if (!Dummy::validate(config)) {
		return false;
	}
	if (!config.contains("url") || !config["url"].is_string() || config["url"].empty()) {
		LOG(ERROR) << "Url is not exists, not string or empty";
		return false;
	}
	if (!config.contains("token") || !config["token"].is_string() || config["token"].empty()) {
		LOG(ERROR) << "Token is not exists, not string or empty";
		return false;
	}
	if (!config.contains("api") || !config["api"].is_string() || config["api"].empty()) {
		LOG(ERROR) << "Api is not exists, not string or empty";
		return false;
	}
	return true;
}

bool Http::send(Slot& slot) {
	const AVFrame* frame = slot.frame();
	if (frame == nullptr) {
		LOG(ERROR) << mName << ": Error encodind frame";
		return true;
	}

	const AVPacket* picture = packet(slot, AV_CODEC_ID_MJPEG);
	if (picture == nullptr) {
		LOG(ERROR) << mName << ": Error encodind packet";
		return true;
	}

	auto& meta = slot.meta();

	LOG(INFO) << mName
	          << ": Event stream name = " << slot.streamName()
	          << ", timestamp = " << timestampNow()
	          << ", frame number = " << frame->coded_picture_number
	          << ", frame dts = " << frame->pkt_dts
	          << ", frame pts = " << frame->pts
	          << ", frame size = " << frame->pkt_size
	          << ", frame width = " << frame->width
	          << ", frame height = " << frame->height
	          << ", packet pts: " << picture->pts
	          << ", packet dts: " << picture->dts
	          << ", packet size: " << picture->size
	          << ", stream index: " << picture->stream_index
	          << ", meta = " << meta.dump();

	std::vector<uint8_t> image;
	image.assign(picture->data, picture->data + picture->size);

	json body;
	body["timestamp"] = timestampNow();

	body["files"] = json::array();
	body["files"].push_back(json::object());
	body["files"].back()["file"] = json::binary(image);
	body["files"].back()["is_main"] = true;
	body["files"].back()["format"] = "jpg";

	auto& meta = slot.meta();
	auto mit = meta.end();
	if (--mit != meta.end()) {
		body["event_type"] = mit.key();
		body["info"] = mit.value();
	} else {
		body["event_type"] = "none";
	}

	std::vector<uint8_t> bodyPacked = json::to_msgpack(body);
	// LOG(INFO) << "body: " << body;
	std::string str(bodyPacked.begin(), bodyPacked.end());

	httplib::Client cli(mUrl.c_str());
	// cli.set_connection_timeout(0, 2000000); // 1 seconds
	// cli.set_read_timeout(2, 0); // 2 seconds
	// cli.set_write_timeout(2, 0); // 2 seconds
	httplib::Headers headers = {
		{"Authorization", std::string("Token ") + mToken}
	};

	if (auto res = cli.Post(mApi.c_str(), headers, str, "application/msgpack")) {
		if (res->status != 200 && res->status != 201) {
			LOG(ERROR) << mName << ": HTTP status = " << res->status << ", body: " << res->body;
			return false;
		}
	} else {
		LOG(ERROR) << mName << ": Post error = " << static_cast<int>(res.error());
		return false;
	}

	return true;
}

}
