/**
 * @file NeuralNetworkBuilder.hpp
 * @brief Builder pattern for constructing neural networks
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_CORE_NEURALNETWORKBUILDER_HPP
#define NEURAX_CORE_NEURALNETWORKBUILDER_HPP

#include "network/INetwork.hpp"
#include "layers/ILayer.hpp"
#include "graph/Graph.hpp"
#include <string>

namespace neurax {
namespace core {

class ILayer;

/**
 * @brief Builder for constructing neural networks
 * 
 * Supports both:
 * - Legacy linear layer API: addLayer()
 * - Graph-based API: addNode() with explicit tensor connections
 */
class NeuralNetworkBuilder {
private:
    std::unique_ptr<INetwork> network_;
    std::unique_ptr<graph::Graph> graph_;

    neurax::hal::AcceleratorType accelerator_type_;
    std::unique_ptr<neurax::hal::IAccelerator> accelerator_;

    bool use_graph_mode_;
    int node_counter_;

public:
    NeuralNetworkBuilder();

    // ==================== Legacy Linear API ====================

    /**
     * @brief Add a layer to the network (linear execution)
     * @param layer Layer to add (ownership transferred)
     * @return Reference to this builder for chaining
     */
    NeuralNetworkBuilder& addLayer(ILayer* layer);

    // ==================== Graph-based API ====================

    /**
     * @brief Add a node to the computation graph
     * @param layer Layer/operation to execute (ownership transferred)
     * @param inputs Names of input tensors
     * @param outputs Names of output tensors
     * @param name Optional node name (auto-generated if empty)
     * @return Reference to this builder for chaining
     */
    NeuralNetworkBuilder& addNode(
        std::unique_ptr<ILayer> layer,
        const std::vector<std::string>& inputs,
        const std::vector<std::string>& outputs,
        const std::string& name = "");

    /**
     * @brief Add a node using raw pointer (ownership transferred)
     * @param layer Layer to add
     * @param inputs Names of input tensors
     * @param outputs Names of output tensors
     * @param name Optional node name
     * @return Reference to this builder for chaining
     */
    NeuralNetworkBuilder& addNode(
        ILayer* layer,
        const std::vector<std::string>& inputs,
        const std::vector<std::string>& outputs,
        const std::string& name = "");

    /**
     * @brief Register tensor metadata in the graph
     * @param name Tensor name
     * @param shape Tensor shape
     * @param dtype Data type
     * @param constant Whether tensor is constant (weights/biases)
     * @return Reference to this builder for chaining
     */
    NeuralNetworkBuilder& registerTensor(
        const std::string& name,
        const tensor::Shape& shape,
        tensor::DataType dtype = tensor::DataType::FLOAT32,
        bool constant = false);

    /**
     * @brief Set graph input tensor names
     * @param input_names Names of tensors that are graph inputs
     * @return Reference to this builder for chaining
     */
    NeuralNetworkBuilder& setInputs(const std::vector<std::string>& input_names);

    /**
     * @brief Set graph output tensor names
     * @param output_names Names of tensors that are graph outputs
     * @return Reference to this builder for chaining
     */
    NeuralNetworkBuilder& setOutputs(const std::vector<std::string>& output_names);

    // ==================== Common API ====================

    /**
     * @brief Set the hardware accelerator type
     * @param type Accelerator type to use
     * @return Reference to this builder for chaining
     */
    NeuralNetworkBuilder& useAccelerator(neurax::hal::AcceleratorType type);

    /**
     * @brief Enable or disable graph execution mode
     * @param enable true to use graph execution
     * @return Reference to this builder for chaining
     */
    NeuralNetworkBuilder& useGraphExecution(bool enable = true);

    /**
     * @brief Build the neural network
     * @return Unique pointer to the constructed network
     */
    std::unique_ptr<INetwork> build();
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_NEURALNETWORKBUILDER_HPP