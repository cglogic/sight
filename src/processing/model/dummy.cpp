#include "dummy.h"

#include <glog/logging.h>

namespace Sight::Model {

Dummy::Dummy() {
}

Dummy::Dummy(Dummy&& other) {
}

Dummy::~Dummy() {
}

bool Dummy::validate(const json& config) {
	return true;
}

bool Dummy::load() {
	return true;
}

bool Dummy::process(const AVFrame& image) {
	return true;
}

bool Dummy::preprocess(const AVFrame& image) {
	return true;
}

bool Dummy::postprocess(const AVFrame& image) {
	return true;
}

}
