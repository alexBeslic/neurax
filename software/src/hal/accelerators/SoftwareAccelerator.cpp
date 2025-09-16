/**
 * @file SoftwareAccelerator.cpp
 * @brief Software-only accelerator implementation
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "SoftwareAccelerator.hpp"
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace neurax {
namespace hal {

using neurax::tensor::Tensor;
using neurax::tensor::Shape;
using neurax::tensor::DataType;

SoftwareAccelerator::SoftwareAccelerator()
    : initialized_(false)
    , use_16bit_precision_(false)
    , debug_mode_(false)
    , operation_in_progress_(false) {
}

SoftwareAccelerator::~SoftwareAccelerator() {
    cleanup();
}

bool SoftwareAccelerator::initialize() {
    if (initialized_) {
        return true;
    }

    // Software accelerator always initializes successfully
    initialized_ = true;
    return true;
}

void SoftwareAccelerator::cleanup() {
    initialized_ = false;
}

bool SoftwareAccelerator::is_available() const {
    return true; // Software fallback is always available
}

AcceleratorStatus SoftwareAccelerator::get_status() const {
    AcceleratorStatus status;
    status.active_type = AcceleratorType::SOFTWARE_FALLBACK;
    status.requested_type = AcceleratorType::SOFTWARE_FALLBACK;
    status.using_fallback = true;
    status.is_available = true;
    status.last_error = "";
    return status;
}

// Neural Network Operations - Basic implementations

Tensor SoftwareAccelerator::convolution(const Tensor& input,
                                      const Tensor& weights,
                                      const Tensor& bias,
                                      const ConvolutionConfig& config) {
    if (!initialized_) {
        throw std::runtime_error("Accelerator not initialized");
    }

    // Validate input dimensions (expecting 4D: batch, height, width, channels)
    if (input.shape().dims().size() != 4) {
        throw std::runtime_error("Input tensor must be 4D (batch, height, width, input_channels)");
    }

    // Validate weight dimensions (expecting 4D: kernel_height, kernel_width, input_channels, output_channels)
    if (weights.shape().dims().size() != 4) {
        throw std::runtime_error("Weight tensor must be 4D (kernel_height, kernel_width, input_channels, output_channels)");
    }

    const auto& input_dims = input.shape().dims();
    const auto& weight_dims = weights.shape().dims();

    size_t batch_size = input_dims[0];
    size_t input_height = input_dims[1];
    size_t input_width = input_dims[2];
    size_t input_channels = input_dims[3];

    size_t kernel_height = weight_dims[0];
    size_t kernel_width = weight_dims[1];
    size_t weight_input_channels = weight_dims[2];
    size_t output_channels = weight_dims[3];

    // Validate configuration
    if (kernel_height != config.kernel_size || kernel_width != config.kernel_size) {
        throw std::runtime_error("Weight kernel size must match config kernel size");
    }
    if (input_channels != weight_input_channels || input_channels != config.input_channels) {
        throw std::runtime_error("Input channels mismatch");
    }
    if (output_channels != config.output_channels) {
        throw std::runtime_error("Output channels mismatch");
    }

    // Calculate output dimensions
    size_t output_height = (input_height + 2 * config.padding - config.kernel_size) / config.stride + 1;
    size_t output_width = (input_width + 2 * config.padding - config.kernel_size) / config.stride + 1;

    Shape output_shape({batch_size, output_height, output_width, output_channels});
    Tensor output = Tensor::zeros(output_shape, DataType::FLOAT32);

    const float* input_data = input.data_ptr<float>();
    const float* weight_data = weights.data_ptr<float>();
    const float* bias_data = bias.shape().numel() > 0 ? bias.data_ptr<float>() : nullptr;
    float* output_data = output.data_ptr<float>();

    // Perform convolution
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t out_c = 0; out_c < output_channels; ++out_c) {
            for (size_t out_h = 0; out_h < output_height; ++out_h) {
                for (size_t out_w = 0; out_w < output_width; ++out_w) {

                    float conv_sum = 0.0f;

                    // Convolve over all input channels and kernel window
                    for (size_t in_c = 0; in_c < input_channels; ++in_c) {
                        for (size_t kh = 0; kh < kernel_height; ++kh) {
                            for (size_t kw = 0; kw < kernel_width; ++kw) {

                                // Calculate input position with padding
                                int in_h = (int)(out_h * config.stride) + (int)kh - (int)config.padding;
                                int in_w = (int)(out_w * config.stride) + (int)kw - (int)config.padding;

                                // Check bounds (padding with zeros)
                                if (in_h >= 0 && in_h < (int)input_height &&
                                    in_w >= 0 && in_w < (int)input_width) {

                                    size_t input_idx = ((b * input_height + in_h) * input_width + in_w) * input_channels + in_c;
                                    size_t weight_idx = ((kh * kernel_width + kw) * input_channels + in_c) * output_channels + out_c;

                                    conv_sum += input_data[input_idx] * weight_data[weight_idx];
                                }
                            }
                        }
                    }

                    // Add bias if provided
                    if (bias_data) {
                        conv_sum += bias_data[out_c];
                    }

                    size_t output_idx = ((b * output_height + out_h) * output_width + out_w) * output_channels + out_c;
                    output_data[output_idx] = conv_sum;
                }
            }
        }
    }

    return output;
}

Tensor SoftwareAccelerator::activation(const Tensor& input,
                                     ActivationType type) {
    if (!initialized_) {
        throw std::runtime_error("Accelerator not initialized");
    }

    // Create output tensor with same shape and data type
    Tensor output = Tensor::zeros(input.shape(), input.dtype());

    // Get data pointers
    const float* input_data = input.data_ptr<float>();
    float* output_data = output.data_ptr<float>();
    size_t num_elements = input.shape().numel();

    // Apply activation function based on type
    switch (type) {
        case ActivationType::RELU:
            for (size_t i = 0; i < num_elements; ++i) {
                output_data[i] = std::max(0.0f, input_data[i]);
            }
            break;

        case ActivationType::SIGMOID:
            for (size_t i = 0; i < num_elements; ++i) {
                output_data[i] = 1.0f / (1.0f + std::exp(-input_data[i]));
            }
            break;

        case ActivationType::TANH:
            for (size_t i = 0; i < num_elements; ++i) {
                output_data[i] = std::tanh(input_data[i]);
            }
            break;

        case ActivationType::LINEAR:
            std::memcpy(output_data, input_data, input.nbytes());
            break;

        default:
            throw std::runtime_error("Unsupported activation type");
    }

    return output;
}

Tensor SoftwareAccelerator::pooling(const Tensor& input,
                                  const PoolingConfig& config) {
    if (!initialized_) {
        throw std::runtime_error("Accelerator not initialized");
    }

    // Validate input dimensions (expecting 4D: batch, height, width, channels)
    if (input.shape().dims().size() != 4) {
        throw std::runtime_error("Input tensor must be 4D (batch, height, width, channels)");
    }

    const auto& input_dims = input.shape().dims();
    size_t batch_size = input_dims[0];
    size_t input_height = input_dims[1];
    size_t input_width = input_dims[2];
    size_t channels = input_dims[3];

    // Calculate output dimensions
    size_t output_height = (input_height - config.pool_size) / config.stride + 1;
    size_t output_width = (input_width - config.pool_size) / config.stride + 1;

    Shape output_shape({batch_size, output_height, output_width, channels});
    Tensor output = Tensor::zeros(output_shape, DataType::FLOAT32);

    const float* input_data = input.data_ptr<float>();
    float* output_data = output.data_ptr<float>();

    // Perform pooling operation
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t c = 0; c < channels; ++c) {
            for (size_t out_h = 0; out_h < output_height; ++out_h) {
                for (size_t out_w = 0; out_w < output_width; ++out_w) {

                    size_t in_h_start = out_h * config.stride;
                    size_t in_w_start = out_w * config.stride;

                    float pool_value = 0.0f;
                    bool first_value = true;
                    size_t count = 0;

                    // Pool over the kernel window
                    for (size_t kh = 0; kh < config.pool_size; ++kh) {
                        for (size_t kw = 0; kw < config.pool_size; ++kw) {
                            size_t in_h = in_h_start + kh;
                            size_t in_w = in_w_start + kw;

                            if (in_h < input_height && in_w < input_width) {
                                size_t input_idx = ((b * input_height + in_h) * input_width + in_w) * channels + c;
                                float value = input_data[input_idx];

                                if (config.type == PoolingType::MAX) {
                                    if (first_value || value > pool_value) {
                                        pool_value = value;
                                        first_value = false;
                                    }
                                } else if (config.type == PoolingType::AVERAGE) {
                                    pool_value += value;
                                    count++;
                                }
                            }
                        }
                    }

                    if (config.type == PoolingType::AVERAGE && count > 0) {
                        pool_value /= count;
                    }

                    size_t output_idx = ((b * output_height + out_h) * output_width + out_w) * channels + c;
                    output_data[output_idx] = pool_value;
                }
            }
        }
    }

    return output;
}

Tensor SoftwareAccelerator::dense(const Tensor& input,
                                const Tensor& weights,
                                const Tensor& bias) {
    if (!initialized_) {
        throw std::runtime_error("Accelerator not initialized");
    }

    // Validate input dimensions
    if (input.shape().dims().size() != 2) {
        throw std::runtime_error("Input tensor must be 2D (batch_size, input_features)");
    }
    if (weights.shape().dims().size() != 2) {
        throw std::runtime_error("Weight tensor must be 2D (input_features, output_features)");
    }

    const auto& input_shape = input.shape().dims();
    const auto& weight_shape = weights.shape().dims();

    size_t batch_size = input_shape[0];
    size_t input_features = input_shape[1];
    size_t output_features = weight_shape[1];

    // Check dimensions compatibility
    if (input_features != weight_shape[0]) {
        throw std::runtime_error("Input features must match weight input dimension");
    }

    // Check bias dimensions if provided
    if (bias.shape().numel() > 0) {
        if (bias.shape().dims().size() != 1 || bias.shape().dims()[0] != output_features) {
            throw std::runtime_error("Bias must be 1D with size equal to output features");
        }
    }

    // Create output tensor
    Shape output_shape({batch_size, output_features});
    Tensor output = Tensor::zeros(output_shape, DataType::FLOAT32);

    // Get data pointers
    const float* input_data = input.data_ptr<float>();
    const float* weight_data = weights.data_ptr<float>();
    const float* bias_data = bias.shape().numel() > 0 ? bias.data_ptr<float>() : nullptr;
    float* output_data = output.data_ptr<float>();

    // Perform matrix multiplication: output = input * weights + bias
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t out_f = 0; out_f < output_features; ++out_f) {
            float sum = 0.0f;

            // Dot product of input row with weight column
            for (size_t in_f = 0; in_f < input_features; ++in_f) {
                sum += input_data[b * input_features + in_f] *
                       weight_data[in_f * output_features + out_f];
            }

            // Add bias if provided
            if (bias_data) {
                sum += bias_data[out_f];
            }

            output_data[b * output_features + out_f] = sum;
        }
    }

    return output;
}

// Configuration methods

bool SoftwareAccelerator::set_precision(bool use_16bit) {
    use_16bit_precision_ = use_16bit;
    return true;
}

bool SoftwareAccelerator::is_16bit_precision() const {
    return use_16bit_precision_;
}

void SoftwareAccelerator::set_debug_mode(bool enable) {
    debug_mode_ = enable;
}

bool SoftwareAccelerator::is_debug_mode() const {
    return debug_mode_;
}

} // namespace hal
} // namespace neurax
