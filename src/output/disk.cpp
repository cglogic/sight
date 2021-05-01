#include "disk.h"

#include <fstream>

#include <glog/logging.h>

namespace Sight::Output {

Disk::Disk(const json& config,
           size_t id,
           std::vector<std::vector<Slot>>& slot,
           Queue<uint32_t>& queue) :
	Dummy(config, id, slot, queue),
	mPath(config["path"]) {
}

Disk::Disk(Disk&& other) :
	Dummy(std::move(other)),
	mPath(std::move(other.mPath)) {
}

Disk::~Disk() {
}

bool Disk::validate(const json& config) {
	if (!Dummy::validate(config)) {
		return false;
	}
	if (!config.contains("path") || !config["path"].is_string() || config["path"].empty()) {
		LOG(ERROR) << "Path is not exists, not string or empty";
		return false;
	}
	return true;
}

bool Disk::start() {
	fs::directory_entry dir(mPath);
	if (!dir.exists()) {
		if (!fs::create_directory(mPath)) {
			LOG(ERROR) << mName << ": Can not create root directory, path = " << mPath;
			return false;
		}
	}
	return Dummy::start();
}

bool Disk::send(Slot& slot) {
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
	          << ": Event stream id = " << slot.streamId()
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

	// Create stream directory if it does not exist
	fs::path streamDir(mPath / std::to_string(slot.streamId()));
	if (!fs::directory_entry(streamDir).exists()) {
		if (!fs::create_directory(streamDir)) {
			LOG(ERROR) << mName
			           << ": Can not create stream directory, path = "
			           << streamDir;
			return true;
		}
	}

	// Create event directory
	fs::path eventDir(streamDir / timestampNow());
	if (!fs::create_directory(eventDir)) {
		LOG(ERROR) << mName
		           << ": Can not create event directory, path = "
		           << eventDir;
		return true;
	}

	// Write frame
	fs::path frameFile(eventDir / "frame.jpeg");
	std::ofstream fileFrame(frameFile, std::ios::binary);
	if (!fileFrame.is_open()) {
		LOG(ERROR) << mName
		           << ": Can't open file to save frame, file = "
		           << frameFile;
		return true;
	}
	fileFrame.write(reinterpret_cast<char*>(picture->data), picture->size);

	// Write meta
	fs::path metaFile(eventDir / "meta.json");
	std::ofstream fileMeta(metaFile);
	if (!fileMeta.is_open()) {
		LOG(ERROR) << mName
		           << ": Can't open file to save meta, file = "
		           << metaFile;
		return true;
	}
	std::string metaString = meta.dump(4);
	fileMeta << metaString;

	return true;
}

}
