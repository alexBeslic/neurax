
#ifndef CONV2D_LAYER_CPP
#define CONV2D_LAYER_CPP

#include "Conv2dLayer.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"
#include <vector>

namespace neurax {
namespace core {

using namespace ::neurax::hal;

Conv2dLayer::Conv2dLayer(int inChannels, int outChannels, int kernelSize, int stride, int padding){
    config_ = std::make_unique<neurax::hal::ConvolutionConfig>();
    config_->input_channels = inChannels;
    config_->output_channels = outChannels;
    config_->kernel_size = kernelSize;
    config_->stride = stride;
    config_->padding = padding;
}

void Conv2dLayer::forward(const Tensor& input, Tensor& output) {
    // Implement the forward pass for the Conv2d layer
    if (!accelerator_) {
        throw std::runtime_error("No accelerator assigned to Conv2dLayer");
    }
    output = accelerator_->convolution(input, weights_, bias_, *config_);
}

Tensor Conv2dLayer::forward(const Tensor& input) {
    // Implement the forward pass for the Conv2d layer
    if (!accelerator_) {
        throw std::runtime_error("No accelerator assigned to Conv2dLayer");
    }
    return accelerator_->convolution(input, weights_, bias_, *config_);
}

void Conv2dLayer::loadWeights(const Tensor& weights, const Tensor& bias) {
    this->weights_=std::move(weights);
    this->bias_=std::move(bias);
}

void Conv2dLayer::addAccelerator(neurax::hal::IAccelerator* accelerator) {
    this->accelerator_ = accelerator;
}

Conv2dLayer::~Conv2dLayer() {
}

} // namespace core
} // namespace neurax
#endif // CONV2D_LAYER_HPP