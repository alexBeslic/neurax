
// NeuralNetworkBuilder
#ifndef NEURAX_CORE_CONV2DBUILDER_HPP
#define NEURAX_CORE_CONV2DBUILDER_HPP

#include "layers/ILayer.hpp"

namespace neurax {
namespace core {

class Conv2dLayer;

class Conv2dBuilder {
    friend class LayerBuilder;
    Conv2dLayer* layer_;

    Conv2dBuilder();
    Conv2dBuilder(const Conv2dBuilder&) = delete;
    Conv2dBuilder(Conv2dBuilder&&) = delete;
    Conv2dBuilder& operator=(const Conv2dBuilder&) = delete;

public:

    Conv2dBuilder& addWeights(const Tensor& weights, const Tensor& bias);
    ILayer* build();

    Conv2dBuilder& inputChanels(size_t channels);
    Conv2dBuilder& outputChanels(size_t channels);
    Conv2dBuilder& kernelSize(size_t size);
    Conv2dBuilder& stride(size_t stride);
    Conv2dBuilder& padding(size_t padding);

};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_CONV2DBUILDER_HPP