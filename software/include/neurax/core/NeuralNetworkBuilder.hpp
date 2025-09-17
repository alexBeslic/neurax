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
    std::unique_ptr<INetwork> network_;

    neurax::hal::AcceleratorType accelerator_type_;
    std::unique_ptr<neurax::hal::IAccelerator> accelerator_;
public:

    NeuralNetworkBuilder();

    NeuralNetworkBuilder& addLayer(ILayer* layer);

    NeuralNetworkBuilder& useAccelerator(neurax::hal::AcceleratorType type);

    std::unique_ptr<INetwork> build();
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_NEURALNETWORKBUILDER_HPP