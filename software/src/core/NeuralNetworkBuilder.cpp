

#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "network/NeuralNetwork.hpp"
#include "neurax/core/layers/ILayer.hpp"
#include "neurax/hal/AcceleratorFactory.hpp"

namespace neurax {
namespace core {

NeuralNetworkBuilder::NeuralNetworkBuilder() : network_(new NeuralNetwork) {}

NeuralNetworkBuilder& NeuralNetworkBuilder::addLayer(ILayer* layer) {
    network_->addLayer(std::unique_ptr<ILayer>(layer));
    return *this;
}

NeuralNetworkBuilder& NeuralNetworkBuilder::useAccelerator(neurax::hal::AcceleratorType type) {
    accelerator_type_ = type;
    return *this;
}

std::unique_ptr<INetwork> NeuralNetworkBuilder::build() {
    accelerator_ = neurax::hal::AcceleratorFactory::create(accelerator_type_);
    network_->addAccelerator(std::move(accelerator_));

    return std::move(network_);
}

} // namespace core
} // namespace neurax