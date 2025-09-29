
#ifndef CONV2D_LAYER_CPP
#define CONV2D_LAYER_CPP

#include "Conv2dLayer.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"
#include <vector>

namespace neurax {
namespace core {

using namespace ::neurax::hal;


Conv2dLayer::Conv2dLayer() : accelerator_(nullptr)
                            ,config_(std::make_unique<neurax::hal::ConvolutionConfig>()) {
    // Default configuration
}

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

void Conv2dLayer::setInputChannels(int channels) {
    config_->input_channels = channels;
}

void Conv2dLayer::setOutputChannels(int channels) {
    config_->output_channels = channels;
}

void Conv2dLayer::setKernelSize(int size) {
    config_->kernel_size = size;
}

void Conv2dLayer::setStride(int stride) {
    config_->stride = stride;
}

void Conv2dLayer::setPadding(int padding) {
    config_->padding = padding;
}

Conv2dLayer::~Conv2dLayer() {
}

} // namespace core
} // namespace neurax
#endif // CONV2D_LAYER_HPP