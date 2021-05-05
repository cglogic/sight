#include "module.h"

#include <glog/logging.h>

namespace Sight {

Module::Module(const json& config,
               size_t id) :
	mId(id) {
	mName = config["name"];
	mType = config["type"];
}

Module::Module(Module&& other) :
	mId(std::exchange(other.mId, 0)),
	mName(std::move(other.mName)),
	mType(std::move(other.mType)) {
	if (other.mRun.test()) {
		other.terminate();
		other.wait();
		run();
	}
}

Module::~Module() {
	terminate();
	wait();
}

bool Module::validate(const json& config) {
	if (!config.contains("name") || !config["name"].is_string() || config["name"].empty()) {
		LOG(ERROR) << "Name is not exists, not string or empty";
		return false;
	}
	if (!config.contains("type") || !config["type"].is_string() || config["type"].empty()) {
		LOG(ERROR) << "Type is not exists, not string or empty";
		return false;
	}
	return true;
}

uint32_t Module::pack(uint16_t stream, uint8_t slot, bool flag) {
	return (stream << 16) | (slot << 8) | (flag ? 0x1 : 0x0);
}

void Module::unpack(uint32_t packed, uint16_t& stream, uint8_t& slot, bool& flag) {
	stream = packed >> 16;
	slot = (packed >> 8) & 0xffff;
	flag = packed & 0x1;
}

void Module::run() {
	if (!mRun.test()) {
		mFinished.clear();
		mRun.test_and_set();
		mWorker = std::thread(&Module::worker, this);
	}
}

bool Module::running() const {
	if (mWorker.joinable()) {
		return !mFinished.test();
	}
	return false;
}

void Module::terminate() {
	if (mRun.test()) {
		mRun.clear();
	}
}

void Module::wait() {
	if (mWorker.joinable()) {
		mWorker.join();
	}
}

bool Module::start() {
	return true;
}

void Module::stop() {
}

void Module::task() {
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

bool Module::active() {
	return mRun.test();
}

void Module::deactivate() {
	mRun.clear();
}

void Module::worker() {
	pthread_setname_np(pthread_self(), (mName + ":worker").c_str());
	if (start()) {
		LOG(INFO) << mName << ": Started";
		while (mRun.test()) {
			task();
		}
	} else {
		LOG(ERROR) << mName << ": Start failed";
	}
	stop();
	LOG(INFO) << mName << ": Stopped";
	mFinished.test_and_set();
}

}
