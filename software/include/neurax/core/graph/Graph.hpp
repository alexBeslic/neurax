/**
 * @file Graph.hpp
 * @brief Container for the computation graph structure
 *
 * @author NEURAX Development Team
 * @date January 2026
 * @version 2.0
 */

#ifndef NEURAX_CORE_GRAPH_HPP
#define NEURAX_CORE_GRAPH_HPP

#include "GraphTensor.hpp"
#include "GraphNode.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>

namespace neurax {
namespace core {
namespace graph {

/**
 * @brief Container for the computation graph structure
 * 
 * Holds all nodes and tensor metadata for DAG-based execution.
 * Supports multiple inputs/outputs and skip connections.
 */
class Graph {
public:
    std::unordered_map<std::string, GraphTensor> tensors;   ///< Tensor metadata registry
    std::vector<std::unique_ptr<GraphNode>> nodes;          ///< Ordered list of nodes
    
    std::vector<std::string> input_names;   ///< Graph-level input tensor names
    std::vector<std::string> output_names;  ///< Graph-level output tensor names

    /**
     * @brief Default constructor
     */
    Graph() = default;

    /**
     * @brief Add a node to the graph
     * @param node Node to add (ownership transferred)
     * @return Pointer to the added node
     */
    GraphNode* addNode(std::unique_ptr<GraphNode> node) {
        if (!node) {
            throw std::invalid_argument("Cannot add null node to graph");
        }
        nodes.push_back(std::move(node));
        return nodes.back().get();
    }

    /**
     * @brief Register a tensor in the graph
     * @param tensor Tensor metadata to register
     */
    void registerTensor(const GraphTensor& tensor) {
        tensors[tensor.name] = tensor;
    }

    /**
     * @brief Register a tensor with parameters
     * @param name Tensor name
     * @param shape Tensor shape
     * @param dtype Data type
     * @param constant Whether tensor is constant
     */
    void registerTensor(const std::string& name, 
                        const tensor::Shape& shape,
                        tensor::DataType dtype = tensor::DataType::FLOAT32,
                        bool constant = false) {
        tensors[name] = GraphTensor(name, shape, dtype, constant);
    }

    /**
     * @brief Check if a tensor exists in the graph
     * @param name Tensor name
     * @return true if tensor is registered
     */
    bool hasTensor(const std::string& name) const {
        return tensors.find(name) != tensors.end();
    }

    /**
     * @brief Get tensor metadata by name
     * @param name Tensor name
     * @return Const reference to tensor metadata
     * @throws std::out_of_range if tensor not found
     */
    const GraphTensor& getTensor(const std::string& name) const {
        auto it = tensors.find(name);
        if (it == tensors.end()) {
            throw std::out_of_range("Tensor not found: " + name);
        }
        return it->second;
    }

    /**
     * @brief Get number of nodes in the graph
     */
    size_t numNodes() const { return nodes.size(); }

    /**
     * @brief Get number of registered tensors
     */
    size_t numTensors() const { return tensors.size(); }

    /**
     * @brief Find a node by name
     * @param name Node name
     * @return Pointer to node, or nullptr if not found
     */
    GraphNode* findNode(const std::string& name) {
        for (auto& node : nodes) {
            if (node->name == name) {
                return node.get();
            }
        }
        return nullptr;
    }

    /**
     * @brief Find a node by name (const version)
     * @param name Node name
     * @return Const pointer to node, or nullptr if not found
     */
    const GraphNode* findNode(const std::string& name) const {
        for (const auto& node : nodes) {
            if (node->name == name) {
                return node.get();
            }
        }
        return nullptr;
    }

    /**
     * @brief Validate graph structure
     * @return true if graph is valid (all tensor references exist)
     */
    bool validate() const {
        // Check that all graph inputs are either registered or will be provided at runtime
        // Check that all node input tensors are either graph inputs or produced by another node
        
        std::unordered_map<std::string, bool> available;
        
        // Mark graph inputs as available
        for (const auto& inp : input_names) {
            available[inp] = true;
        }
        
        // Mark constant tensors as available
        for (const auto& [name, tensor] : tensors) {
            if (tensor.constant) {
                available[name] = true;
            }
        }
        
        // Check each node's inputs and mark outputs as available
        for (const auto& node : nodes) {
            for (const auto& inp : node->input_tensors) {
                if (!available[inp]) {
                    return false; // Input not available
                }
            }
            for (const auto& out : node->output_tensors) {
                available[out] = true;
            }
        }
        
        // Check that all graph outputs are available
        for (const auto& out : output_names) {
            if (!available[out]) {
                return false;
            }
        }
        
        return true;
    }
};

} // namespace graph
} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_GRAPH_HPP
