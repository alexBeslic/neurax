/*
 * NEURAX Neural Network Accelerator Library
 * Private Header File
 * 
 * Author: NEURAX Team
 */

#ifndef NEURAX_PRIVATE_H
#define NEURAX_PRIVATE_H

#include "neurax.h"
#include <stdint.h>
#include <stdbool.h>

// Register addresses (relative to base)
#define NEURAX_REG_CONTROL      0x00
#define NEURAX_REG_STATUS       0x04
#define NEURAX_REG_CONV_CONFIG  0x08
#define NEURAX_REG_POOL_CONFIG  0x0C
#define NEURAX_REG_ACT_CONFIG   0x10
#define NEURAX_REG_DIM_CONFIG   0x14
#define NEURAX_REG_WEIGHT_ADDR  0x18
#define NEURAX_REG_BIAS_ADDR    0x1C

// Control register bits
#define CTRL_START      (1 << 0)
#define CTRL_RESET      (1 << 1)
#define CTRL_CONV_EN    (1 << 2)
#define CTRL_POOL_EN    (1 << 3)
#define CTRL_ACT_EN     (1 << 4)
#define CTRL_DATA_WIDTH (1 << 5)

// Status register bits
#define STAT_BUSY       (1 << 0)
#define STAT_DONE       (1 << 1)
#define STAT_ERROR      (1 << 2)

// Device structure (private)
struct neurax_device {
    neurax_config_t config;
    bool initialized;
    int device_fd;              // Device file descriptor
    void* mapped_memory;        // Mapped device memory
    size_t mapped_size;         // Size of mapped memory
    uint32_t* register_base;    // Register base address
    bool hardware_available;    // Hardware availability flag
};

// Internal configuration constants
#define NEURAX_MAX_TENSOR_DIMS 4
#define NEURAX_MAX_LAYERS 256
#define NEURAX_DEFAULT_TIMEOUT_MS 5000

// Model structure (private)
struct neurax_model {
    neurax_device_t* device;
    uint32_t num_layers;
    void* layer_configs;        // Array of layer configurations
    neurax_tensor_t** weights;  // Array of weight tensors
    neurax_tensor_t** biases;   // Array of bias tensors
    char* model_data;           // Raw model data
    size_t model_size;          // Size of model data
    bool loaded;                // Model load status
};

// Layer type enumeration
typedef enum {
    NEURAX_LAYER_CONV2D = 0,
    NEURAX_LAYER_POOLING = 1,
    NEURAX_LAYER_ACTIVATION = 2,
    NEURAX_LAYER_DENSE = 3,
    NEURAX_LAYER_BATCH_NORM = 4
} neurax_layer_type_t;

// Generic layer configuration (for future model-based API)
// NOTE: Currently unused - planned for future model loading functionality
typedef struct {
    neurax_layer_type_t type;
    uint32_t input_shape[4];    // [batch, height, width, channels]
    uint32_t output_shape[4];   // [batch, height, width, channels]
    void* layer_params;         // Pointer to specific layer parameters
} neurax_layer_config_t;

// Internal function declarations

// Hardware acceleration functions
neurax_error_t neurax_hw_conv2d(neurax_device_t* device,
                               const neurax_tensor_t* input,
                               const neurax_tensor_t* weights,
                               const neurax_tensor_t* bias,
                               const neurax_conv_config_t* config,
                               neurax_tensor_t* output);

neurax_error_t neurax_hw_pooling(neurax_device_t* device,
                                const neurax_tensor_t* input,
                                const neurax_pool_config_t* config,
                                neurax_tensor_t* output);

neurax_error_t neurax_hw_activation(neurax_device_t* device,
                                   const neurax_tensor_t* input,
                                   neurax_activation_t activation,
                                   neurax_tensor_t* output);

// CPU emulation functions
neurax_error_t neurax_cpu_conv2d(const neurax_tensor_t* input,
                                const neurax_tensor_t* weights,
                                const neurax_tensor_t* bias,
                                const neurax_conv_config_t* config,
                                neurax_tensor_t* output);

neurax_error_t neurax_cpu_pooling(const neurax_tensor_t* input,
                                 const neurax_pool_config_t* config,
                                 neurax_tensor_t* output);

neurax_error_t neurax_cpu_activation(const neurax_tensor_t* input,
                                    neurax_activation_t activation,
                                    neurax_tensor_t* output);

// Utility functions
neurax_error_t neurax_validate_tensor(const neurax_tensor_t* tensor);
neurax_error_t neurax_validate_conv_config(const neurax_conv_config_t* config);
neurax_error_t neurax_validate_pool_config(const neurax_pool_config_t* config);

// Memory management functions
neurax_error_t neurax_alloc_aligned(size_t size, size_t alignment, void** ptr);
neurax_error_t neurax_free_aligned(void* ptr);

// Data conversion functions
neurax_error_t neurax_convert_data_type(const void* src, neurax_data_type_t src_type,
                                       void* dst, neurax_data_type_t dst_type,
                                       size_t num_elements);

