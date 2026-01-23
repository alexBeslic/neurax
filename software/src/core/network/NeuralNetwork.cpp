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
    return accelerator_.get();
}

void NeuralNetwork::addAccelerator(std::unique_ptr<neurax::hal::IAccelerator> accelerator) {
    accelerator_ = std::move(accelerator);
    
    // Add accelerator to linear layers
    for (auto& layer : layers_) {
        layer->addAccelerator(accelerator_.get());
    }
    
    // Update graph executor if it exists
    if (executor_) {
        executor_->setAccelerator(accelerator_.get());
    }
}

void NeuralNetwork::setGraph(std::unique_ptr<graph::Graph> g) {
    graph_ = std::move(g);
    
    if (graph_) {
        executor_ = std::make_unique<execution::GraphExecutor>(
            graph_.get(), accelerator_.get());
        
        if (!executor_->buildSchedule()) {
            throw std::runtime_error("Failed to build graph schedule - possible cycle detected");
        }
    } else {
        executor_.reset();
    }
}

void NeuralNetwork::enableGraphExecution(bool enable) {
    if (enable && !graph_) {
        throw std::runtime_error("Cannot enable graph execution without a graph");
    }
    use_graph_execution_ = enable;
}

void NeuralNetwork::infer(const Tensor& input, Tensor& output) {
    if (use_graph_execution_ && executor_) {
        // Use graph execution with default input/output names
        std::unordered_map<std::string, Tensor> inputs;
        std::unordered_map<std::string, Tensor> outputs;
        
        // Use first graph input name, or "input" as default
        std::string input_name = graph_->input_names.empty() ? "input" : graph_->input_names[0];
        inputs[input_name] = input;
        
        executor_->run(inputs, outputs);
        
        // Get first output
        std::string output_name = graph_->output_names.empty() ? "output" : graph_->output_names[0];
        auto it = outputs.find(output_name);
        if (it != outputs.end()) {
            output = std::move(it->second);
        } else if (!outputs.empty()) {
            output = std::move(outputs.begin()->second);
        }
    } else {
        // Legacy linear execution
        const Tensor *current_input = &input;
        Tensor current_output;

        for (const auto& layer : layers_) {
            layer->forward(*current_input, current_output);
            current_input = &current_output;
        }

        output = current_output;
    }
}

Tensor NeuralNetwork::infer(const Tensor& input) {
    Tensor output;
    infer(input, output);
    return output;
}

void NeuralNetwork::infer(const std::unordered_map<std::string, Tensor>& inputs,
                          std::unordered_map<std::string, Tensor>& outputs) {
    if (!executor_) {
        throw std::runtime_error("Graph executor not initialized. Set a graph first.");
    }
    
    executor_->run(inputs, outputs);
}

NeuralNetwork::~NeuralNetwork() {
    if (accelerator_) {
        accelerator_->cleanup();
    }
}

} // namespace core
} // namespace neurax
