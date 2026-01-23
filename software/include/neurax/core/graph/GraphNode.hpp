/**
 * @file GraphNode.hpp
 * @brief Represents a single operation node in the execution graph
 *
 * @author NEURAX Development Team
 * @date January 2026
 * @version 2.0
 */

#ifndef NEURAX_CORE_GRAPH_NODE_HPP
#define NEURAX_CORE_GRAPH_NODE_HPP

#include "neurax/core/layers/ILayer.hpp"
#include <string>
#include <vector>
#include <memory>

namespace neurax {
namespace core {
namespace graph {

/**
 * @brief Represents a single operation node in the execution graph
 * 
 * Each node wraps an ILayer and specifies its input/output tensor connections.
 * Supports multiple inputs (e.g., for Add, Concat) and multiple outputs.
 */
struct GraphNode {
    std::string name;                           ///< Unique node identifier
    std::unique_ptr<ILayer> layer;              ///< The operation/layer to execute
    
    std::vector<std::string> input_tensors;     ///< Names of input tensors
    std::vector<std::string> output_tensors;    ///< Names of output tensors

    /**
     * @brief Default constructor
     */
    GraphNode() = default;
    
    /**
     * @brief Parameterized constructor
     * @param node_name Unique name for this node
     * @param op Layer/operation to execute
     */
    GraphNode(const std::string& node_name, std::unique_ptr<ILayer> op)
        : name(node_name)
        , layer(std::move(op)) 
    {}

    /**
     * @brief Full constructor with input/output specification
     * @param node_name Unique name for this node
     * @param op Layer/operation to execute
     * @param inputs Names of input tensors
     * @param outputs Names of output tensors
     */
    GraphNode(const std::string& node_name, 
              std::unique_ptr<ILayer> op,
              const std::vector<std::string>& inputs,
              const std::vector<std::string>& outputs)
        : name(node_name)
        , layer(std::move(op))
        , input_tensors(inputs)
        , output_tensors(outputs)
    {}

    /**
     * @brief Check if node is valid
     * @return true if node has a layer and at least one output
     */
    bool isValid() const {
        return layer != nullptr && !output_tensors.empty();
    }

    /**
     * @brief Get number of inputs
     */
    size_t numInputs() const { return input_tensors.size(); }

    /**
     * @brief Get number of outputs
     */
    size_t numOutputs() const { return output_tensors.size(); }
};

} // namespace graph
} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_GRAPH_NODE_HPP
