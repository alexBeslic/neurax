/**
 * @file Network.hpp
 * @brief Implementation of the Network class
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_CORE_NETWORK_HPP
#define NEURAX_CORE_NETWORK_HPP

#include "neurax/core/network/INetwork.hpp"
#include "neurax/core/graph/Graph.hpp"
#include "neurax/core/execution/GraphExecutor.hpp"
#include <vector>
#include <unordered_map>

namespace neurax {

namespace core {

/**
 * @brief Neural network implementation supporting both linear and graph execution
 * 
 * Supports:
 * - Legacy sequential layer execution (backward compatible)
 * - DAG-based graph execution with multiple inputs/outputs
 * - Skip connections and branching
 */
class NeuralNetwork : public INetwork
{
private:
    friend class NeuralNetworkBuilder;
    std::unique_ptr<neurax::hal::IAccelerator> accelerator_;
    
    // Legacy linear execution
    std::vector<std::unique_ptr<ILayer>> layers_;
    
    // Graph-based execution
    std::unique_ptr<graph::Graph> graph_;
    std::unique_ptr<execution::GraphExecutor> executor_;
    bool use_graph_execution_;

    NeuralNetwork() : use_graph_execution_(false) {}
    ~NeuralNetwork();

public:
    // INetwork interface (legacy)
    void addLayer(std::unique_ptr<ILayer> layer) override;
    void removeLayer(size_t index) override;
    std::unique_ptr<ILayer>& getLayer(size_t index) override;
    size_t getLayerCount() const override;
    void addAccelerator(std::unique_ptr<neurax::hal::IAccelerator> accelerator) override;
    neurax::hal::IAccelerator* getAccelerator() override;
    void infer(const Tensor& input, Tensor& output) override;
    Tensor infer(const Tensor& input) override;

    // Graph-based API
    
    /**
     * @brief Set the computation graph
     * @param g Graph to use for execution (ownership transferred)
     */
    void setGraph(std::unique_ptr<graph::Graph> g);

    /**
     * @brief Enable or disable graph execution mode
     * @param enable true to use graph execution, false for linear
     */
    void enableGraphExecution(bool enable);

    /**
     * @brief Check if graph execution is enabled
     */
    bool isGraphExecutionEnabled() const { return use_graph_execution_; }

    /**
     * @brief Multi-input/output inference using graph execution
     * @param inputs Map of input tensor names to tensors
     * @param outputs Map to store output tensors
     */
    void infer(const std::unordered_map<std::string, Tensor>& inputs,
               std::unordered_map<std::string, Tensor>& outputs);

    /**
     * @brief Get the underlying graph (if set)
     * @return Pointer to graph, or nullptr
     */
    graph::Graph* getGraph() { return graph_.get(); }

    /**
     * @brief Get the graph executor (if created)
     * @return Pointer to executor, or nullptr
     */
    execution::GraphExecutor* getExecutor() { return executor_.get(); }
};

} // namespace core
} // namespace neurax

#endif /* NEURAX_CORE_NETWORK_HPP */