#include "BiasAddLayer.hpp"

namespace neurax {
namespace core {

BiasAddLayer::BiasAddLayer() : accelerator_(nullptr) {}

BiasAddLayer::~BiasAddLayer() {}

void BiasAddLayer::forward(const Tensor& input, Tensor& output) {
    if (accelerator_ == nullptr) {
        throw std::runtime_error("BiasAddLayer: No accelerator set");
    }
    output = accelerator_->bias_add(input, bias_);
}

Tensor BiasAddLayer::forward(const Tensor& input) {
    if (accelerator_ == nullptr) {
        throw std::runtime_error("BiasAddLayer: No accelerator set");
    }
    return accelerator_->bias_add(input, bias_);
}

void BiasAddLayer::addAccelerator(neurax::hal::IAccelerator* accelerator) {
    accelerator_ = accelerator;
}

void BiasAddLayer::setBias(const tensor::Tensor& bias) {
    bias_ = bias;
}

} // namespace core
} // namespace neurax
