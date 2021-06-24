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
	Disk(Disk&& other) noexcept;
	~Disk();

	static bool validate(const json& config);

protected:
	bool start() override;
	bool send(Slot& slot) override;

private:
	fs::path mPath;

};

}
