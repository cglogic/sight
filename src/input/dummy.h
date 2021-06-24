#pragma once

#include "module.h"

#include "queue.h"
#include "slot.h"

namespace Sight::Input {

class Dummy
	: public Module {
public:
	Dummy(const json& config,
	      size_t id,
	      std::vector<Slot>& slot,
	      std::vector<Queue<uint32_t>>& queue,
	      std::vector<size_t>& queueId);
	Dummy(const Dummy& other) = delete;
	Dummy(Dummy&& other) noexcept;
	virtual ~Dummy();

	static bool validate(const json& config);

protected:
	enum class Result {
		success,
		again,
		eof,
		changed,
		unsupported,
		error
	};

	void task() override;
	virtual Result read(AVFrame* frame);

	bool mLive = true;

private:
	std::vector<Slot>& mSlot;
	std::vector<Queue<uint32_t>>& mQueue;
	std::vector<size_t> mQueueId;

	AVFrame* mFrame = NULL;
	uint8_t mSlotId = 0;

};

}
