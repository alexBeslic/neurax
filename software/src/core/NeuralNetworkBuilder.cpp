

#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "network/NeuralNetwork.hpp"
#include "neurax/core/layers/ILayer.hpp"

namespace neurax {
namespace core {

NeuralNetworkBuilder::NeuralNetworkBuilder() : network_(new NeuralNetwork) {}

NeuralNetworkBuilder& NeuralNetworkBuilder::addLayer(ILayer* layer) {
    layer->addAccelerator(network_->getAccelerator());

    network_->addLayer(std::unique_ptr<ILayer>(layer));
    return *this;
}

NeuralNetworkBuilder& NeuralNetworkBuilder::addAccelerator(neurax::hal::IAccelerator* accelerator) {
    network_->addAccelerator(accelerator);
    return *this;
}

} // namespace core
} // namespace neurax