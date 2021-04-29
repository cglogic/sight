#pragma once

#include "dummy.h"

namespace Sight::Output {

class Http
	: public Dummy {
public:
	Http(const json& config,
	     size_t id,
	     std::vector<std::vector<Slot>>& slot,
	     Queue<uint32_t>& queue);
	Http(const Http& other) = delete;
	Http(Http&& other);
	virtual ~Http();

	static bool validate(const json& config);

protected:
	virtual bool send(Slot& slot);

private:
	std::string mUrl;
	std::string mToken;
	std::string mApi;

};

}
