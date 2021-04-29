#pragma once

#include "module.h"

#include <list>
#include <vector>

#include "slot.h"
#include "queue.h"

#include "input/dummy.h"
#include "processing/dummy.h"
#include "output/dummy.h"

namespace Sight {

class Pipeline
	: public Module {
public:
	Pipeline(const json& config,
	         size_t id);
	Pipeline(const Pipeline& other) = delete;
	Pipeline(Pipeline&& other);
	virtual ~Pipeline();

	static bool validate(const json& config);

protected:
	virtual bool start();
	virtual void stop();
	virtual void task();

	static size_t slotSize(const json& config, const json& node);
	static size_t stageCount(const json& config, const json& node);
	static std::vector<size_t> queueIds(const json& config, const json& node);

private:
	std::vector<std::vector<Slot>> mSlot;
	std::vector<Queue<uint32_t>> mQueue;

	std::vector<std::unique_ptr<Input::Dummy>> mInput;
	std::vector<std::unique_ptr<Processing::Dummy>> mProcessing;
	std::vector<std::unique_ptr<Output::Dummy>> mOutput;

};

}
