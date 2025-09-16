// NeuralNetworkBuilder
#ifndef NEURAX_CORE_NEURALNETWORKBUILDER_HPP
#define NEURAX_CORE_NEURALNETWORKBUILDER_HPP

#include "network/INetwork.hpp"
#include "layers/ILayer.hpp"

namespace neurax {
namespace core {

class ILayer;

class NeuralNetworkBuilder {
private:
    INetwork* network_;
public:

    NeuralNetworkBuilder(INetwork* network) : network_(network) {}
    NeuralNetworkBuilder();

    NeuralNetworkBuilder& addAccelerator(neurax::hal::IAccelerator* accelerator);

    NeuralNetworkBuilder& addLayer(ILayer* layer);

    INetwork* build() { return network_; }
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_NEURALNETWORKBUILDER_HPP