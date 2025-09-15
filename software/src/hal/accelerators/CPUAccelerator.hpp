/**
 * @file CPUAccelerator.hpp
 * @brief CPU-optimized accelerator implementation
 * 
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_HAL_CPU_ACCELERATOR_HPP
#define NEURAX_HAL_CPU_ACCELERATOR_HPP

#include "neurax/hal/IAccelerator.hpp"
#include <atomic>

namespace neurax {
namespace hal {

/**
 * @brief CPU-optimized implementation of accelerator interface
 * 
 * This accelerator uses CPU computations with SIMD optimizations
 * and multi-threading for improved performance over software fallback.
 * 
 * Performance characteristics:
 * - Faster than pure software implementation through optimizations
 * - Uses multi-threaded computation when beneficial
 * - SIMD instructions for vectorized operations
 * - Cache-aware memory access patterns
 */
class CPUAccelerator : public IAccelerator {
private:
    bool initialized_;
    bool use_16bit_precision_;
    bool debug_mode_;
    mutable std::atomic<bool> operation_in_progress_;
    std::string last_error_;

public:
    /**
     * @brief Constructor
     */
    CPUAccelerator();

    /**
     * @brief Destructor
     */
    ~CPUAccelerator() override;

    // IAccelerator interface implementation

    AcceleratorType get_type() const override {
        return AcceleratorType::CPU_OPTIMIZED;
    }

    bool initialize() override;
    void cleanup() override;
    bool is_available() const override;
    AcceleratorStatus get_status() const override;

    // Neural network operations
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

    // Configuration
    bool set_precision(bool use_16bit) override;
    bool is_16bit_precision() const override;
    void set_debug_mode(bool enable) override;
    bool is_debug_mode() const override;

private:
    /**
     * @brief CPU-optimized implementation of 2D convolution
     */
    Tensor cpu_convolution_optimized(const Tensor& input,
                                   const Tensor& weights,
                                   const Tensor& bias,
                                   const ConvolutionConfig& config);

    /**
     * @brief Single-threaded optimized convolution
     */
    Tensor cpu_convolution_single_threaded(const float* input_data,
                                          const float* weight_data,
                                          const float* bias_data,
                                          float* output_data,
                                          size_t batch_size,
                                          size_t input_height,
                                          size_t input_width,
                                          size_t input_channels,
                                          size_t output_height,
                                          size_t output_width,
                                          size_t output_channels,
                                          const ConvolutionConfig& config);

    /**
     * @brief Multi-threaded optimized convolution
     */
    Tensor cpu_convolution_threaded(const float* input_data,
                                   const float* weight_data,
                                   const float* bias_data,
                                   float* output_data,
                                   size_t batch_size,
                                   size_t input_height,
                                   size_t input_width,
                                   size_t input_channels,
                                   size_t output_height,
                                   size_t output_width,
                                   size_t output_channels,
                                   const ConvolutionConfig& config);

    /**
     * @brief Apply activation function element-wise with SIMD
     */
    void apply_activation_function_simd(float* output_data,
                                       const float* input_data,
                                       size_t num_elements,
                                       ActivationType type);

    /**
     * @brief Apply activation function element-wise with SIMD
     */
    void apply_activation_function(float* data, size_t size, ActivationType type);

    /**
     * @brief Matrix multiplication optimized for CPU
     */
    Tensor matrix_multiply_optimized(const Tensor& input,
                                    const Tensor& weights);

    /**
     * @brief Validate tensor dimensions for operations
     */
    void validate_tensor_dimensions(const Tensor& tensor,
                                  const std::vector<size_t>& expected_dims,
                                  const std::string& operation_name) const;

    /**
     * @brief Log debug message if debug mode is enabled
     */
    void debug_log(const std::string& message) const;

    /**
     * @brief Set error message and optionally throw exception
     */
    void set_error(const std::string& error, bool throw_exception = true);
};

} // namespace hal
} // namespace neurax

#endif /* NEURAX_HAL_CPU_ACCELERATOR_HPP */
