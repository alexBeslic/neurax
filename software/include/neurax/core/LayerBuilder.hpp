// NeuralNetworkBuilder
#ifndef NEURAX_CORE_LAYERBUILDER_HPP
#define NEURAX_CORE_LAYERBUILDER_HPP

#include "layers/ILayer.hpp"

namespace neurax {
namespace core {

class ILayer;

class LayerBuilder {
private:
    ILayer* layer_;
public:

    LayerBuilder& conv2d();


    LayerBuilder& addWeights(const Tensor& weights, const Tensor& bias);
    ILayer* build() { return layer_; }

};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_NEURALNETWORKBUILDER_HPP