#include "dummy.h"

#include <set>

#include <glog/logging.h>

namespace Sight::Processing {

Dummy::Dummy(const json& config,
             size_t id,
             std::vector<std::vector<Slot>>& slot,
             Queue<uint32_t>& queueIn,
             std::vector<Queue<uint32_t>>& queueOut,
             std::vector<size_t>& queueOutId) :
	Module(config, id),
	mSlot(slot),
	mQueueIn(queueIn),
	mQueueOut(queueOut),
	mQueueOutId(queueOutId) {
	if (config.contains("delay") && config["delay"].is_number()) {
		mDelay = config["delay"];
	}
	if (config.contains("drop") && config["drop"].is_boolean()) {
		mDrop = config["drop"];
	}
}

Dummy::Dummy(Dummy&& other) noexcept :
	Module(std::move(other)),
	mDelay(std::exchange(other.mDelay, 0)),
	mDrop(std::exchange(other.mDrop, false)),
	mSlot(other.mSlot),
	mQueueIn(other.mQueueIn),
	mQueueOut(other.mQueueOut),
	mQueueOutId(other.mQueueOutId) {
}

Dummy::~Dummy() {
}

bool Dummy::validate(const json& config) {
	if (!Module::validate(config)) {
		return false;
	}
	if (config.contains("drop") && !config["drop"].is_boolean()) {
		LOG(ERROR) << "Drop time is not boolean";
		return false;
	}
	if (config.contains("out") && config["out"].is_array() && !config["out"].empty()) {
		auto& out = config["out"];
		std::set<std::string> outUnique;
		for (size_t id = 0; id < out.size(); ++id) {
			if (out[id].is_string()) {
				std::string name = out[id];
				if (!outUnique.contains(name)) {
					outUnique.insert(name);
				} else {
					LOG(ERROR) << "Out array has duplicate values";
					return false;
				}
			} else {
				LOG(ERROR) << "Out array has non string value";
				return false;
			}
		}
	} else {
		LOG(ERROR) << "Out is not exists, not array or empty";
		return false;
	}
	return true;
}

void Dummy::task() {
	if (mQueueIn.ready()) {
		uint16_t streamId = 0;
		uint8_t slotId = 0;
		bool process = false;
		unpack(mQueueIn.get(), streamId, slotId, process);
		auto& slot = mSlot[streamId][slotId];
		if (process) {
			process = detect(slot);
		}
		for (auto& queueId : mQueueOutId) {
			mQueueOut[queueId].put(pack(streamId, slotId, process));
			mQueueOut[queueId].notify();
		}
		slot.unref();
	}
}

bool Dummy::detect([[maybe_unused]]Slot& slot) {
	// auto& info = slot.info(mType);
	if (mDelay > 0) {
		// info["delayed"] = mDelay;
		std::this_thread::sleep_for(std::chrono::milliseconds(mDelay));
	}
	if (mDrop) {
		return false;
	}
	return true;
}

}
