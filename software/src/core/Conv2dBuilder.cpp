
#include "neurax/core/Conv2dBuilder.hpp"
#include "layers/Conv2dLayer.hpp"

namespace neurax {
namespace core {

Conv2dBuilder::Conv2dBuilder() {
    layer_ = new Conv2dLayer();
}

Conv2dBuilder& Conv2dBuilder::addWeights(const Tensor& weights, const Tensor& bias) {
    layer_->loadWeights(weights, bias);
    return *this;
}

Conv2dBuilder& Conv2dBuilder::inputChanels(size_t channels) {
    layer_->setInputChannels(static_cast<int>(channels));
    return *this;
}
Conv2dBuilder& Conv2dBuilder::outputChanels(size_t channels) {
    layer_->setOutputChannels(static_cast<int>(channels));
    return *this;
}
Conv2dBuilder& Conv2dBuilder::kernelSize(size_t size) {
    layer_->setKernelSize(static_cast<int>(size));
    return *this;
}
Conv2dBuilder& Conv2dBuilder::stride(size_t stride) {
    layer_->setStride(static_cast<int>(stride));
    return *this;
}
Conv2dBuilder& Conv2dBuilder::padding(size_t padding) {
    layer_->setPadding(static_cast<int>(padding));
    return *this;
}

ILayer* Conv2dBuilder::build() {
    return layer_;
}

} // namespace core
} // namespace neurax