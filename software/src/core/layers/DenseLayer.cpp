#ifndef DENSE_LAYER_CPP
#define DENSE_LAYER_CPP

#include "DenseLayer.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

namespace neurax {
namespace core {

using namespace ::neurax::hal;

DenseLayer::DenseLayer() : accelerator_(nullptr), units_(0) {}

DenseLayer::DenseLayer(int units) : accelerator_(nullptr), units_(units) {}

DenseLayer::~DenseLayer() {}

void DenseLayer::forward(const Tensor& input, Tensor& output) {
    if (!accelerator_) {
        throw std::runtime_error("No accelerator assigned to DenseLayer");
    }
    output = accelerator_->dense(input, weights_, bias_);
}

Tensor DenseLayer::forward(const Tensor& input) {
    if (!accelerator_) {
        throw std::runtime_error("No accelerator assigned to DenseLayer");
    }
    return accelerator_->dense(input, weights_, bias_);
}

void DenseLayer::loadWeights(const Tensor& weights, const Tensor& bias) {
    this->weights_ = std::move(weights);
    this->bias_ = std::move(bias);
}

void DenseLayer::addAccelerator(neurax::hal::IAccelerator* accelerator) {
    this->accelerator_ = accelerator;
}

void DenseLayer::setUnits(int units) {
    units_ = units;
}

} // namespace core
} // namespace neurax

#endif // DENSE_LAYER_CPP
