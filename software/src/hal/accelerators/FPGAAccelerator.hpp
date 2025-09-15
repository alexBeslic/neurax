/**
 * @file FPGAAccelerator.hpp
 * @brief FPGA accelerator implementation for DE1-SoC platform
 * 
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_HAL_FPGA_ACCELERATOR_HPP
#define NEURAX_HAL_FPGA_ACCELERATOR_HPP

#include "neurax/hal/IAccelerator.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"
#include "neurax/bsp/neurax_bsp.h"
#include <atomic>
#include <mutex>

namespace neurax {
namespace hal {

/**
 * @brief FPGA accelerator implementation using DE1-SoC BSP
 * 
 * This accelerator uses the Board Support Package (BSP) to communicate
 * with the FPGA hardware on the DE1-SoC platform. It provides hardware-
 * accelerated neural network operations through custom FPGA logic.
 */
class FPGAAccelerator : public IAccelerator {
private:
    std::atomic<bool> initialized_;
    std::atomic<bool> available_;
    std::atomic<bool> debug_mode_;
    std::atomic<bool> use_16bit_precision_;
    mutable std::mutex bsp_mutex_;  // Protect BSP calls from concurrent access
    mutable AcceleratorStatus status_;  // Make status mutable for const methods
    
public:
    /**
     * @brief Constructor
     */
    FPGAAccelerator();
    
    /**
     * @brief Destructor
     */
    ~FPGAAccelerator() override;
    
    // IAccelerator interface implementation
    AcceleratorType get_type() const override;
    bool initialize() override;
    void cleanup() override;
    bool is_available() const override;
    AcceleratorStatus get_status() const override;
    
    // Neural Network Operations
    Tensor convolution(const Tensor& input,
                             const Tensor& weights,
                             const Tensor& bias,
                             const ConvolutionConfig& config) override;
    
    Tensor activation(const Tensor& input,
                           ActivationType type) override;
    
    Tensor pooling(const Tensor& input,
                        const PoolingConfig& config) override;
    
    Tensor dense(const Tensor& input,
                       const Tensor& weights,
                       const Tensor& bias) override;
    
    // Configuration and Control
    bool set_precision(bool use_16bit) override;
    bool is_16bit_precision() const override;
    void set_debug_mode(bool enable) override;
    bool is_debug_mode() const override;
    
private:
    /**
     * @brief Convert HAL activation type to BSP activation type
     */
    neurax_activation_t convert_activation_type(ActivationType type) const;
    
    /**
     * @brief Convert HAL pooling type to BSP pooling type
     */
    neurax_pooling_t convert_pooling_type(PoolingType type) const;
    
    /**
     * @brief Convert HAL data precision to BSP data width
     */
    neurax_data_width_t convert_data_width(bool use_16bit) const;
    
    /**
     * @brief Setup DMA transfer for tensor data
     */
    bool setup_dma_transfer(const Tensor& input_tensor, 
                           Tensor& output_tensor,
                           size_t expected_output_size);
    
    /**
     * @brief Wait for FPGA operation completion
     */
    bool wait_for_completion(uint32_t timeout_ms = 5000);
    
    /**
     * @brief Update internal status based on BSP state
     */
    void update_status() const;
    
    /**
     * @brief Log debug message if debug mode is enabled
     */
    void debug_log(const std::string& message) const;
    
    /**
     * @brief Validate tensor dimensions for FPGA constraints
     */
    bool validate_tensor_constraints(const Tensor& tensor) const;
    
    /**
     * @brief Calculate expected output size for convolution
     */
    size_t calculate_conv_output_size(const std::vector<size_t>& input_shape,
                                     const ConvolutionConfig& config) const;
    
    /**
     * @brief Calculate expected output size for pooling
     */
    size_t calculate_pool_output_size(const std::vector<size_t>& input_shape,
                                     const PoolingConfig& config) const;
};

} // namespace hal
} // namespace neurax

#endif /* NEURAX_HAL_FPGA_ACCELERATOR_HPP */
