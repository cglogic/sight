#pragma once

#include "module.h"

#include "queue.h"
#include "slot.h"

namespace Sight::Processing {

class Dummy
	: public Module {
public:
	Dummy(const json& config,
	      size_t id,
	      std::vector<std::vector<Slot>>& slot,
	      Queue<uint32_t>& queueIn,
	      std::vector<Queue<uint32_t>>& queueOut,
	      std::vector<size_t>& queueOutId);
	Dummy(const Dummy& other) = delete;
	Dummy(Dummy&& other) noexcept;
	virtual ~Dummy();

	static bool validate(const json& config);

protected:
	void task() override;
	virtual bool detect(Slot& slot);

	uint64_t mDelay = 0;
	bool mDrop = false;

private:
	std::vector<std::vector<Slot>>& mSlot;
	Queue<uint32_t>& mQueueIn;
	std::vector<Queue<uint32_t>>& mQueueOut;
	std::vector<size_t> mQueueOutId;

};

}
