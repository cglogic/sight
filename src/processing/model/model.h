#pragma once

#include "dummy.h"

extern "C" {
#	include <libavcodec/avcodec.h>
#	include <libavformat/avformat.h>
#	include <libavutil/imgutils.h>
#	include <libswscale/swscale.h>
}

#include <nlohmann/json.hpp>

#include <mxnet/c_api.h>
#include <mxnet/tuple.h>
#include <mxnet-cpp/MxNetCpp.h>
#include <mxnet-cpp/initializer.h>

namespace Sight::Model {

using json = nlohmann::json;

class Model
	: Dummy {
public:
	Model();
	Model(const Model& other) = delete;
	Model(Model&& other);
	virtual ~Model();

	static bool validate(const json& config);

	bool load() override;
	bool process(const AVFrame& image) override;

protected:
	bool preprocess(const AVFrame& image) override;
	bool postprocess(const AVFrame& image) override;

};

}
