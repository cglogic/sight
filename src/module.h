#pragma once

#include <thread>
#include <chrono>
#include <atomic>

#include <nlohmann/json.hpp>

namespace Sight {

using json = nlohmann::json;

class Module {
public:
	Module(const json& config,
	       size_t id);
	Module(const Module& other) = delete;
	Module(Module&& other) noexcept;
	virtual ~Module();

	void run();
	bool running() const;
	void terminate();
	void wait();

	static bool validate(const json& config);

protected:
	virtual bool start();
	virtual void stop();
	virtual void task();

	bool active();
	void deactivate();

	size_t mId = 0;
	std::string mName;
	std::string mType;

	static uint32_t pack(uint16_t stream, uint8_t slot, bool flag);
	static void unpack(uint32_t packed, uint16_t& stream, uint8_t& slot, bool& flag);

private:
	std::atomic_flag mRun = ATOMIC_FLAG_INIT;
	std::atomic_flag mFinished = ATOMIC_FLAG_INIT;

	void worker();
	std::thread mWorker;

};

}
