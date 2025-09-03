/*
 * NEURAX Activation and Pooling Implementation
 * 
 * Author: NEURAX Team
 */

#include "neurax.h"
#include "neurax_private.h"
#include <string.h>
#include <math.h>
#include <float.h>

// Activation function implementation
neurax_error_t neurax_activation(neurax_device_t* device,
                                const neurax_tensor_t* input,
                                neurax_activation_t activation,
                                neurax_tensor_t* output) {
    
    if (!device || !input || !output) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (!device->initialized) {
        return NEURAX_ERROR_NOT_INITIALIZED;
    }
    
    // Validate inputs
    neurax_error_t error = neurax_validate_tensor(input);
    if (error != NEURAX_SUCCESS) return error;
    
    error = neurax_validate_tensor(output);
    if (error != NEURAX_SUCCESS) return error;
    
    // Check tensor compatibility
    if (input->width != output->width || input->height != output->height ||
        input->channels != output->channels || input->batch_size != output->batch_size) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    NEURAX_LOG_INFO("Executing activation function: %d", activation);
    
    // Choose implementation
    if (device->hardware_available && device->config.use_hardware) {
        return neurax_hw_activation(device, input, activation, output);
    } else {
        return neurax_cpu_activation(input, activation, output);
    }
}

// Hardware activation implementation
neurax_error_t neurax_hw_activation(neurax_device_t* device,
                                   const neurax_tensor_t* input,
                                   neurax_activation_t activation,
                                   neurax_tensor_t* output) {
    
    NEURAX_LOG_DEBUG("Using hardware acceleration for activation");
    
    // Configure activation function
    uint32_t act_config = activation & 0x3;
    NEURAX_WRITE_REG(device, NEURAX_REG_ACT_CONFIG, act_config);
    
    // Enable only activation function
    uint32_t control = CTRL_ACT_EN;
    if (input->data_type == NEURAX_DATA_UINT16 || input->data_type == NEURAX_DATA_INT16) {
        control |= CTRL_DATA_WIDTH;
    }
    
    NEURAX_WRITE_REG(device, NEURAX_REG_CONTROL, control | CTRL_START);
    
    // Wait for completion
    neurax_error_t error = neurax_wait_for_completion(device, NEURAX_DEFAULT_TIMEOUT_MS);
    if (error != NEURAX_SUCCESS) {
        NEURAX_LOG_ERROR("Hardware activation timeout or error");
        return error;
    }
    
    // For now, fall back to CPU implementation until DMA is implemented
    return neurax_cpu_activation(input, activation, output);
}

// CPU activation implementation
neurax_error_t neurax_cpu_activation(const neurax_tensor_t* input,
                                    neurax_activation_t activation,
                                    neurax_tensor_t* output) {
    
    NEURAX_LOG_DEBUG("Using CPU implementation for activation");
    
    size_t total_elements = neurax_tensor_total_elements(input);
    
    for (size_t i = 0; i < total_elements; i++) {
        float input_val = neurax_get_tensor_element(input, i);
        float output_val = neurax_apply_activation(input_val, activation);
        neurax_set_tensor_element(output, i, output_val);
    }
    
    return NEURAX_SUCCESS;
}

// Pooling implementation
neurax_error_t neurax_pooling(neurax_device_t* device,
                             const neurax_tensor_t* input,
                             const neurax_pool_config_t* config,
                             neurax_tensor_t* output) {
    
    if (!device || !input || !config || !output) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (!device->initialized) {
        return NEURAX_ERROR_NOT_INITIALIZED;
    }
    
    // Validate inputs
    neurax_error_t error = neurax_validate_tensor(input);
    if (error != NEURAX_SUCCESS) return error;
    
    error = neurax_validate_tensor(output);
    if (error != NEURAX_SUCCESS) return error;
    
    error = neurax_validate_pool_config(config);
    if (error != NEURAX_SUCCESS) return error;
    
    NEURAX_LOG_INFO("Executing pooling: %dx%d, type=%d", 
                    config->pool_width, config->pool_height, config->pool_type);
    
    // Choose implementation
    if (device->hardware_available && device->config.use_hardware) {
        return neurax_hw_pooling(device, input, config, output);
    } else {
        return neurax_cpu_pooling(input, config, output);
    }
}

// Hardware pooling implementation
neurax_error_t neurax_hw_pooling(neurax_device_t* device,
                                const neurax_tensor_t* input,
                                const neurax_pool_config_t* config,
                                neurax_tensor_t* output) {
    
    NEURAX_LOG_DEBUG("Using hardware acceleration for pooling");
    
    // Configure pooling operation
    uint32_t pool_config = 0;
    pool_config |= (config->pool_type & 0x1);                          // Bit 0: pool type
    pool_config |= ((config->pool_width - 2) & 0x7) << 1;              // Bits 3:1: pool size
    pool_config |= ((config->stride_x - 1) & 0x7) << 4;                // Bits 6:4: stride
    
    NEURAX_WRITE_REG(device, NEURAX_REG_POOL_CONFIG, pool_config);
    
    // Set dimension configuration
    uint32_t dim_config = (input->width & 0xFFFF) | ((input->height & 0xFFFF) << 16);
    NEURAX_WRITE_REG(device, NEURAX_REG_DIM_CONFIG, dim_config);
    
    // Enable pooling
    uint32_t control = CTRL_POOL_EN;
    if (input->data_type == NEURAX_DATA_UINT16 || input->data_type == NEURAX_DATA_INT16) {
        control |= CTRL_DATA_WIDTH;
    }
    
    NEURAX_WRITE_REG(device, NEURAX_REG_CONTROL, control | CTRL_START);
    
    // Wait for completion
    neurax_error_t error = neurax_wait_for_completion(device, NEURAX_DEFAULT_TIMEOUT_MS);
    if (error != NEURAX_SUCCESS) {
        NEURAX_LOG_ERROR("Hardware pooling timeout or error");
        return error;
    }
    
    // For now, fall back to CPU implementation
    return neurax_cpu_pooling(input, config, output);
}

