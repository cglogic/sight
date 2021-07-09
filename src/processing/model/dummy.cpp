#include "dummy.h"

#include <glog/logging.h>

namespace Sight::Model {

Dummy::Dummy(const json& config) {
}

Dummy::Dummy(Dummy&& other) {
}

Dummy::~Dummy() {
}

bool Dummy::validate(const json& config) {
	return true;
}

}
