/*
 * NEURAX Convolution Implementation
 * Hardware and CPU implementations of 2D convolution
 * 
 * Author: NEURAX Team
 */

#include "neurax.h"
#include "neurax_private.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

// Execute convolution operation
neurax_error_t neurax_conv2d(neurax_device_t* device,
                            const neurax_tensor_t* input,
                            const neurax_tensor_t* weights,
                            const neurax_tensor_t* bias,
                            const neurax_conv_config_t* config,
                            neurax_tensor_t* output) {
    
    if (!device || !input || !weights || !config || !output) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (!device->initialized) {
        return NEURAX_ERROR_NOT_INITIALIZED;
    }
    
    // Validate inputs
    neurax_error_t error = neurax_validate_tensor(input);
    if (error != NEURAX_SUCCESS) return error;
    
    error = neurax_validate_tensor(weights);
    if (error != NEURAX_SUCCESS) return error;
    
    error = neurax_validate_tensor(output);
    if (error != NEURAX_SUCCESS) return error;
    
    error = neurax_validate_conv_config(config);
    if (error != NEURAX_SUCCESS) return error;
    
    // Check if bias is provided and valid
    if (bias && config->use_bias) {
        error = neurax_validate_tensor(bias);
        if (error != NEURAX_SUCCESS) return error;
    }
    
    NEURAX_LOG_INFO("Executing convolution: %dx%dx%d -> %dx%dx%d",
                    input->width, input->height, input->channels,
                    output->width, output->height, output->channels);
    
    // Choose implementation based on hardware availability
    if (device->hardware_available && device->config.use_hardware) {
        return neurax_hw_conv2d(device, input, weights, bias, config, output);
    } else {
        return neurax_cpu_conv2d(input, weights, bias, config, output);
    }
}

// Hardware implementation
neurax_error_t neurax_hw_conv2d(neurax_device_t* device,
                               const neurax_tensor_t* input,
                               const neurax_tensor_t* weights,
                               const neurax_tensor_t* bias,
                               const neurax_conv_config_t* config,
                               neurax_tensor_t* output) {
    
    NEURAX_LOG_DEBUG("Using hardware acceleration for convolution");
    
    // Configure hardware registers using bit fields
    neurax_conv_config_reg_t conv_config = {.raw = 0};
    conv_config.bits.kernel_size = config->kernel_width - 1;     // Bits 3:0
    conv_config.bits.stride = config->stride_x - 1;              // Bits 6:4
    conv_config.bits.padding = config->padding_x;                // Bits 8:7
    conv_config.bits.use_bias = config->use_bias ? 1 : 0;        // Bit 9
    conv_config.bits.input_channels = config->input_channels - 1; // Bits 12:10
    
    NEURAX_WRITE_REG(device, NEURAX_REG_CONV_CONFIG, conv_config.raw);
    
    // Set dimension configuration
    neurax_dim_config_reg_t dim_config = {.raw = 0};
    dim_config.bits.width = input->width;
    dim_config.bits.height = input->height;
    NEURAX_WRITE_REG(device, NEURAX_REG_DIM_CONFIG, dim_config.raw);
    
    // Set activation configuration
    neurax_act_config_reg_t act_config = {.raw = 0};
    act_config.bits.activation = config->activation;
    NEURAX_WRITE_REG(device, NEURAX_REG_ACT_CONFIG, act_config.raw);
    
    // Configure control register
    neurax_control_reg_t control = {.raw = 0};
    if (input->data_type == NEURAX_DATA_UINT16 || input->data_type == NEURAX_DATA_INT16) {
        control.bits.data_width = 1;
    }
    control.bits.conv_en = 1;
    if (config->activation != NEURAX_ACTIVATION_LINEAR) {
        control.bits.act_en = 1;
    }
    
    NEURAX_WRITE_REG(device, NEURAX_REG_CONTROL, control.raw);
    
    // TODO: Implement DMA data transfer
    // For now, we'll fall back to CPU implementation
    NEURAX_LOG_DEBUG("DMA transfer not implemented, falling back to CPU");
    
    return neurax_cpu_conv2d(input, weights, bias, config, output);
}

