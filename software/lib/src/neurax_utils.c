/*
 * NEURAX Validation and Utility Functions
 * 
 * Author: NEURAX Team
 */

#include "neurax.h"
#include "neurax_private.h"
#include <string.h>
#include <stdlib.h>

// Tensor validation
neurax_error_t neurax_validate_tensor(const neurax_tensor_t* tensor) {
    if (!tensor) {
        NEURAX_LOG_ERROR("Tensor pointer is NULL");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (!tensor->data) {
        NEURAX_LOG_ERROR("Tensor data pointer is NULL");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (tensor->width == 0 || tensor->height == 0 || 
        tensor->channels == 0 || tensor->batch_size == 0) {
        NEURAX_LOG_ERROR("Tensor has zero dimensions");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    size_t expected_size = tensor->width * tensor->height * 
                          tensor->channels * tensor->batch_size * 
                          neurax_get_element_size(tensor->data_type);
    
    if (tensor->data_size != expected_size) {
        NEURAX_LOG_ERROR("Tensor data size mismatch: expected %zu, got %zu", 
                        expected_size, tensor->data_size);
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    return NEURAX_SUCCESS;
}

// Convolution configuration validation
neurax_error_t neurax_validate_conv_config(const neurax_conv_config_t* config) {
    if (!config) {
        NEURAX_LOG_ERROR("Convolution config pointer is NULL");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->kernel_width == 0 || config->kernel_height == 0) {
        NEURAX_LOG_ERROR("Kernel dimensions cannot be zero");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->kernel_width > 11 || config->kernel_height > 11) {
        NEURAX_LOG_ERROR("Kernel size too large (max 11x11)");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->stride_x == 0 || config->stride_y == 0) {
        NEURAX_LOG_ERROR("Stride cannot be zero");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->stride_x > 8 || config->stride_y > 8) {
        NEURAX_LOG_ERROR("Stride too large (max 8)");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->input_channels == 0 || config->output_channels == 0) {
        NEURAX_LOG_ERROR("Channel count cannot be zero");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->activation > NEURAX_ACTIVATION_LINEAR) {
        NEURAX_LOG_ERROR("Invalid activation function");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    return NEURAX_SUCCESS;
}

// Pooling configuration validation
neurax_error_t neurax_validate_pool_config(const neurax_pool_config_t* config) {
    if (!config) {
        NEURAX_LOG_ERROR("Pooling config pointer is NULL");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->pool_width == 0 || config->pool_height == 0) {
        NEURAX_LOG_ERROR("Pool dimensions cannot be zero");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->pool_width > 8 || config->pool_height > 8) {
        NEURAX_LOG_ERROR("Pool size too large (max 8x8)");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->stride_x == 0 || config->stride_y == 0) {
        NEURAX_LOG_ERROR("Stride cannot be zero");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (config->pool_type > NEURAX_POOL_AVERAGE) {
        NEURAX_LOG_ERROR("Invalid pooling type");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    return NEURAX_SUCCESS;
}

// Optimal configuration
neurax_error_t neurax_get_optimal_config(neurax_device_t* device, neurax_config_t* config) {
    if (!device || !config) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    // Copy current configuration
    memcpy(config, &device->config, sizeof(neurax_config_t));
    
    // Optimize based on hardware capabilities
    if (device->hardware_available) {
        config->use_hardware = true;
        config->max_kernel_size = 11;
        config->num_multipliers = 64; // Assume 64 parallel multipliers
        config->data_type = NEURAX_DATA_INT16; // 16-bit for better precision
    } else {
        config->use_hardware = false;
        config->max_kernel_size = 11;
        config->num_multipliers = 1; // CPU serial processing
        config->data_type = NEURAX_DATA_FLOAT32; // Float for CPU
    }
    
    return NEURAX_SUCCESS;
}

// Benchmark layer performance
neurax_error_t neurax_benchmark_layer(neurax_device_t* device,
                                     const char* layer_type,
                                     uint32_t iterations,
                                     double* time_ms) {
    if (!device || !layer_type || !time_ms || iterations == 0) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    NEURAX_LOG_INFO("Benchmarking %s layer for %u iterations", layer_type, iterations);
    
    // Create test tensors
    neurax_tensor_t* input = NULL;
    neurax_tensor_t* output = NULL;
    neurax_error_t error;
    
    // Create 224x224x3 input tensor (typical image size)
    error = neurax_tensor_create(224, 224, 3, 1, NEURAX_DATA_FLOAT32, &input);
    if (error != NEURAX_SUCCESS) return error;
    
    // Fill with random data
    float* input_data = (float*)input->data;
    for (size_t i = 0; i < neurax_tensor_total_elements(input); i++) {
        input_data[i] = (float)rand() / RAND_MAX;
    }
    
    neurax_perf_stats_t stats;
    neurax_perf_start(&stats);
    
    if (strcmp(layer_type, "conv2d") == 0) {
        // Create output tensor for convolution
        error = neurax_tensor_create(222, 222, 64, 1, NEURAX_DATA_FLOAT32, &output);
        if (error != NEURAX_SUCCESS) goto cleanup;
        
        // Create weight tensor (3x3x3x64)
        neurax_tensor_t* weights = NULL;
        error = neurax_tensor_create(3, 3, 3, 64, NEURAX_DATA_FLOAT32, &weights);
        if (error != NEURAX_SUCCESS) goto cleanup;
        
        // Fill weights with random data
        float* weight_data = (float*)weights->data;
        for (size_t i = 0; i < neurax_tensor_total_elements(weights); i++) {
            weight_data[i] = (float)rand() / RAND_MAX - 0.5f;
        }
        
        // Configure convolution
        neurax_conv_config_t conv_config = {
            .kernel_width = 3,
            .kernel_height = 3,
            .stride_x = 1,
            .stride_y = 1,
            .padding_x = 0,
            .padding_y = 0,
            .input_channels = 3,
            .output_channels = 64,
            .use_bias = false,
            .activation = NEURAX_ACTIVATION_RELU
        };
        
        // Run benchmark
        for (uint32_t i = 0; i < iterations; i++) {
            error = neurax_conv2d(device, input, weights, NULL, &conv_config, output);
            if (error != NEURAX_SUCCESS) {
                neurax_tensor_destroy(weights);
                goto cleanup;
            }
        }
        
        neurax_tensor_destroy(weights);
        
    } else if (strcmp(layer_type, "pooling") == 0) {
        // Create output tensor for pooling
        error = neurax_tensor_create(112, 112, 3, 1, NEURAX_DATA_FLOAT32, &output);
        if (error != NEURAX_SUCCESS) goto cleanup;
        
        // Configure pooling
        neurax_pool_config_t pool_config = {
            .pool_width = 2,
            .pool_height = 2,
            .stride_x = 2,
            .stride_y = 2,
            .pool_type = NEURAX_POOL_MAX
        };
        
        // Run benchmark
        for (uint32_t i = 0; i < iterations; i++) {
            error = neurax_pooling(device, input, &pool_config, output);
            if (error != NEURAX_SUCCESS) goto cleanup;
        }
        
    } else if (strcmp(layer_type, "activation") == 0) {
        // Create output tensor for activation
        error = neurax_tensor_create(224, 224, 3, 1, NEURAX_DATA_FLOAT32, &output);
        if (error != NEURAX_SUCCESS) goto cleanup;
        
        // Run benchmark
        for (uint32_t i = 0; i < iterations; i++) {
            error = neurax_activation(device, input, NEURAX_ACTIVATION_RELU, output);
            if (error != NEURAX_SUCCESS) goto cleanup;
        }
        
    } else {
        error = NEURAX_ERROR_INVALID_PARAM;
        goto cleanup;
    }
    
    neurax_perf_end(&stats);
    
    *time_ms = stats.total_time_ms;
    
    NEURAX_LOG_INFO("Benchmark results:");
    NEURAX_LOG_INFO("  Total time: %.2f ms", stats.total_time_ms);
    NEURAX_LOG_INFO("  Average time per iteration: %.2f ms", stats.total_time_ms / iterations);
    
cleanup:
    if (input) neurax_tensor_destroy(input);
    if (output) neurax_tensor_destroy(output);
    
    return error;
}

// Memory allocation with alignment
neurax_error_t neurax_alloc_aligned(size_t size, size_t alignment, void** ptr) {
    if (!ptr || size == 0 || alignment == 0) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    // Use posix_memalign if available, otherwise use regular malloc
    #ifdef _POSIX_C_SOURCE
    if (posix_memalign(ptr, alignment, size) != 0) {
        return NEURAX_ERROR_MEMORY_ALLOCATION;
    }
    #else
    *ptr = malloc(size);
    if (!*ptr) {
        return NEURAX_ERROR_MEMORY_ALLOCATION;
    }
    #endif
    
    return NEURAX_SUCCESS;
}

// Free aligned memory
neurax_error_t neurax_free_aligned(void* ptr) {
    if (ptr) {
        free(ptr);
    }
    return NEURAX_SUCCESS;
}

// Data type conversion
neurax_error_t neurax_convert_data_type(const void* src, neurax_data_type_t src_type,
                                       void* dst, neurax_data_type_t dst_type,
                                       size_t num_elements) {
    if (!src || !dst || num_elements == 0) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    // If types are the same, just copy
    if (src_type == dst_type) {
        size_t element_size = neurax_get_element_size(src_type);
        memcpy(dst, src, num_elements * element_size);
        return NEURAX_SUCCESS;
    }
    
    // Convert each element
    for (size_t i = 0; i < num_elements; i++) {
        float temp_value = 0.0f;
        
        // Read source value
        switch (src_type) {
            case NEURAX_DATA_UINT8:
                temp_value = ((uint8_t*)src)[i];
                break;
            case NEURAX_DATA_INT8:
                temp_value = ((int8_t*)src)[i];
                break;
            case NEURAX_DATA_UINT16:
                temp_value = ((uint16_t*)src)[i];
                break;
            case NEURAX_DATA_INT16:
                temp_value = ((int16_t*)src)[i];
                break;
            case NEURAX_DATA_FLOAT32:
                temp_value = ((float*)src)[i];
                break;
        }
        
        // Write destination value
        switch (dst_type) {
            case NEURAX_DATA_UINT8:
                ((uint8_t*)dst)[i] = (uint8_t)fmax(0, fmin(255, temp_value));
                break;
            case NEURAX_DATA_INT8:
                ((int8_t*)dst)[i] = (int8_t)fmax(-128, fmin(127, temp_value));
                break;
            case NEURAX_DATA_UINT16:
                ((uint16_t*)dst)[i] = (uint16_t)fmax(0, fmin(65535, temp_value));
                break;
            case NEURAX_DATA_INT16:
                ((int16_t*)dst)[i] = (int16_t)fmax(-32768, fmin(32767, temp_value));
                break;
            case NEURAX_DATA_FLOAT32:
                ((float*)dst)[i] = temp_value;
                break;
        }
    }
    
    return NEURAX_SUCCESS;
}

size_t neurax_tensor_total_elements(const neurax_tensor_t* tensor) {
    if (!tensor) return 0;
    return tensor->width * tensor->height * tensor->channels * tensor->batch_size;
}