// Helper functions for tensor operations
float neurax_get_tensor_value(const neurax_tensor_t* tensor, uint32_t batch, uint32_t y, uint32_t x, uint32_t c);
float neurax_get_weight_value(const neurax_tensor_t* weights, uint32_t out_ch, uint32_t in_ch, uint32_t ky, uint32_t kx);
float neurax_get_bias_value(const neurax_tensor_t* bias, uint32_t channel);
void neurax_set_tensor_value(neurax_tensor_t* tensor, uint32_t batch, uint32_t y, uint32_t x, uint32_t c, float value);
float neurax_get_tensor_element(const neurax_tensor_t* tensor, size_t index);
void neurax_set_tensor_element(neurax_tensor_t* tensor, size_t index, float value);
float neurax_apply_activation(float value, neurax_activation_t activation);

// Hardware register access
void neurax_write_reg(neurax_device_t* device, uint32_t offset, uint32_t value);
uint32_t neurax_read_reg(neurax_device_t* device, uint32_t offset);
neurax_error_t neurax_wait_for_completion(neurax_device_t* device, uint32_t timeout_ms);

// Performance profiling
typedef struct {
    double total_time_ms;
    double hw_time_ms;
    double data_transfer_time_ms;
    uint32_t num_operations;
} neurax_perf_stats_t;

neurax_error_t neurax_perf_start(neurax_perf_stats_t* stats);
neurax_error_t neurax_perf_end(neurax_perf_stats_t* stats);
void neurax_perf_print(const neurax_perf_stats_t* stats);

// Debug and logging
#ifdef NEURAX_DEBUG
#define NEURAX_LOG_DEBUG(fmt, ...) printf("[NEURAX DEBUG] " fmt "\n", ##__VA_ARGS__)
#define NEURAX_LOG_INFO(fmt, ...) printf("[NEURAX INFO] " fmt "\n", ##__VA_ARGS__)
#define NEURAX_LOG_ERROR(fmt, ...) printf("[NEURAX ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define NEURAX_LOG_DEBUG(fmt, ...)
#define NEURAX_LOG_INFO(fmt, ...)
#define NEURAX_LOG_ERROR(fmt, ...)
#endif

// Register access macros
#define NEURAX_WRITE_REG(device, offset, value) neurax_write_reg(device, offset, value)
#define NEURAX_READ_REG(device, offset) neurax_read_reg(device, offset)

// Hardware register bit field structures
typedef union {
    uint32_t raw;
    struct {
        uint32_t kernel_size    : 4;  // Bits 3:0  - Kernel size (width-1)
        uint32_t stride         : 3;  // Bits 6:4  - Stride (stride-1)  
        uint32_t padding        : 2;  // Bits 8:7  - Padding
        uint32_t use_bias       : 1;  // Bit 9     - Bias enable
        uint32_t input_channels : 3;  // Bits 12:10 - Input channels (count-1)
        uint32_t reserved       : 19; // Bits 31:13 - Reserved
    } bits;
} neurax_conv_config_reg_t;

typedef union {
    uint32_t raw;
    struct {
        uint32_t width          : 16; // Bits 15:0  - Input width
        uint32_t height         : 16; // Bits 31:16 - Input height
    } bits;
} neurax_dim_config_reg_t;

typedef union {
    uint32_t raw;
    struct {
        uint32_t activation     : 2;  // Bits 1:0   - Activation function
        uint32_t reserved       : 30; // Bits 31:2  - Reserved
    } bits;
} neurax_act_config_reg_t;

typedef union {
    uint32_t raw;
    struct {
        uint32_t start          : 1;  // Bit 0      - Start operation
        uint32_t reset          : 1;  // Bit 1      - Reset
        uint32_t conv_en        : 1;  // Bit 2      - Convolution enable
        uint32_t pool_en        : 1;  // Bit 3      - Pooling enable
        uint32_t act_en         : 1;  // Bit 4      - Activation enable
        uint32_t data_width     : 1;  // Bit 5      - Data width (0=8bit, 1=16bit)
        uint32_t reserved       : 26; // Bits 31:6  - Reserved
    } bits;
} neurax_control_reg_t;

// Inline helper functions
static inline size_t neurax_get_element_size(neurax_data_type_t type) {
    switch (type) {
        case NEURAX_DATA_UINT8:
        case NEURAX_DATA_INT8:
            return 1;
        case NEURAX_DATA_UINT16:
        case NEURAX_DATA_INT16:
            return 2;
        case NEURAX_DATA_FLOAT32:
            return 4;
        default:
            return 0;
    }
}

static inline bool neurax_is_signed_type(neurax_data_type_t type) {
    return (type == NEURAX_DATA_INT8 || type == NEURAX_DATA_INT16);
}

// Hardware register helper functions
static inline void neurax_write_conv_config(neurax_device_t* device, const neurax_conv_config_reg_t* config) {
    NEURAX_WRITE_REG(device, NEURAX_REG_CONV_CONFIG, config->raw);
}

static inline neurax_conv_config_reg_t neurax_read_conv_config(neurax_device_t* device) {
    neurax_conv_config_reg_t config;
    config.raw = NEURAX_READ_REG(device, NEURAX_REG_CONV_CONFIG);
    return config;
}

static inline void neurax_write_control(neurax_device_t* device, const neurax_control_reg_t* control) {
    NEURAX_WRITE_REG(device, NEURAX_REG_CONTROL, control->raw);
}

static inline neurax_control_reg_t neurax_read_control(neurax_device_t* device) {
    neurax_control_reg_t control;
    control.raw = NEURAX_READ_REG(device, NEURAX_REG_CONTROL);
    return control;
}

#endif // NEURAX_PRIVATE_H
