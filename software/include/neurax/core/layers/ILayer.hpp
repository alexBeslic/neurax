/**
 * @file ILayer.hpp
 * @brief Interface for neural network layers
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_CORE_ILAYER_HPP
#define NEURAX_CORE_ILAYER_HPP

#include "neurax/tensor/Tensor.hpp"
#include "neurax/hal/IAccelerator.hpp"
#include <vector>
#include <stdexcept>

namespace neurax {

namespace core {

using namespace neurax::tensor;

/**
 * @brief Abstract interface for all neural network layers
 * 
 * Supports both single-input (legacy) and multi-input (graph) execution.
 */
class ILayer
{
public:
    virtual ~ILayer() = default;

    /**
     * @brief Single-input forward pass (legacy interface)
     * @param input Input tensor
     * @param output Output tensor (modified in place)
     */
    virtual void forward(const Tensor& input, Tensor& output) = 0;

    /**
     * @brief Single-input forward pass returning tensor
     * @param input Input tensor
     * @return Output tensor
     */
    virtual Tensor forward(const Tensor& input) = 0;

    /**
     * @brief Multi-input forward pass for graph execution
     * @param inputs Vector of input tensor pointers
     * @return Vector of output tensors
     * 
     * Default implementation handles single-input layers.
     * Override for layers that accept multiple inputs (Add, Concat, etc.)
     */
    virtual std::vector<Tensor> forward(const std::vector<const Tensor*>& inputs) {
        if (inputs.size() != numInputs()) {
            throw std::runtime_error("Layer expects " + std::to_string(numInputs()) + 
                                     " inputs, got " + std::to_string(inputs.size()));
        }
        // Default: single input, single output
        return { forward(*inputs[0]) };
    }

    /**
     * @brief Set the hardware accelerator for this layer
     * @param accelerator Accelerator to use
     */
    virtual void addAccelerator(neurax::hal::IAccelerator* accelerator) = 0;

    /**
     * @brief Get number of expected inputs
     * @return Number of inputs (default: 1)
     */
    virtual size_t numInputs() const { return 1; }

    /**
     * @brief Get number of outputs
     * @return Number of outputs (default: 1)
     */
    virtual size_t numOutputs() const { return 1; }
};

} // namespace core
} // namespace neurax

#endif /* NEURAX_CORE_ILAYER_HPP */