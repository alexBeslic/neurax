/**
 * @file GraphExecutor.cpp
 * @brief Implementation of graph execution engine
 *
 * @author NEURAX Development Team
 * @date January 2026
 * @version 2.0
 */

#include "neurax/core/execution/GraphExecutor.hpp"
#include <stdexcept>
#include <algorithm>
#include <unordered_set>

namespace neurax {
namespace core {
namespace execution {

GraphExecutor::GraphExecutor(graph::Graph* g, hal::IAccelerator* acc)
    : graph_(g)
    , accelerator_(acc)
    , schedule_built_(false)
{
    if (!graph_) {
        throw std::invalid_argument("GraphExecutor requires a valid graph pointer");
    }
}

bool GraphExecutor::buildSchedule() {
    buildRefcountTable();
    schedule_ = topologicalSort();
    schedule_built_ = (schedule_.size() == graph_->nodes.size());
    
    // Assign accelerator to all layers if available
    if (accelerator_) {
        for (auto* node : schedule_) {
            if (node->layer) {
                node->layer->addAccelerator(accelerator_);
            }
        }
    }
    
    return schedule_built_;
}

std::vector<graph::GraphNode*> GraphExecutor::topologicalSort() {
    // Build producer map: tensor_name -> producing node
    std::unordered_map<std::string, graph::GraphNode*> producers;
    for (auto& node : graph_->nodes) {
        for (const auto& out : node->output_tensors) {
            producers[out] = node.get();
        }
    }
    
    // Build consumer map: tensor_name -> list of consuming nodes
    std::unordered_map<std::string, std::vector<graph::GraphNode*>> consumers;
    for (auto& node : graph_->nodes) {
        for (const auto& inp : node->input_tensors) {
            consumers[inp].push_back(node.get());
        }
    }
    
    // Initialize indegree map - count dependencies for each node
    std::unordered_map<graph::GraphNode*, int> indegree;
    for (auto& node : graph_->nodes) {
        indegree[node.get()] = 0;
    }
    
    // Count dependencies: for each input tensor that has a producer node,
    // increment the consumer's indegree
    for (auto& node : graph_->nodes) {
        std::unordered_set<graph::GraphNode*> deps; // Avoid counting same producer twice
        for (const auto& inp : node->input_tensors) {
            auto it = producers.find(inp);
            if (it != producers.end() && it->second != node.get()) {
                deps.insert(it->second);
            }
        }
        indegree[node.get()] = static_cast<int>(deps.size());
    }
    
    // Kahn's algorithm - BFS from nodes with no dependencies
    std::queue<graph::GraphNode*> ready;
    for (auto& node : graph_->nodes) {
        if (indegree[node.get()] == 0) {
            ready.push(node.get());
        }
    }
    
    std::vector<graph::GraphNode*> result;
    result.reserve(graph_->nodes.size());
    
    while (!ready.empty()) {
        auto* current = ready.front();
        ready.pop();
        result.push_back(current);
        
        // For each output tensor of current node, reduce indegree of consumers
        for (const auto& out : current->output_tensors) {
            auto it = consumers.find(out);
            if (it != consumers.end()) {
                for (auto* consumer : it->second) {
                    if (--indegree[consumer] == 0) {
                        ready.push(consumer);
                    }
                }
            }
        }
    }
    
    // If result size != node count, there's a cycle
    return result;
}

void GraphExecutor::buildRefcountTable() {
    tensor_refcount_base_.clear();
    
    // Count how many times each tensor is used as input
    for (auto& node : graph_->nodes) {
        for (const auto& inp : node->input_tensors) {
            tensor_refcount_base_[inp]++;
        }
    }
    
    // Graph outputs need extra refcount to prevent early freeing
    for (const auto& out : graph_->output_names) {
        tensor_refcount_base_[out]++;
    }
}

void GraphExecutor::executeNode(graph::GraphNode* node,
                                 std::unordered_map<std::string, int>& refcount) {
    // Gather input tensors
    std::vector<const tensor::Tensor*> node_inputs;
    node_inputs.reserve(node->input_tensors.size());
    
    for (const auto& inp_name : node->input_tensors) {
        auto it = tensor_pool_.find(inp_name);
        if (it == tensor_pool_.end()) {
            throw std::runtime_error("GraphExecutor: Missing input tensor '" + inp_name + 
                                     "' for node '" + node->name + "'");
        }
        node_inputs.push_back(&it->second);
    }
    
    // Execute the layer
    std::vector<tensor::Tensor> node_outputs;
    
    if (node_inputs.size() == 1) {
        // Use single-input forward for compatibility
        node_outputs.push_back(node->layer->forward(*node_inputs[0]));
    } else {
        // Use multi-input forward
        node_outputs = node->layer->forward(node_inputs);
    }
    
    // Verify output count
    if (node_outputs.size() != node->output_tensors.size()) {
        throw std::runtime_error("GraphExecutor: Output count mismatch for node '" + 
                                 node->name + "'. Expected " + 
                                 std::to_string(node->output_tensors.size()) + 
                                 ", got " + std::to_string(node_outputs.size()));
    }
    
    // Store outputs in tensor pool
    for (size_t i = 0; i < node_outputs.size(); i++) {
        tensor_pool_[node->output_tensors[i]] = std::move(node_outputs[i]);
    }
    
    // Decrement refcounts and free unused tensors (memory optimization)
    for (const auto& inp_name : node->input_tensors) {
        if (--refcount[inp_name] == 0) {
            // Check if it's not a graph output before freeing
            bool is_output = std::find(graph_->output_names.begin(), 
                                       graph_->output_names.end(), 
                                       inp_name) != graph_->output_names.end();
            if (!is_output) {
                tensor_pool_.erase(inp_name);
            }
        }
    }
}

void GraphExecutor::run(
    const std::unordered_map<std::string, tensor::Tensor>& inputs,
    std::unordered_map<std::string, tensor::Tensor>& outputs) {
    
    if (!schedule_built_) {
        throw std::runtime_error("GraphExecutor: Schedule not built. Call buildSchedule() first.");
    }
    
    // Reset tensor pool for this run
    tensor_pool_.clear();
    
    // Create working copy of refcount table
    auto refcount = tensor_refcount_base_;
    
    // Validate and load inputs
    for (const auto& inp_name : graph_->input_names) {
        auto it = inputs.find(inp_name);
        if (it == inputs.end()) {
            throw std::runtime_error("GraphExecutor: Missing graph input tensor '" + inp_name + "'");
        }
        tensor_pool_[inp_name] = it->second;
    }
    
    // Execute schedule
    for (auto* node : schedule_) {
        executeNode(node, refcount);
    }
    
    // Collect outputs
    outputs.clear();
    for (const auto& out_name : graph_->output_names) {
        auto it = tensor_pool_.find(out_name);
        if (it != tensor_pool_.end()) {
            outputs[out_name] = std::move(it->second);
        } else {
            throw std::runtime_error("GraphExecutor: Output tensor '" + out_name + 
                                     "' not produced during execution");
        }
    }
    
    // Clear remaining tensors
    tensor_pool_.clear();
}

} // namespace execution
} // namespace core
} // namespace neurax
