#include "model.h"

#include <glog/logging.h>

namespace Sight::Model {

Model::Model() {
}

Model::Model(Model&& other) {
}

Model::~Model() {
}

bool Model::validate(const json& config) {
	return true;
}

bool Model::load() {
	return true;
}

bool Model::process(const AVFrame& image) {
	return true;
}

bool Model::preprocess(const AVFrame& image) {
	return true;
}

bool Model::postprocess(const AVFrame& image) {
	return true;
}

}
