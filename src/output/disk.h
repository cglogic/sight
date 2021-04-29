#pragma once

#include "dummy.h"

#include <filesystem>

namespace Sight::Output {

namespace fs = std::filesystem;

class Disk
	: public Dummy {
public:
	Disk(const json& config,
	     size_t id,
	     std::vector<std::vector<Slot>>& slot,
	     Queue<uint32_t>& queue);
	Disk(const Disk& other) = delete;
	Disk(Disk&& other);
	virtual ~Disk();

	static bool validate(const json& config);

protected:
	virtual bool start();
	virtual bool send(Slot& slot);

private:
	fs::path mPath;

};

}
