

#ifndef POOLING_LAYER_CPP
#define POOLING_LAYER_CPP

#include "PoolingLayer.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"
#include <vector>

namespace neurax {
namespace core {

using namespace ::neurax::hal;


PoolingLayer::PoolingLayer() : accelerator_(nullptr)
                            ,config_(std::make_unique<neurax::hal::PoolingConfig>()) {
    // Default configuration
}
PoolingLayer::~PoolingLayer() {
}

void PoolingLayer::forward(const Tensor& input, Tensor& output) {
    // Implement the forward pass for the Conv2d layer
    if (!accelerator_) {
        throw std::runtime_error("No accelerator assigned to PoolingLayer");
    }
    output = accelerator_->pooling(input, *config_);
}

Tensor PoolingLayer::forward(const Tensor& input) {
    // Implement the forward pass for the Conv2d layer
    if (!accelerator_) {
        throw std::runtime_error("No accelerator assigned to PoolingLayer");
    }
    return accelerator_->pooling(input, *config_);
}

void PoolingLayer::addAccelerator(neurax::hal::IAccelerator* accelerator) {
    this->accelerator_ = accelerator;
}

void PoolingLayer::setPoolSize(size_t size) {
    config_->pool_size = size;
}
void PoolingLayer::setStride(size_t stride) {
    config_->stride = stride;
}
void PoolingLayer::setType(neurax::hal::PoolingType type) {
    config_->type = type;
}

} // namespace core
} // namespace neurax
#endif // CONV2D_LAYER_HPP