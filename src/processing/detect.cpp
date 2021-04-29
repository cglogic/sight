#include "detect.h"

#include <glog/logging.h>

namespace Sight::Processing {

Detect::Detect(const json& config,
               size_t id,
               std::vector<std::vector<Slot>>& slot,
               Queue<uint32_t>& queueIn,
               std::vector<Queue<uint32_t>>& queueOut,
               std::vector<size_t>& queueOutId) :
	Dummy(config, id, slot, queueIn, queueOut, queueOutId) {
}

Detect::Detect(Detect&& other) :
	Dummy(std::move(other)) {
}

Detect::~Detect() {
}

bool Detect::validate(const json& config) {
	if (!Dummy::validate(config)) {
		return false;
	}
	return true;
}

bool Detect::detect(Slot& slot) {
	const AVFrame& frame = slot.frame(AV_PIX_FMT_RGB24, 416, 416);

	// Do the work

	auto& meta = slot.meta(mType);
	meta["in"] = 0;
	meta["out"] = 0;

	return true;
}

}