// CPU pooling implementation
neurax_error_t neurax_cpu_pooling(const neurax_tensor_t* input,
                                 const neurax_pool_config_t* config,
                                 neurax_tensor_t* output) {
    
    NEURAX_LOG_DEBUG("Using CPU implementation for pooling");
    
    // Calculate output dimensions
    uint32_t out_height = (input->height - config->pool_height) / config->stride_y + 1;
    uint32_t out_width = (input->width - config->pool_width) / config->stride_x + 1;
    
    if (output->height != out_height || output->width != out_width) {
        NEURAX_LOG_ERROR("Output tensor dimensions don't match calculated dimensions");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    // Clear output tensor
    memset(output->data, 0, output->data_size);
    
    // Perform pooling for each batch
    for (uint32_t batch = 0; batch < input->batch_size; batch++) {
        for (uint32_t ch = 0; ch < input->channels; ch++) {
            for (uint32_t out_y = 0; out_y < out_height; out_y++) {
                for (uint32_t out_x = 0; out_x < out_width; out_x++) {
                    
                    float pool_result;
                    uint32_t valid_count = 0;
                    
                    if (config->pool_type == NEURAX_POOL_MAX) {
                        pool_result = -FLT_MAX;
                    } else {
                        pool_result = 0.0f;
                    }
                    
                    // Pool window operation
                    for (uint32_t py = 0; py < config->pool_height; py++) {
                        for (uint32_t px = 0; px < config->pool_width; px++) {
                            
                            uint32_t in_y = out_y * config->stride_y + py;
                            uint32_t in_x = out_x * config->stride_x + px;
                            
                            if (in_y < input->height && in_x < input->width) {
                                float value = neurax_get_tensor_value(input, batch, in_y, in_x, ch);
                                
                                if (config->pool_type == NEURAX_POOL_MAX) {
                                    if (value > pool_result || valid_count == 0) {
                                        pool_result = value;
                                    }
                                } else { // Average pooling
                                    pool_result += value;
                                }
                                valid_count++;
                            }
                        }
                    }
                    
                    // Finalize result
                    if (config->pool_type == NEURAX_POOL_AVERAGE && valid_count > 0) {
                        pool_result /= valid_count;
                    }
                    
                    // Store result
                    neurax_set_tensor_value(output, batch, out_y, out_x, ch, pool_result);
                }
            }
        }
    }
    
    return NEURAX_SUCCESS;
}

// Helper function to get tensor element by linear index
float neurax_get_tensor_element(const neurax_tensor_t* tensor, size_t index) {
    switch (tensor->data_type) {
        case NEURAX_DATA_UINT8: {
            uint8_t* data = (uint8_t*)tensor->data;
            return (float)data[index];
        }
        case NEURAX_DATA_INT8: {
            int8_t* data = (int8_t*)tensor->data;
            return (float)data[index];
        }
        case NEURAX_DATA_UINT16: {
            uint16_t* data = (uint16_t*)tensor->data;
            return (float)data[index];
        }
        case NEURAX_DATA_INT16: {
            int16_t* data = (int16_t*)tensor->data;
            return (float)data[index];
        }
        case NEURAX_DATA_FLOAT32: {
            float* data = (float*)tensor->data;
            return data[index];
        }
        default:
            return 0.0f;
    }
}

// Helper function to set tensor element by linear index
void neurax_set_tensor_element(neurax_tensor_t* tensor, size_t index, float value) {
    switch (tensor->data_type) {
        case NEURAX_DATA_UINT8: {
            uint8_t* data = (uint8_t*)tensor->data;
            data[index] = (uint8_t)fmax(0, fmin(255, value));
            break;
        }
        case NEURAX_DATA_INT8: {
            int8_t* data = (int8_t*)tensor->data;
            data[index] = (int8_t)fmax(-128, fmin(127, value));
            break;
        }
        case NEURAX_DATA_UINT16: {
            uint16_t* data = (uint16_t*)tensor->data;
            data[index] = (uint16_t)fmax(0, fmin(65535, value));
            break;
        }
        case NEURAX_DATA_INT16: {
            int16_t* data = (int16_t*)tensor->data;
            data[index] = (int16_t)fmax(-32768, fmin(32767, value));
            break;
        }
        case NEURAX_DATA_FLOAT32: {
            float* data = (float*)tensor->data;
            data[index] = value;
            break;
        }
    }
}
