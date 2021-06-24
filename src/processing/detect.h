#pragma once

#include "dummy.h"

namespace Sight::Processing {

class Detect
	: public Dummy {
public:
	Detect(const json& config,
	       size_t id,
	       std::vector<std::vector<Slot>>& slot,
	       Queue<uint32_t>& queueIn,
	       std::vector<Queue<uint32_t>>& queueOut,
	       std::vector<size_t>& queueOutId);
	Detect(const Detect& other) = delete;
	Detect(Detect&& other) noexcept;
	~Detect();

	static bool validate(const json& config);

protected:
	bool detect(Slot& slot) override;

};

}
