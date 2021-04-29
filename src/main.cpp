#include <cstdlib>
#include <csignal>
#include <list>
#include <string>
#include <fstream>
#include <streambuf>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "pipeline.h"

using namespace Sight;

DEFINE_string(config, "./config.json", "path to config file");

volatile static std::sig_atomic_t gSignal = 0;

static void signalHandler(int signal) {
	if (signal == SIGINT) {
		LOG(INFO) << "Terminate";
		gSignal = signal;
	}
}

int main(int argc, char** argv) {
	google::InitGoogleLogging(argv[0]);
	google::InstallFailureSignalHandler();
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	LOG(INFO) << "Started";

	LOG(INFO) << "Using config file: " << FLAGS_config;
	std::ifstream file(FLAGS_config);
	std::string input, err;
	if (!file.is_open()) {
		LOG(ERROR) << "Can't open config file";
		return EXIT_SUCCESS;
	}

	file.seekg(0, std::ios::end);
	input.reserve(file.tellg());
	file.seekg(0, std::ios::beg);
	input.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	if (file.fail()) {
		LOG(ERROR) << "Can't open config file";
		return EXIT_SUCCESS;
	}

	json config;
	try {
		config = json::parse(input);
	} catch (json::parse_error& e) {
		LOG(ERROR) << "JSON parse error: " << e.what();
		return EXIT_SUCCESS;
	}
	// LOG(INFO) << "Loaded config: \n" << config.dump(4);

	std::list<std::unique_ptr<Pipeline>> pipeline;
	if (config["pipeline"].is_array()) {
		for (size_t id = 0; id < config["pipeline"].size(); ++id) {
			if (config["pipeline"][id].is_object()) {
				auto pipe = config["pipeline"][id];
				if (Pipeline::validate(pipe)) {
					pipeline.push_back(std::make_unique<Pipeline>(pipe, id));
				} else {
					LOG(ERROR) << "Incorrect pipeline config, pipeline = " << id;
				}
			} else {
				LOG(ERROR) << "Pipeline config must be a dictionary, pipeline = " << id;
			}
		}
	} else {
		LOG(ERROR) << "Config has no pipelines";
		return EXIT_SUCCESS;
	}

	for (auto& p : pipeline) {
		p->run();
	}

	std::signal(SIGINT, signalHandler);
	while (gSignal == 0 && !pipeline.empty()) {
		auto it = pipeline.begin();
		while (it != pipeline.end()) {
			if (!(*it)->running()) {
				(*it)->terminate();
				(*it)->wait();
				break;
			}
			++it;
		}
		if (it != pipeline.end()) {
			pipeline.erase(it);
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	for (auto& p : pipeline) {
		p->terminate();
	}
	for (auto& p : pipeline) {
		p->wait();
	}
	pipeline.clear();

	LOG(INFO) << "Stopped";
	return EXIT_SUCCESS;
}
