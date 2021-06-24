#include "pipeline.h"

#include <set>

#include <glog/logging.h>

#ifdef INPUT_STREAM
#	include "input/stream.h"
#endif
#ifdef PROCESSING_DETECT
#	include "processing/detect.h"
#endif
#ifdef OUTPUT_DISK
#	include "output/disk.h"
#endif
#ifdef OUTPUT_HTTP
#	include "output/http.h"
#endif

namespace Sight {

Pipeline::Pipeline(const json& config, size_t id) :
	Module(config, id) {
	// Create slots
	mSlot.reserve(config["input"].size());
	for (size_t id = 0; id < config["input"].size(); ++id) {
		mSlot.push_back(std::vector<Slot>());
		std::string streamName = config["input"][id]["name"];
		size_t slotCount = slotSize(config, config["input"][id]);
		size_t stages = stageCount(config, config["input"][id]);
		mSlot[id].reserve(slotCount + 1);
		for (size_t slotId = 0; slotId < slotCount + 1; ++slotId) {
			mSlot[id].push_back(Slot(id, streamName, stages));
		}
	}

	// Create queues
	size_t queueCount = config["processing"].size() + config["output"].size();
	mQueue.reserve(queueCount);
	for (size_t id = 0; id < queueCount; ++id) {
		mQueue.push_back(Queue<uint32_t>());
	}

	// Create inputs
	mInput.reserve(config["input"].size());
	for (size_t id = 0; id < config["input"].size(); ++id) {
		auto& input = config["input"][id];
		std::vector<size_t> queueId(queueIds(config, input));
		if (input["type"] == "dummy") {
			mInput.push_back(std::make_unique<Input::Dummy>(input, id, mSlot[id], mQueue, queueId));
#ifdef INPUT_STREAM
		} else if (input["type"] == "stream") {
			mInput.push_back(std::make_unique<Input::Stream>(input, id, mSlot[id], mQueue, queueId));
#endif
		}
	}

	// Create processors
	mProcessing.reserve(config["processing"].size());
	for (size_t id = 0; id < config["processing"].size(); ++id) {
		auto& processing = config["processing"][id];
		std::vector<size_t> queueId(queueIds(config, processing));
		if (processing["type"] == "dummy") {
			mProcessing.push_back(std::make_unique<Processing::Dummy>
			                      (processing, id, mSlot, mQueue[config["output"].size() + id], mQueue, queueId));
#ifdef PROCESSING_DETECT
		} else if (processing["type"] == "detect") {
			mProcessing.push_back(std::make_unique<Processing::Detect>
			                      (processing, id, mSlot, mQueue[config["output"].size() + id], mQueue, queueId));
#endif
		}
	}

	// Create optputs
	mOutput.reserve(config["output"].size());
	for (size_t id = 0; id < config["output"].size(); ++id) {
		auto& output = config["output"][id];
		if (output["type"] == "dummy") {
			mOutput.push_back(std::make_unique<Output::Dummy>(output, id, mSlot, mQueue[id]));
#ifdef OUTPUT_DISK
		} else if (output["type"] == "disk") {
			mOutput.push_back(std::make_unique<Output::Disk>(output, id, mSlot, mQueue[id]));
#endif
#ifdef OUTPUT_HTTP
		} else if (output["type"] == "http") {
			mOutput.push_back(std::make_unique<Output::Http>(output, id, mSlot, mQueue[id]));
#endif
		}
	}
}

Pipeline::Pipeline(Pipeline&& other) noexcept :
	Module(std::move(other)),
	mSlot(std::move(other.mSlot)),
	mQueue(std::move(other.mQueue)),
	mInput(std::move(other.mInput)),
	mProcessing(std::move(other.mProcessing)),
	mOutput(std::move(other.mOutput)) {
}

Pipeline::~Pipeline() {
	mInput.clear();
	mProcessing.clear();
	mOutput.clear();

	mQueue.clear();
	mSlot.clear();
}

bool Pipeline::validate(const json& config) {
	if (!Module::validate(config)) {
		return false;
	}

	// Validate inputs
	std::set<std::string> inputUnique;
	if (!config.contains("input") || !config["input"].is_array() || config["input"].empty()) {
		LOG(ERROR) << "Input does not exist, not array or empty";
		return false;
	}
	for (size_t id = 0; id < config["input"].size(); ++id) {
		if (!config["input"][id].is_object()) {
			LOG(ERROR) << "Input is not an object, id = " << id;
			return false;
		}
		auto& input = config["input"][id];

		// Validate nodes itself
		if (!Module::validate(input)) {
			return false;
		}
		if (input["type"] == "dummy") {
			if (!Input::Dummy::validate(input)) {
				return false;
			}
#ifdef INPUT_STREAM
		} else if (input["type"] == "stream") {
			if (!Input::Stream::validate(input)) {
				return false;
			}
#endif
		} else {
			LOG(ERROR) << "Unknown input type = " << input["type"];
			return false;
		}

		// Validate unique names
		std::string name = input["name"];
		if (!inputUnique.contains(name)) {
			inputUnique.insert(name);
		} else {
			LOG(ERROR) << "Input names are not unique, name = " << name;
			return false;
		}
	}

	// Validate processing
	std::set<std::string> processingUnique;
	if (!config.contains("processing") || !config["processing"].is_array() || config["processing"].empty()) {
		LOG(ERROR) << "Processing does not exist, not array or empty";
		return false;
	}
	for (size_t id = 0; id < config["processing"].size(); ++id) {
		if (!config["processing"][id].is_object()) {
			LOG(ERROR) << "Processing is not an object, id = " << id;
			return false;
		}
		auto& processing = config["processing"][id];

		// Validate nodes itself
		if (!Module::validate(processing)) {
			return false;
		}
		if (processing["type"] == "dummy") {
			if (!Processing::Dummy::validate(processing)) {
				return false;
			}
#ifdef PROCESSING_DETECT
		} else if (processing["type"] == "detect") {
			if (!Processing::Detect::validate(processing)) {
				return false;
			}
#endif
		} else {
			LOG(ERROR) << "Unknown processing type = " << processing["type"];
			return false;
		}

		// Validate unique names
		std::string name = processing["name"];
		if (!processingUnique.contains(name)) {
			processingUnique.insert(name);
		} else {
			LOG(ERROR) << "Processing names are not unique, name = " << name;
			return false;
		}
	}

	// Validate output
	std::set<std::string> outputUnique;
	if (!config.contains("output") || !config["output"].is_array() || config["output"].empty()) {
		LOG(ERROR) << "Output does not exist, not array or empty";
		return false;
	}
	for (size_t id = 0; id < config["output"].size(); ++id) {
		if (!config["output"][id].is_object()) {
			LOG(ERROR) << "Output is not an object, id = " << id;
			return false;
		}
		auto& output = config["output"][id];

		// Validate nodes itself
		if (!Module::validate(output)) {
			return false;
		}
		if (output["type"] == "dummy") {
			if (!Output::Dummy::validate(output)) {
				return false;
			}
#ifdef OUTPUT_DISK
		} else if (output["type"] == "disk") {
			if (!Output::Disk::validate(output)) {
				return false;
			}
#endif
#ifdef OUTPUT_HTTP
		} else if (output["type"] == "http") {
			if (!Output::Http::validate(output)) {
				return false;
			}
#endif
		} else {
			LOG(ERROR) << "Unknown output type = " << output["type"];
			return false;
		}

		// Validate unique names
		std::string name = output["name"];
		if (!outputUnique.contains(name)) {
			outputUnique.insert(name);
		} else {
			LOG(ERROR) << "Output names are not unique, name = " << name;
			return false;
		}
	}

	// Validate unique nodes name
	std::set<std::string> nodeUnique;
	nodeUnique.insert(processingUnique.begin(), processingUnique.end());
	nodeUnique.insert(inputUnique.begin(), inputUnique.end());
	nodeUnique.insert(outputUnique.begin(), outputUnique.end());
	if (nodeUnique.size() != inputUnique.size() + processingUnique.size() + outputUnique.size()) {
		LOG(ERROR) << "Node names are not unique";
		return false;
	}

	// Validate that all nodes connected
	std::set<std::string> processingConnected;
	std::set<std::string> outputConnected;
	for (size_t id = 0; id < config["input"].size(); ++id) {
		auto input = config["input"][id];
		auto connection = input["out"];
		for (size_t outId = 0; outId < connection.size(); ++outId) {
			std::string name = connection[outId];
			if (processingUnique.contains(name)) {
				processingConnected.insert(name);
			} else {
				LOG(ERROR) << "Input node not connected = " << input["name"]
				           << ", out = " << name;
				return false;
			}
		}
	}
	for (size_t id = 0; id < config["processing"].size(); ++id) {
		auto processing = config["processing"][id];
		auto connection = processing["out"];
		for (size_t outId = 0; outId < connection.size(); ++outId) {
			std::string name = connection[outId];
			if (processingUnique.contains(name)) {
				processingConnected.insert(name);
			} else if (outputUnique.contains(name)) {
				outputConnected.insert(name);
			} else {
				LOG(ERROR) << "Processing node not connected = " << processing["name"]
				           << ", out = " << name;
				return false;
			}
		}
	}
	if (config["processing"].size() != processingConnected.size() ||
	    config["output"].size() != outputConnected.size()) {
		LOG(ERROR) << "There is not connected output";
		return false;
	}

	// TODO: Detect cycles

	return true;
}

bool Pipeline::start() {
	for (auto& output : mOutput) {
		output->run();
	}

	for (auto& processing : mProcessing) {
		processing->run();
	}

	for (auto& input : mInput) {
		input->run();
	}

	return true;
}

void Pipeline::stop() {
	for (auto& input : mInput) {
		input->terminate();
	}
	for (auto& input : mInput) {
		input->wait();
	}

	for (auto& processing : mProcessing) {
		processing->terminate();
	}
	for (auto& processing : mProcessing) {
		processing->wait();
	}

	for (auto& output : mOutput) {
		output->terminate();
	}
	for (auto& output : mOutput) {
		output->wait();
	}
}

void Pipeline::task() {
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	size_t finished = 0;
	for (auto& input : mInput) {
		if (!input->running()) {
			++finished;
		}
	}
	if (finished == mInput.size()) {
		deactivate();
	}
}

size_t Pipeline::slotSize(const json& config, const json& node) {
	size_t count = 0;
	for (size_t outId = 0; outId < node["out"].size(); ++outId) {
		auto& name = node["out"][outId];
		for (size_t id = 0; id < config["output"].size(); ++id) {
			auto& output = config["output"][id];
			if (output["name"] == name) {
				count = 1;
				break;
			}
		}
		for (size_t id = 0; id < config["processing"].size(); ++id) {
			auto& processor = config["processing"][id];
			if (processor["name"] == name) {
				size_t path = slotSize(config, processor) + 1;
				if (path > count) {
					count = path;
				}
			}
		}
	}
	return count;
}

size_t Pipeline::stageCount(const json& config, const json& node) {
	size_t count = 0;
	for (size_t outId = 0; outId < node["out"].size(); ++outId) {
		auto& name = node["out"][outId];
		for (size_t id = 0; id < config["processing"].size(); ++id) {
			auto& processor = config["processing"][id];
			if (processor["name"] == name) {
				++count += stageCount(config, processor);
				break;
			}
		}
		for (size_t id = 0; id < config["output"].size(); ++id) {
			auto& output = config["output"][id];
			if (output["name"] == name) {
				++count;
				break;
			}
		}
	}
	return count;
}

std::vector<size_t> Pipeline::queueIds(const json& config, const json& node) {
	std::vector<size_t> result;
	for (size_t outId = 0; outId < node["out"].size(); ++outId) {
		auto& outName = node["out"][outId];
		for (size_t id = 0; id < config["output"].size(); ++id) {
			auto& output = config["output"][id];
			if (output["name"] == outName) {
				result.push_back(id);
			}
		}
		for (size_t id = 0; id < config["processing"].size(); ++id) {
			auto& processor = config["processing"][id];
			if (processor["name"] == outName) {
				result.push_back(config["output"].size() + id);
			}
		}
	}
	return result;
}

}
