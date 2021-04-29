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
	Dummy(Dummy&& other);
	virtual ~Dummy();

	static bool validate(const json& config);
	std::string timestampNow();

	const AVPacket& packet(const Slot& slot);

protected:
	virtual void task();
	virtual bool start();
	virtual void stop();
	virtual bool send(Slot& slot);

	bool encode(Slot& slot, AVCodecID format);

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
		AVCodec* codec = NULL;
		AVCodecContext* context = NULL;
	};

	std::map<size_t, Encoder> mEncoder;

	struct Packet {
		int frameId = 0;
		AVPacket* packet = NULL;
	};

	std::map<size_t, Packet> mPacket;

};

}
