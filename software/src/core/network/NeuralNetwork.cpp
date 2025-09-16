/**
 * @file Network.cpp
 * @brief Implementation of the Network class
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "NeuralNetwork.hpp"
#include <vector>
#include <stdexcept>
namespace neurax {

namespace core {

void NeuralNetwork::addLayer(std::unique_ptr<ILayer> layer) {
    layers_.push_back(std::move(layer));
}

void NeuralNetwork::removeLayer(size_t index) {
    if (index < layers_.size()) {
        layers_.erase(layers_.begin() + index);
    }
}

std::unique_ptr<ILayer>& NeuralNetwork::getLayer(size_t index) {
    if (index >= layers_.size()) {
        throw std::out_of_range("Layer index out of range");
    }
    return layers_[index];
}

size_t NeuralNetwork::getLayerCount() const {
    return layers_.size();
}

neurax::hal::IAccelerator* NeuralNetwork::getAccelerator() {
    return accelerator_;
}


void NeuralNetwork::infer(const Tensor& input, Tensor& output){
    const Tensor *current_input = &input;
    Tensor current_output;

    for (const auto& layer : layers_) {
        layer->forward(*current_input, current_output);
        current_input = &current_output; // Output of current layer is input to next
    }

    output = current_output; // Final output
}


Tensor NeuralNetwork::infer(const Tensor& input){
    const Tensor *current_input = &input;
    Tensor current_output;

    for (const auto& layer : layers_) {
        layer->forward(*current_input, current_output);
        current_input = &current_output; // Output of current layer is input to next
    }

    return current_output; // Final output
}

} // namespace core
} // namespace neurax
