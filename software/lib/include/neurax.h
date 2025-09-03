/*
 * NEURAX Neural Network Accelerator Library
 * Main API Header File
 * 
 * Author: NEURAX Team
 * Date: August 2025
 */

#ifndef NEURAX_H
#define NEURAX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Version information
#define NEURAX_VERSION_MAJOR 1
#define NEURAX_VERSION_MINOR 0
#define NEURAX_VERSION_PATCH 0

// Error codes
typedef enum {
    NEURAX_SUCCESS = 0,
    NEURAX_ERROR_INVALID_PARAM = -1,
    NEURAX_ERROR_NOT_INITIALIZED = -2,
    NEURAX_ERROR_DEVICE_NOT_FOUND = -3,
    NEURAX_ERROR_MEMORY_ALLOCATION = -4,
    NEURAX_ERROR_HARDWARE_FAILURE = -5,
    NEURAX_ERROR_TIMEOUT = -6,
    NEURAX_ERROR_INVALID_MODEL = -7,
    NEURAX_ERROR_BUFFER_OVERFLOW = -8
} neurax_error_t;

// Data types
typedef enum {
    NEURAX_DATA_UINT8 = 0,
    NEURAX_DATA_INT8 = 1,
    NEURAX_DATA_UINT16 = 2,
    NEURAX_DATA_INT16 = 3,
    NEURAX_DATA_FLOAT32 = 4
} neurax_data_type_t;

// Activation functions
typedef enum {
    NEURAX_ACTIVATION_RELU = 0,
    NEURAX_ACTIVATION_TANH = 1,
    NEURAX_ACTIVATION_SIGMOID = 2,
    NEURAX_ACTIVATION_LINEAR = 3
} neurax_activation_t;

// Pooling types
typedef enum {
    NEURAX_POOL_MAX = 0,
    NEURAX_POOL_AVERAGE = 1
} neurax_pool_type_t;

// Device configuration
typedef struct {
    uint32_t base_address;          // FPGA device base address
    uint32_t memory_size;           // Available memory size
    bool use_hardware;              // True for FPGA, false for CPU emulation
    uint32_t max_kernel_size;       // Maximum supported kernel size
    uint32_t num_multipliers;       // Number of parallel multipliers
    neurax_data_type_t data_type;   // Default data type
} neurax_config_t;

// Layer configuration structures
typedef struct {
    uint32_t kernel_width;
    uint32_t kernel_height;
    uint32_t stride_x;
    uint32_t stride_y;
    uint32_t padding_x;
    uint32_t padding_y;
    uint32_t input_channels;
    uint32_t output_channels;
    bool use_bias;
    neurax_activation_t activation;
} neurax_conv_config_t;

typedef struct {
    uint32_t pool_width;
    uint32_t pool_height;
    uint32_t stride_x;
    uint32_t stride_y;
    neurax_pool_type_t pool_type;
} neurax_pool_config_t;

// Tensor structure
typedef struct {
    void* data;                     // Pointer to data
    uint32_t width;                 // Width dimension
    uint32_t height;                // Height dimension
    uint32_t channels;              // Number of channels
    uint32_t batch_size;            // Batch size
    neurax_data_type_t data_type;   // Data type
    size_t data_size;               // Size of data in bytes
} neurax_tensor_t;

// Neural network model structure
typedef struct neurax_model neurax_model_t;

// Device handle
typedef struct neurax_device neurax_device_t;

// Core API functions

/**
 * Initialize NEURAX library
 * @param config Device configuration
 * @param device Output device handle
 * @return Error code
 */
neurax_error_t neurax_init(const neurax_config_t* config, neurax_device_t** device);

/**
 * Cleanup and close NEURAX device
 * @param device Device handle
 * @return Error code
 */
neurax_error_t neurax_cleanup(neurax_device_t* device);

/**
 * Get library version string
 * @return Version string
 */
const char* neurax_get_version(void);

/**
 * Get error string for error code
 * @param error Error code
 * @return Error description string
 */
const char* neurax_get_error_string(neurax_error_t error);

// Tensor management functions

