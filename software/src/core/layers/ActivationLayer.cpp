#ifndef ACTIVATION_LAYER_CPP
#define ACTIVATION_LAYER_CPP

#include "ActivationLayer.hpp"
#include <cmath>

namespace neurax {
namespace core {

using namespace ::neurax::hal;

ActivationLayer::ActivationLayer() : ActivationLayer(neurax::hal::ActivationType::RELU) {}

ActivationLayer::ActivationLayer(neurax::hal::ActivationType type) : accelerator_(nullptr), type_(type) {}

ActivationLayer::~ActivationLayer() {}

void ActivationLayer::forward(const Tensor& input, Tensor& output) {
    if (!accelerator_) {
        // Software fallback using Tensor API
        output = Tensor::empty(input.shape(), input.dtype());
        size_t n = input.numel();
        const float* inptr = input.data_ptr<float>();
        float* outptr = output.data_ptr<float>();
        for (size_t i = 0; i < n; ++i) {
            float v = inptr[i];
            switch (type_) {
                case ActivationType::RELU:
                    outptr[i] = v > 0.0f ? v : 0.0f;
                    break;
                case ActivationType::TANH:
                    outptr[i] = std::tanh(v);
                    break;
                case ActivationType::SIGMOID:
                    outptr[i] = 1.0f / (1.0f + std::exp(-v));
                    break;
                case ActivationType::LINEAR:
                default:
                    outptr[i] = v;
            }
        }
        return;
    }

    output = accelerator_->activation(input, type_);
}

Tensor ActivationLayer::forward(const Tensor& input) {
    if (!accelerator_) {
        Tensor out;
        forward(input, out);
        return out;
    }
    return accelerator_->activation(input, type_);
}

void ActivationLayer::addAccelerator(neurax::hal::IAccelerator* accelerator) {
    this->accelerator_ = accelerator;
}

void ActivationLayer::setType(neurax::hal::ActivationType type) {
    type_ = type;
}

} // namespace core
} // namespace neurax

#endif // ACTIVATION_LAYER_CPP
