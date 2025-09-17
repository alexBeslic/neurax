/**
 * @file AcceleratorTypes.hpp
 * @brief Common types and enums for the Hardware Abstraction Layer
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_HAL_ACCELERATOR_TYPES_HPP
#define NEURAX_HAL_ACCELERATOR_TYPES_HPP

#include <cstdint>
#include <vector>
#include <string>

namespace neurax {
namespace hal {

/**
 * @brief Supported accelerator types
 */
enum class AcceleratorType {
    FPGA_DE1SOC,       ///< DE1-SoC FPGA accelerator
    GPU_OPENCL,        ///< GPU accelerator via OpenCL
    CPU_OPTIMIZED,     ///< CPU with SIMD optimizations
    SOFTWARE_FALLBACK  ///< Pure software implementation (always available)
};

/**
 * @brief Activation function types
 */
enum class ActivationType {
    RELU,
    TANH,
    SIGMOID,
    LINEAR
};

/**
 * @brief Pooling types
 */
enum class PoolingType {
    MAX,
    AVERAGE
};

/**
 * @brief Convolution configuration
 */
struct ConvolutionConfig {
    uint32_t kernel_size;
    uint32_t stride;
    uint32_t padding;
    uint32_t input_channels;
    uint32_t output_channels;

    // Default constructor
    ConvolutionConfig()
        : kernel_size(3), stride(1), padding(0), input_channels(1), output_channels(1) {}

    // Constructor with basic parameters
    ConvolutionConfig(uint32_t ks, uint32_t s, uint32_t p)
        : kernel_size(ks), stride(s), padding(p), input_channels(1), output_channels(1) {}

    // Full constructor
    ConvolutionConfig(uint32_t ks, uint32_t s, uint32_t p, uint32_t ic, uint32_t oc)
        : kernel_size(ks), stride(s), padding(p), input_channels(ic), output_channels(oc) {}
};

/**
 * @brief Pooling configuration
 */
struct PoolingConfig {
    uint32_t pool_size;     // Primary name for pool kernel size
    uint32_t stride;
    PoolingType type;
    uint32_t kernel_size;   // Alias for pool_size for compatibility

    // Default constructor
    PoolingConfig()
        : pool_size(2), stride(2), type(PoolingType::MAX), kernel_size(2) {}

    // Constructor with basic parameters
    PoolingConfig(uint32_t ps, uint32_t s, PoolingType t)
        : pool_size(ps), stride(s), type(t), kernel_size(ps) {}
};

/**
 * @brief Accelerator status information
 */
struct AcceleratorStatus {
    AcceleratorType active_type;
    AcceleratorType requested_type;
    bool using_fallback;
    std::string last_error;
    bool is_available;

    AcceleratorStatus()
        : active_type(AcceleratorType::SOFTWARE_FALLBACK)
        , requested_type(AcceleratorType::SOFTWARE_FALLBACK)
        , using_fallback(true)
        , is_available(true) {}
};

} // namespace hal
} // namespace neurax

#endif /* NEURAX_HAL_ACCELERATOR_TYPES_HPP */
