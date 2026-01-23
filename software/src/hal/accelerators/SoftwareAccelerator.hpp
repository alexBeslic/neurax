/**
 * @file SoftwareAccelerator.hpp
 * @brief Software-only     // Inherited interface methods
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
                const Tensor& bias) override;lementation (guaranteed fallback)
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_HAL_SOFTWARE_ACCELERATOR_HPP
#define NEURAX_HAL_SOFTWARE_ACCELERATOR_HPP

#include "neurax/hal/IAccelerator.hpp"
#include <atomic>

namespace neurax {
namespace hal {

/**
 * @brief Pure software implementation of accelerator interface
 *
 * This accelerator uses only CPU computations without any hardware
 * acceleration. It serves as the guaranteed fallback that always works
 * regardless of hardware availability.
 *
 * Performance characteristics:
 * - Slower than hardware accelerators but reliable
 * - Uses single-threaded computation (can be extended to multi-threaded)
 * - No external dependencies beyond standard C++ library
 * - Works on any platform that supports C++17
 */
class SoftwareAccelerator : public IAccelerator {
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
    SoftwareAccelerator();

    /**
     * @brief Destructor
     */
    ~SoftwareAccelerator() override;

    // IAccelerator interface implementation

    AcceleratorType get_type() const override {
        return AcceleratorType::SOFTWARE_FALLBACK;
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

    Tensor batchnorm(const Tensor& input,
                     const Tensor& gamma,
                     const Tensor& beta,
                     float epsilon,
                     float momentum) override;

    Tensor bias_add(const Tensor& input,
                    const Tensor& bias) override;

    // Configuration
    bool set_precision(bool use_16bit) override;
    bool is_16bit_precision() const override;
    void set_debug_mode(bool enable) override;
    bool is_debug_mode() const override;

private:
    /**
     * @brief Software implementation of 2D convolution
     */
    Tensor software_convolution_2d(const Tensor& input,
                                         const Tensor& weights,
                                         const Tensor& bias,
                                         const ConvolutionConfig& config);

    /**
     * @brief Calculate output dimensions for convolution
     */
    std::vector<size_t> calculate_conv_output_shape(
        const std::vector<size_t>& input_shape,
        const ConvolutionConfig& config) const;

    /**
     * @brief Software implementation of max pooling
     */
    Tensor software_max_pooling(const Tensor& input,
                                      const PoolingConfig& config);

    /**
     * @brief Software implementation of average pooling
     */
    Tensor software_avg_pooling(const Tensor& input,
                                      const PoolingConfig& config);

    /**
     * @brief Calculate output dimensions for pooling
     */
    std::vector<size_t> calculate_pool_output_shape(
        const std::vector<size_t>& input_shape,
        const PoolingConfig& config) const;

    /**
     * @brief Apply activation function element-wise
     */
    void apply_activation_function(float* data, size_t size, ActivationType type);

    /**
     * @brief ReLU activation function
     */
    inline float relu(float x) const { return x > 0.0f ? x : 0.0f; }

    /**
     * @brief Tanh activation function
     */
    inline float tanh_activation(float x) const;

    /**
     * @brief Sigmoid activation function
     */
    inline float sigmoid(float x) const;

    /**
     * @brief Matrix multiplication for dense layer
     */
    Tensor matrix_multiply(const Tensor& input,
                                 const Tensor& weights);

    /**
     * @brief Add bias to tensor
     */
    void add_bias(Tensor& tensor, const Tensor& bias);

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

#endif /* NEURAX_HAL_SOFTWARE_ACCELERATOR_HPP */
