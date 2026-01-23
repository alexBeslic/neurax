/**
 * @file NeuralNetworkBuilder.cpp
 * @brief Implementation of the neural network builder
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "network/NeuralNetwork.hpp"
#include "neurax/core/layers/ILayer.hpp"
#include "neurax/hal/AcceleratorFactory.hpp"

namespace neurax {
namespace core {

NeuralNetworkBuilder::NeuralNetworkBuilder() 
    : network_(new NeuralNetwork)
    , graph_(std::make_unique<graph::Graph>())
    , accelerator_type_(hal::AcceleratorType::SOFTWARE_FALLBACK)
    , use_graph_mode_(false)
    , node_counter_(0)
{}

// ==================== Legacy Linear API ====================

NeuralNetworkBuilder& NeuralNetworkBuilder::addLayer(ILayer* layer) {
    network_->addLayer(std::unique_ptr<ILayer>(layer));
    return *this;
}

// ==================== Graph-based API ====================

NeuralNetworkBuilder& NeuralNetworkBuilder::addNode(
    std::unique_ptr<ILayer> layer,
    const std::vector<std::string>& inputs,
    const std::vector<std::string>& outputs,
    const std::string& name) {
    
    use_graph_mode_ = true;
    
    auto node = std::make_unique<graph::GraphNode>();
    node->name = name.empty() ? "node_" + std::to_string(node_counter_++) : name;
    node->layer = std::move(layer);
    node->input_tensors = inputs;
    node->output_tensors = outputs;
    
    graph_->addNode(std::move(node));
    return *this;
}

NeuralNetworkBuilder& NeuralNetworkBuilder::addNode(
    ILayer* layer,
    const std::vector<std::string>& inputs,
    const std::vector<std::string>& outputs,
    const std::string& name) {
    
    return addNode(std::unique_ptr<ILayer>(layer), inputs, outputs, name);
}

NeuralNetworkBuilder& NeuralNetworkBuilder::registerTensor(
    const std::string& name,
    const tensor::Shape& shape,
    tensor::DataType dtype,
    bool constant) {
    
    graph_->registerTensor(name, shape, dtype, constant);
    return *this;
}

NeuralNetworkBuilder& NeuralNetworkBuilder::setInputs(
    const std::vector<std::string>& input_names) {
    graph_->input_names = input_names;
    return *this;
}

NeuralNetworkBuilder& NeuralNetworkBuilder::setOutputs(
    const std::vector<std::string>& output_names) {
    graph_->output_names = output_names;
    return *this;
}

// ==================== Common API ====================

NeuralNetworkBuilder& NeuralNetworkBuilder::useAccelerator(
    neurax::hal::AcceleratorType type) {
    accelerator_type_ = type;
    return *this;
}

NeuralNetworkBuilder& NeuralNetworkBuilder::useGraphExecution(bool enable) {
    use_graph_mode_ = enable;
    return *this;
}

std::unique_ptr<INetwork> NeuralNetworkBuilder::build() {
    accelerator_ = neurax::hal::AcceleratorFactory::create(accelerator_type_);
    
    auto* nn = dynamic_cast<NeuralNetwork*>(network_.get());
    if (nn) {
        nn->addAccelerator(std::move(accelerator_));
        
        if (use_graph_mode_ && graph_ && graph_->numNodes() > 0) {
            nn->setGraph(std::move(graph_));
            nn->enableGraphExecution(true);
        }
    }
    
    return std::move(network_);
}

} // namespace core
} // namespace neurax