/**
 * Create a new tensor
 * @param width Width dimension
 * @param height Height dimension
 * @param channels Number of channels
 * @param batch_size Batch size
 * @param data_type Data type
 * @param tensor Output tensor
 * @return Error code
 */
neurax_error_t neurax_tensor_create(uint32_t width, uint32_t height, 
                                   uint32_t channels, uint32_t batch_size,
                                   neurax_data_type_t data_type,
                                   neurax_tensor_t** tensor);

/**
 * Destroy a tensor and free memory
 * @param tensor Tensor to destroy
 * @return Error code
 */
neurax_error_t neurax_tensor_destroy(neurax_tensor_t* tensor);

/**
 * Copy data to tensor
 * @param tensor Target tensor
 * @param data Source data
 * @param size Size of data in bytes
 * @return Error code
 */
neurax_error_t neurax_tensor_set_data(neurax_tensor_t* tensor, const void* data, size_t size);

/**
 * Get data from tensor
 * @param tensor Source tensor
 * @param data Output buffer
 * @param size Size of output buffer
 * @return Error code
 */
neurax_error_t neurax_tensor_get_data(const neurax_tensor_t* tensor, void* data, size_t size);

/**
 * Get total number of elements in tensor
 * @param tensor Input tensor
 * @return Total number of elements
 */
size_t neurax_tensor_total_elements(const neurax_tensor_t* tensor);

// Layer execution functions

/**
 * Execute 2D convolution
 * @param device Device handle
 * @param input Input tensor
 * @param weights Weight tensor
 * @param bias Bias tensor (can be NULL)
 * @param config Convolution configuration
 * @param output Output tensor
 * @return Error code
 */
neurax_error_t neurax_conv2d(neurax_device_t* device,
                            const neurax_tensor_t* input,
                            const neurax_tensor_t* weights,
                            const neurax_tensor_t* bias,
                            const neurax_conv_config_t* config,
                            neurax_tensor_t* output);

/**
 * Execute pooling operation
 * @param device Device handle
 * @param input Input tensor
 * @param config Pooling configuration
 * @param output Output tensor
 * @return Error code
 */
neurax_error_t neurax_pooling(neurax_device_t* device,
                             const neurax_tensor_t* input,
                             const neurax_pool_config_t* config,
                             neurax_tensor_t* output);

/**
 * Execute activation function
 * @param device Device handle
 * @param input Input tensor
 * @param activation Activation function type
 * @param output Output tensor
 * @return Error code
 */
neurax_error_t neurax_activation(neurax_device_t* device,
                                const neurax_tensor_t* input,
                                neurax_activation_t activation,
                                neurax_tensor_t* output);

// Model management functions

/**
 * Load model from file
 * @param device Device handle
 * @param filename Model file path
 * @param model Output model handle
 * @return Error code
 */
neurax_error_t neurax_model_load(neurax_device_t* device,
                                const char* filename,
                                neurax_model_t** model);

/**
 * Destroy model and free resources
 * @param model Model handle
 * @return Error code
 */
neurax_error_t neurax_model_destroy(neurax_model_t* model);

/**
 * Run inference on model
 * @param model Model handle
 * @param input Input tensor
 * @param output Output tensor
 * @return Error code
 */
neurax_error_t neurax_model_inference(neurax_model_t* model,
                                     const neurax_tensor_t* input,
                                     neurax_tensor_t* output);

// Utility functions

/**
 * Get optimal configuration for current hardware
 * @param device Device handle
 * @param config Output configuration
 * @return Error code
 */
neurax_error_t neurax_get_optimal_config(neurax_device_t* device, neurax_config_t* config);

/**
 * Benchmark layer performance
 * @param device Device handle
 * @param layer_type Type of layer to benchmark
 * @param iterations Number of iterations
 * @param time_ms Output time in milliseconds
 * @return Error code
 */
neurax_error_t neurax_benchmark_layer(neurax_device_t* device,
                                     const char* layer_type,
                                     uint32_t iterations,
                                     double* time_ms);

/**
 * Print device information
 * @param device Device handle
 * @return Error code
 */
neurax_error_t neurax_print_device_info(neurax_device_t* device);

#ifdef __cplusplus
}
#endif

#endif // NEURAX_H