// CPU implementation
neurax_error_t neurax_cpu_conv2d(const neurax_tensor_t* input,
                                const neurax_tensor_t* weights,
                                const neurax_tensor_t* bias,
                                const neurax_conv_config_t* config,
                                neurax_tensor_t* output) {
    
    NEURAX_LOG_DEBUG("Using CPU implementation for convolution");
    
    // Calculate output dimensions
    uint32_t out_height = (input->height + 2 * config->padding_y - config->kernel_height) / config->stride_y + 1;
    uint32_t out_width = (input->width + 2 * config->padding_x - config->kernel_width) / config->stride_x + 1;
    
    if (output->height != out_height || output->width != out_width) {
        NEURAX_LOG_ERROR("Output tensor dimensions don't match calculated dimensions");
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    // Clear output tensor
    memset(output->data, 0, output->data_size);
    
    // Perform convolution for each batch
    for (uint32_t batch = 0; batch < input->batch_size; batch++) {
        
        // For each output channel
        for (uint32_t out_ch = 0; out_ch < config->output_channels; out_ch++) {
            
            // For each output position
            for (uint32_t out_y = 0; out_y < out_height; out_y++) {
                for (uint32_t out_x = 0; out_x < out_width; out_x++) {
                    
                    float accumulator = 0.0f;
                    
                    // Convolution operation
                    for (uint32_t in_ch = 0; in_ch < config->input_channels; in_ch++) {
                        for (uint32_t ky = 0; ky < config->kernel_height; ky++) {
                            for (uint32_t kx = 0; kx < config->kernel_width; kx++) {
                                
                                // Calculate input position
                                int32_t in_y = out_y * config->stride_y + ky - config->padding_y;
                                int32_t in_x = out_x * config->stride_x + kx - config->padding_x;
                                
                                // Check bounds
                                if (in_y >= 0 && in_y < (int32_t)input->height &&
                                    in_x >= 0 && in_x < (int32_t)input->width) {
                                    
                                    // Get input and weight values
                                    float input_val = neurax_get_tensor_value(input, batch, in_y, in_x, in_ch);
                                    float weight_val = neurax_get_weight_value(weights, out_ch, in_ch, ky, kx);
                                    
                                    accumulator += input_val * weight_val;
                                }
                            }
                        }
                    }
                    
                    // Add bias if enabled
                    if (config->use_bias && bias) {
                        float bias_val = neurax_get_bias_value(bias, out_ch);
                        accumulator += bias_val;
                    }
                    
                    // Apply activation function
                    float result = neurax_apply_activation(accumulator, config->activation);
                    
                    // Store result
                    neurax_set_tensor_value(output, batch, out_y, out_x, out_ch, result);
                }
            }
        }
    }
    
    return NEURAX_SUCCESS;
}

// Helper function to get tensor value
float neurax_get_tensor_value(const neurax_tensor_t* tensor, uint32_t batch, uint32_t y, uint32_t x, uint32_t c) {
    size_t index = ((batch * tensor->height + y) * tensor->width + x) * tensor->channels + c;
    
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

// Helper function to get weight value
float neurax_get_weight_value(const neurax_tensor_t* weights, uint32_t out_ch, uint32_t in_ch, uint32_t ky, uint32_t kx) {
    // Weights are stored as [output_channels, input_channels, kernel_height, kernel_width]
    size_t index = ((out_ch * weights->channels + in_ch) * weights->height + ky) * weights->width + kx;
    
    switch (weights->data_type) {
        case NEURAX_DATA_UINT8: {
            uint8_t* data = (uint8_t*)weights->data;
            return (float)data[index];
        }
        case NEURAX_DATA_INT8: {
            int8_t* data = (int8_t*)weights->data;
            return (float)data[index];
        }
        case NEURAX_DATA_UINT16: {
            uint16_t* data = (uint16_t*)weights->data;
            return (float)data[index];
        }
        case NEURAX_DATA_INT16: {
            int16_t* data = (int16_t*)weights->data;
            return (float)data[index];
        }
        case NEURAX_DATA_FLOAT32: {
            float* data = (float*)weights->data;
            return data[index];
        }
        default:
            return 0.0f;
    }
}

// Helper function to get bias value
float neurax_get_bias_value(const neurax_tensor_t* bias, uint32_t channel) {
    switch (bias->data_type) {
        case NEURAX_DATA_UINT8: {
            uint8_t* data = (uint8_t*)bias->data;
            return (float)data[channel];
        }
        case NEURAX_DATA_INT8: {
            int8_t* data = (int8_t*)bias->data;
            return (float)data[channel];
        }
        case NEURAX_DATA_UINT16: {
            uint16_t* data = (uint16_t*)bias->data;
            return (float)data[channel];
        }
        case NEURAX_DATA_INT16: {
            int16_t* data = (int16_t*)bias->data;
            return (float)data[channel];
        }
        case NEURAX_DATA_FLOAT32: {
            float* data = (float*)bias->data;
            return data[channel];
        }
        default:
            return 0.0f;
    }
}

// Helper function to set tensor value
void neurax_set_tensor_value(neurax_tensor_t* tensor, uint32_t batch, uint32_t y, uint32_t x, uint32_t c, float value) {
    size_t index = ((batch * tensor->height + y) * tensor->width + x) * tensor->channels + c;
    
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

// Helper function to apply activation
float neurax_apply_activation(float value, neurax_activation_t activation) {
    switch (activation) {
        case NEURAX_ACTIVATION_RELU:
            return fmax(0.0f, value);
        case NEURAX_ACTIVATION_TANH:
            return tanhf(value);
        case NEURAX_ACTIVATION_SIGMOID:
            return 1.0f / (1.0f + expf(-value));
        case NEURAX_ACTIVATION_LINEAR:
        default:
            return value;
    }
}

// Forward declarations for helper functions used in the file
float neurax_get_tensor_value(const neurax_tensor_t* tensor, uint32_t batch, uint32_t y, uint32_t x, uint32_t c);
float neurax_get_weight_value(const neurax_tensor_t* weights, uint32_t out_ch, uint32_t in_ch, uint32_t ky, uint32_t kx);
float neurax_get_bias_value(const neurax_tensor_t* bias, uint32_t channel);
void neurax_set_tensor_value(neurax_tensor_t* tensor, uint32_t batch, uint32_t y, uint32_t x, uint32_t c, float value);
float neurax_apply_activation(float value, neurax_activation_t activation);
