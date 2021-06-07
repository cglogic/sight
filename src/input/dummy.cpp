#include "dummy.h"

#include <set>

#include <glog/logging.h>

namespace Sight::Input {

Dummy::Dummy(const json& config,
             size_t id,
             std::vector<Slot>& slot,
             std::vector<Queue<uint32_t>>& queue,
             std::vector<size_t>& queueId) :
	Module(config, id),
	mSlot(slot),
	mQueue(queue),
	mQueueId(queueId) {
	mLive = config["live"];
	mFrame = av_frame_alloc();
	if (!mFrame) {
		LOG(ERROR) << mName << ": Failed to allocated memory for AVFrame";
	}
}

Dummy::Dummy(Dummy&& other) noexcept :
	Module(std::move(other)),
	mLive(std::exchange(other.mLive, false)),
	mSlot(other.mSlot),
	mQueue(other.mQueue),
	mQueueId(other.mQueueId) {
}

Dummy::~Dummy() {
	av_frame_free(&mFrame);
}

bool Dummy::validate(const json& config) {
	if (!Module::validate(config)) {
		return false;
	}
	if (!config.contains("live") || !config["live"].is_boolean()) {
		LOG(ERROR) << "Live is not exists or not boolean";
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

bool Dummy::start() {
	if (mLive) {
		mDroppedFirst = false;
	}
	return true;
}

void Dummy::task() {
	AVFrame* frame = mFrame;
	bool realFrame = false;
	auto& slot = mSlot[mSlotId];
	if (mLive) {
		if (mDroppedFirst) {
			if (slot.ready()) {
				frame = slot.source();
				realFrame = true;
			}
		} else {
			mDroppedFirst = true;
		}
	} else {
		if (!slot.ready()) {
			slot.wait();
		}
		frame = slot.source();
		realFrame = true;
	}

	Result result = read(frame);
	switch (result) {
		case Result::success:
			if (realFrame) {
				slot.reset();
				for (auto& queueId : mQueueId) {
					mQueue[queueId].put(pack(mId, mSlotId, true));
					mQueue[queueId].notify();
				}
				mSlotId = mSlotId + 1 < mSlot.size() ? mSlotId + 1 : 0;
			}
			break;
		case Result::again:
			// Need more data
			break;
		case Result::eof:
			if (mLive) {
				stop();
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				if (!start()) {
					deactivate();
				}
			} else {
				deactivate();
			}
			break;
		case Result::changed:
			stop();
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			if (!start()) {
				deactivate();
			}
			break;
		case Result::unsupported:
			// It can be audio
			break;
		case Result::error:
			// Nothing for now
			break;
	}
}

Dummy::Result Dummy::read([[maybe_unused]]AVFrame* frame) {
	return Result::eof;
}

}
