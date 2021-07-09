#pragma once

extern "C" {
#	include <libavcodec/avcodec.h>
#	include <libavformat/avformat.h>
#	include <libavutil/imgutils.h>
#	include <libswscale/swscale.h>
}

#include <nlohmann/json.hpp>

namespace Sight::Model {

using json = nlohmann::json;

class Dummy {
public:
	Dummy(const json& config);
	Dummy(const Dummy& other) = delete;
	Dummy(Dummy&& other);
	virtual ~Dummy();

	static bool validate(const json& config);

protected:

};

}
