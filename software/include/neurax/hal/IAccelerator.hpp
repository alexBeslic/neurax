/**
 * @file IAccelerator.hpp
 * @brief Main interface for hardware accelerators
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_HAL_IACCELERATOR_HPP
#define NEURAX_HAL_IACCELERATOR_HPP

#include "AcceleratorTypes.hpp"
#include "AcceleratorException.hpp"
#include "neurax/tensor/Tensor.hpp"
#include <memory>

namespace neurax {
namespace hal {

// Import tensor utilities for convenience
using neurax::tensor::Tensor;

/**
 * @brief Abstract interface for all hardware accelerators
 *
 * This interface defines the contract that all accelerator implementations
 * must follow. It provides a common API for neural network operations
 * regardless of the underlying hardware (FPGA, GPU, CPU, etc.).
 */
class IAccelerator {
public:
    virtual ~IAccelerator() = default;

    /**
     * @brief Get the accelerator type
     * @return Type of this accelerator
     */
    virtual AcceleratorType get_type() const = 0;

    /**
     * @brief Initialize the accelerator hardware
     * @return true if initialization successful, false otherwise
     * @throws HardwareInitializationException on critical errors
     */
    virtual bool initialize() = 0;

    /**
     * @brief Clean up accelerator resources
     */
    virtual void cleanup() = 0;

    /**
     * @brief Check if accelerator is available and ready
     * @return true if accelerator is ready for operations
     */
    virtual bool is_available() const = 0;

    /**
     * @brief Get current accelerator status
     * @return Status information including errors and availability
     */
    virtual AcceleratorStatus get_status() const = 0;

    // Neural Network Operations

    /**
     * @brief Perform convolution operation
     * @param input Input tensor (batch, height, width, channels)
     * @param weights Convolution weights tensor
     * @param bias Bias tensor (optional)
     * @param config Convolution configuration
     * @return Output tensor after convolution
     * @throws HardwareOperationException on operation failure
     */
    virtual Tensor convolution(const Tensor& input,
                              const Tensor& weights,
                              const Tensor& bias,
                              const ConvolutionConfig& config) = 0;

    /**
     * @brief Perform activation function
     * @param input Input tensor
     * @param type Activation function type
     * @return Output tensor after activation
     * @throws HardwareOperationException on operation failure
     */
    virtual Tensor activation(const Tensor& input,
                             ActivationType type) = 0;

    /**
     * @brief Perform activation function (alias for activation)
     * @param input Input tensor
     * @param type Activation function type
     * @return Output tensor after activation
     * @throws HardwareOperationException on operation failure
     */
    virtual Tensor applyActivation(const Tensor& input,
                                  ActivationType type) {
        return activation(input, type);
    }

    /**
     * @brief Perform pooling operation
     * @param input Input tensor
     * @param config Pooling configuration
     * @return Output tensor after pooling
     * @throws HardwareOperationException on operation failure
     */
    virtual Tensor pooling(const Tensor& input,
                          const PoolingConfig& config) = 0;

    /**
     * @brief Perform dense (fully connected) layer operation
     * @param input Input tensor
     * @param weights Weight matrix
     * @param bias Bias vector (optional)
     * @return Output tensor after matrix multiplication
     * @throws HardwareOperationException on operation failure
     */
    virtual Tensor dense(const Tensor& input,
                        const Tensor& weights,
                        const Tensor& bias) = 0;

    /**
     * @brief Perform batch normalization
     * @param input Input tensor (NHWC or 2D)
     * @param gamma Scale parameter (per-channel)
     * @param beta Shift parameter (per-channel)
     * @param epsilon Small constant for numerical stability
     * @param momentum Momentum for running statistics (optional)
     * @return Output tensor after batch normalization
     */
    virtual Tensor batchnorm(const Tensor& input,
                             const Tensor& gamma,
                             const Tensor& beta,
                             float epsilon,
                             float momentum) {
        throw std::runtime_error("batchnorm not implemented for this accelerator");
    }

    // Configuration and Control

    /**
     * @brief Set data precision for operations
     * @param use_16bit If true, use 16-bit precision; otherwise 8-bit
     * @return true if precision setting was successful
     */
    virtual bool set_precision(bool use_16bit) = 0;

    /**
     * @brief Get current precision setting
     * @return true if using 16-bit precision, false for 8-bit
     */
    virtual bool is_16bit_precision() const = 0;

    /**
     * @brief Enable or disable debug mode
     * @param enable If true, enable verbose debugging
     */
    virtual void set_debug_mode(bool enable) = 0;

    /**
     * @brief Check if debug mode is enabled
     * @return true if debug mode is active
     */
    virtual bool is_debug_mode() const = 0;
};

} // namespace hal
} // namespace neurax

#endif /* NEURAX_HAL_IACCELERATOR_HPP */
