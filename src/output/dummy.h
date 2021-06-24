#pragma once

#include "module.h"

#include <map>
#include <atomic>

#include "queue.h"
#include "slot.h"

namespace Sight::Output {

class Dummy
	: public Module {
public:
	Dummy(const json& config,
	      size_t id,
	      std::vector<std::vector<Slot>>& slot,
	      Queue<uint32_t>& queue);
	Dummy(const Dummy& other) = delete;
	Dummy(Dummy&& other) noexcept;
	virtual ~Dummy();

	static bool validate(const json& config);

protected:
	void task() override;
	bool start() override;
	void stop() override;
	virtual bool send(Slot& slot);

	std::string timestampNow();

	const AVPacket* packet(Slot& slot, AVCodecID format);

	bool mLocalTime = true;
	size_t mResendInterval = 0;

private:
	std::vector<std::vector<Slot>>& mSlot;
	Queue<uint32_t>& mQueue;

	Queue<Slot> mSendQueue;
	std::thread mSender;

	void sender();
	std::atomic_flag mSend = ATOMIC_FLAG_INIT;

	struct Encoder {
		AVCodec* mCodec = NULL;
		AVCodecContext* mContext = NULL;
	};

	std::map<size_t, Encoder> mEncoder;

	struct Packet {
		int mFrameId = 0;
		AVPacket* mPacket = NULL;
	};

	std::map<size_t, Packet> mPacket;

};

}
