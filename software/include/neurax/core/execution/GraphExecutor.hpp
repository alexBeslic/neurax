/**
 * @file GraphExecutor.hpp
 * @brief Executes a computation graph using topological ordering
 *
 * @author NEURAX Development Team
 * @date January 2026
 * @version 2.0
 */

#ifndef NEURAX_CORE_GRAPH_EXECUTOR_HPP
#define NEURAX_CORE_GRAPH_EXECUTOR_HPP

#include "neurax/core/graph/Graph.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/hal/IAccelerator.hpp"
#include <unordered_map>
#include <vector>
#include <queue>
#include <string>

namespace neurax {
namespace core {
namespace execution {

/**
 * @brief Executes a computation graph using topological ordering
 * 
 * Responsibilities:
 * - Build dependency graph from node connections
 * - Perform topological sort using Kahn's algorithm
 * - Execute nodes in dependency order
 * - Manage tensor lifetimes (free after last use)
 */
class GraphExecutor {
private:
    graph::Graph* graph_;                   ///< Pointer to the graph structure
    hal::IAccelerator* accelerator_;        ///< Hardware accelerator (optional)
    
    std::vector<graph::GraphNode*> schedule_;           ///< Topologically sorted execution order
    std::unordered_map<std::string, int> tensor_refcount_base_;  ///< Base reference counts
    
    // Runtime tensor storage
    std::unordered_map<std::string, tensor::Tensor> tensor_pool_;

    bool schedule_built_;   ///< Whether schedule has been computed

public:
    /**
     * @brief Constructor
     * @param g Pointer to the graph to execute
     * @param acc Optional hardware accelerator
     */
    GraphExecutor(graph::Graph* g, hal::IAccelerator* acc = nullptr);

    /**
     * @brief Destructor
     */
    ~GraphExecutor() = default;
    
    /**
     * @brief Build execution schedule using topological sort
     * @return true if graph is valid (no cycles), false otherwise
     */
    bool buildSchedule();
    
    /**
     * @brief Execute the graph
     * @param inputs Map of input tensor names to tensors
     * @param outputs Map to store output tensors (cleared and populated)
     * @throws std::runtime_error if schedule not built or execution fails
     */
    void run(const std::unordered_map<std::string, tensor::Tensor>& inputs,
             std::unordered_map<std::string, tensor::Tensor>& outputs);

    /**
     * @brief Get the execution schedule
     * @return Const reference to the node execution order
     */
    const std::vector<graph::GraphNode*>& getSchedule() const { return schedule_; }

    /**
     * @brief Check if schedule has been built
     */
    bool isScheduleBuilt() const { return schedule_built_; }

    /**
     * @brief Set the hardware accelerator
     * @param acc Accelerator to use for execution
     */
    void setAccelerator(hal::IAccelerator* acc) { accelerator_ = acc; }

private:
    /**
     * @brief Topological sort using Kahn's algorithm
     * @return Vector of nodes in execution order, empty if cycle detected
     */
    std::vector<graph::GraphNode*> topologicalSort();
    
    /**
     * @brief Build reference count table for tensor lifetime management
     */
    void buildRefcountTable();
    
    /**
     * @brief Execute a single node
     * @param node Node to execute
     * @param refcount Current reference counts (modified in place)
     */
    void executeNode(graph::GraphNode* node, 
                     std::unordered_map<std::string, int>& refcount);
};

} // namespace execution
} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_GRAPH_EXECUTOR_HPP
