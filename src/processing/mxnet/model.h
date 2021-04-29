#pragma once

extern "C" {
#	include <libavcodec/avcodec.h>
#	include <libavformat/avformat.h>
#	include <libavutil/imgutils.h>
#	include <libswscale/swscale.h>
}

#include <nlohmann/json.hpp>

namespace Sight::Processing {

using json = nlohmann::json;

class Model {
public:
	Model();
	Model(const Model& other) = delete;
	Model(Model&& other);
	virtual ~Model();

	static bool validate(const json& config);

	virtual bool load();
	virtual bool detect(const AVFrame& image);

protected:
	virtual bool preprocess(const AVFrame& image);
	virtual bool postprocess(const AVFrame& image);

};

}
