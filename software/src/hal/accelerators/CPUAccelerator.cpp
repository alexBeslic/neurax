/**
 * @file CPUAccelerator.cpp
 * @brief CPU-optimized accelerator implementation
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "CPUAccelerator.hpp"
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <thread>       // For multi-threading
#include <future>       // For async operations

// Conditional SIMD includes
#ifdef __SSE__
#include <xmmintrin.h>  // SSE
#endif

namespace neurax {
namespace hal {

using neurax::tensor::Tensor;
using neurax::tensor::Shape;
using neurax::tensor::DataType;

CPUAccelerator::CPUAccelerator()
    : initialized_(false)
    , use_16bit_precision_(false)
    , debug_mode_(false)
    , operation_in_progress_(false) {

        bool success = initialize();
        if (!success) {
            throw std::runtime_error("Failed to initialize CPUAccelerator");
        }
}

CPUAccelerator::~CPUAccelerator() {
    cleanup();
}

bool CPUAccelerator::initialize() {
    if (initialized_) {
        return true;
    }

    // CPU accelerator always initializes successfully
    initialized_ = true;
    return true;
}

void CPUAccelerator::cleanup() {
    initialized_ = false;
}

bool CPUAccelerator::is_available() const {
    return true; // CPU accelerator is always available
}

AcceleratorStatus CPUAccelerator::get_status() const {
    AcceleratorStatus status;
    status.active_type = AcceleratorType::CPU_OPTIMIZED;
    status.requested_type = AcceleratorType::CPU_OPTIMIZED;
    status.using_fallback = false;
    status.is_available = true;
    status.last_error = last_error_;
    return status;
}

// Neural Network Operations - CPU optimized implementations

Tensor CPUAccelerator::convolution(const Tensor& input,
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

    // CPU-optimized convolution implementation
    return cpu_convolution_optimized(input, weights, bias, config);
}

Tensor CPUAccelerator::activation(const Tensor& input,
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

    // Use optimized SIMD activation when possible
    apply_activation_function_simd(output_data, input_data, num_elements, type);

    return output;
}

Tensor CPUAccelerator::batchnorm(const Tensor& input,
                                 const Tensor& gamma,
                                 const Tensor& beta,
                                 float epsilon,
                                 float momentum) {
    const auto& s = input.shape();
    if (s.size() == 4) {
        size_t N = s[0];
        size_t H = s[1];
        size_t W = s[2];
        size_t C = s[3];

        if (gamma.empty() || beta.empty()) {
            throw std::runtime_error("CPUAccelerator::batchnorm - gamma/beta empty");
        }
        if (gamma.numel() != C || beta.numel() != C) {
            throw std::runtime_error("CPUAccelerator::batchnorm - gamma/beta size mismatch");
        }

        Tensor output = Tensor::empty(s, input.dtype());
        const float* in_ptr = input.data_ptr<float>();
        float* out_ptr = output.data_ptr<float>();
        const float* gamma_ptr = gamma.data_ptr<float>();
        const float* beta_ptr = beta.data_ptr<float>();

        size_t spatial = N * H * W;
        std::vector<double> mean(C, 0.0), var(C, 0.0);

        for (size_t n = 0; n < N; ++n) for (size_t h = 0; h < H; ++h) for (size_t w = 0; w < W; ++w)
            for (size_t c = 0; c < C; ++c) mean[c] += in_ptr[((n * H + h) * W + w) * C + c];
        for (size_t c = 0; c < C; ++c) mean[c] /= static_cast<double>(spatial);

        for (size_t n = 0; n < N; ++n) for (size_t h = 0; h < H; ++h) for (size_t w = 0; w < W; ++w)
            for (size_t c = 0; c < C; ++c) {
                double d = in_ptr[((n * H + h) * W + w) * C + c] - mean[c];
                var[c] += d * d;
            }
        for (size_t c = 0; c < C; ++c) var[c] /= static_cast<double>(spatial);

        for (size_t n = 0; n < N; ++n) for (size_t h = 0; h < H; ++h) for (size_t w = 0; w < W; ++w)
            for (size_t c = 0; c < C; ++c) {
                size_t idx = ((n * H + h) * W + w) * C + c;
                float normalized = static_cast<float>((in_ptr[idx] - mean[c]) / std::sqrt(var[c] + epsilon));
                out_ptr[idx] = normalized * gamma_ptr[c] + beta_ptr[c];
            }

        return output;
    } else if (s.size() == 2) {
        size_t N = s[0];
        size_t F = s[1];
        if (gamma.empty() || beta.empty()) throw std::runtime_error("CPUAccelerator::batchnorm - gamma/beta empty");
        if (gamma.numel() != F || beta.numel() != F) throw std::runtime_error("CPUAccelerator::batchnorm - gamma/beta size mismatch");

        Tensor output = Tensor::empty(s, input.dtype());
        const float* in_ptr = input.data_ptr<float>();
        float* out_ptr = output.data_ptr<float>();
        const float* gamma_ptr = gamma.data_ptr<float>();
        const float* beta_ptr = beta.data_ptr<float>();

        std::vector<double> mean(F, 0.0), var(F, 0.0);
        for (size_t n = 0; n < N; ++n) for (size_t f = 0; f < F; ++f) mean[f] += in_ptr[n * F + f];
        for (size_t f = 0; f < F; ++f) mean[f] /= static_cast<double>(N);
        for (size_t n = 0; n < N; ++n) for (size_t f = 0; f < F; ++f) { double d = in_ptr[n * F + f] - mean[f]; var[f] += d * d; }
        for (size_t f = 0; f < F; ++f) var[f] /= static_cast<double>(N);
        for (size_t n = 0; n < N; ++n) for (size_t f = 0; f < F; ++f) out_ptr[n * F + f] = static_cast<float>((in_ptr[n * F + f] - mean[f]) / std::sqrt(var[f] + epsilon)) * gamma_ptr[f] + beta_ptr[f];
        return output;
    }

    throw std::runtime_error("CPUAccelerator::batchnorm - unsupported input rank");
}

Tensor CPUAccelerator::pooling(const Tensor& input,
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

Tensor CPUAccelerator::dense(const Tensor& input,
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
    // TODO: Add CPU optimizations like cache blocking and SIMD
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

bool CPUAccelerator::set_precision(bool use_16bit) {
    use_16bit_precision_ = use_16bit;
    return true;
}

bool CPUAccelerator::is_16bit_precision() const {
    return use_16bit_precision_;
}

void CPUAccelerator::set_debug_mode(bool enable) {
    debug_mode_ = enable;
}

bool CPUAccelerator::is_debug_mode() const {
    return debug_mode_;
}

// Private helper methods

void CPUAccelerator::validate_tensor_dimensions(const Tensor& tensor,
                                               const std::vector<size_t>& expected_dims,
                                               const std::string& operation_name) const {
    const auto& actual_dims = tensor.shape().dims();
    if (actual_dims.size() != expected_dims.size()) {
        throw std::runtime_error(operation_name + ": Tensor dimension mismatch");
    }

    for (size_t i = 0; i < expected_dims.size(); ++i) {
        if (expected_dims[i] != 0 && actual_dims[i] != expected_dims[i]) {
            throw std::runtime_error(operation_name + ": Tensor shape mismatch at dimension " + std::to_string(i));
        }
    }
}

void CPUAccelerator::debug_log(const std::string& message) const {
    if (debug_mode_) {
        std::cout << "[CPUAccelerator DEBUG] " << message << std::endl;
    }
}

void CPUAccelerator::set_error(const std::string& error, bool throw_exception) {
    last_error_ = error;
    if (throw_exception) {
        throw std::runtime_error(error);
    }
}

// CPU-optimized helper functions

Tensor CPUAccelerator::cpu_convolution_optimized(const Tensor& input,
                                                const Tensor& weights,
                                                const Tensor& bias,
                                                const ConvolutionConfig& config) {
    const auto& input_dims = input.shape().dims();
    const auto& weight_dims = weights.shape().dims();

    size_t batch_size = input_dims[0];
    size_t input_height = input_dims[1];
    size_t input_width = input_dims[2];
    size_t input_channels = input_dims[3];
    size_t output_channels = weight_dims[3];

    // Calculate output dimensions
    size_t output_height = (input_height + 2 * config.padding - config.kernel_size) / config.stride + 1;
    size_t output_width = (input_width + 2 * config.padding - config.kernel_size) / config.stride + 1;

    Shape output_shape({batch_size, output_height, output_width, output_channels});
    Tensor output = Tensor::zeros(output_shape, DataType::FLOAT32);

    const float* input_data = input.data_ptr<float>();
    const float* weight_data = weights.data_ptr<float>();
    const float* bias_data = bias.shape().numel() > 0 ? bias.data_ptr<float>() : nullptr;
    float* output_data = output.data_ptr<float>();

    // Determine if we should use multi-threading
    size_t total_operations = batch_size * output_height * output_width * output_channels;
    bool use_threading = total_operations > 50000; // Threshold for threading

    if (use_threading && std::thread::hardware_concurrency() > 1) {
        return cpu_convolution_threaded(input_data, weight_data, bias_data, output_data,
                                      batch_size, input_height, input_width, input_channels,
                                      output_height, output_width, output_channels, config);
    } else {
        return cpu_convolution_single_threaded(input_data, weight_data, bias_data, output_data,
                                             batch_size, input_height, input_width, input_channels,
                                             output_height, output_width, output_channels, config);
    }
}

Tensor CPUAccelerator::cpu_convolution_single_threaded(const float* input_data,
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
                                                      const ConvolutionConfig& config) {

    // Cache-friendly loop ordering: batch -> output_height -> output_width -> output_channels
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t out_h = 0; out_h < output_height; ++out_h) {
            for (size_t out_w = 0; out_w < output_width; ++out_w) {

                // Process multiple output channels together for better cache usage
                size_t out_c = 0;

                // SIMD processing for groups of 4 channels when possible
                #ifdef __SSE__
                for (; out_c + 3 < output_channels; out_c += 4) {
                    __m128 conv_sum = _mm_setzero_ps();

                    if (bias_data) {
                        conv_sum = _mm_loadu_ps(&bias_data[out_c]);
                    }

                    // Convolve over kernel window and input channels
                    for (size_t kh = 0; kh < config.kernel_size; ++kh) {
                        for (size_t kw = 0; kw < config.kernel_size; ++kw) {
                            int in_h = (int)(out_h * config.stride) + (int)kh - (int)config.padding;
                            int in_w = (int)(out_w * config.stride) + (int)kw - (int)config.padding;

                            if (in_h >= 0 && in_h < (int)input_height &&
                                in_w >= 0 && in_w < (int)input_width) {

                                for (size_t in_c = 0; in_c < input_channels; ++in_c) {
                                    size_t input_idx = ((b * input_height + in_h) * input_width + in_w) * input_channels + in_c;
                                    float input_val = input_data[input_idx];

                                    size_t weight_base = ((kh * config.kernel_size + kw) * input_channels + in_c) * output_channels + out_c;
                                    __m128 weight_vec = _mm_loadu_ps(&weight_data[weight_base]);
                                    __m128 input_vec = _mm_set1_ps(input_val);

                                    conv_sum = _mm_add_ps(conv_sum, _mm_mul_ps(input_vec, weight_vec));
                                }
                            }
                        }
                    }

                    size_t output_idx = ((b * output_height + out_h) * output_width + out_w) * output_channels + out_c;
                    _mm_storeu_ps(&output_data[output_idx], conv_sum);
                }
                #endif

                // Handle remaining channels
                for (; out_c < output_channels; ++out_c) {
                    float conv_sum = bias_data ? bias_data[out_c] : 0.0f;

                    for (size_t kh = 0; kh < config.kernel_size; ++kh) {
                        for (size_t kw = 0; kw < config.kernel_size; ++kw) {
                            int in_h = (int)(out_h * config.stride) + (int)kh - (int)config.padding;
                            int in_w = (int)(out_w * config.stride) + (int)kw - (int)config.padding;

                            if (in_h >= 0 && in_h < (int)input_height &&
                                in_w >= 0 && in_w < (int)input_width) {

                                for (size_t in_c = 0; in_c < input_channels; ++in_c) {
                                    size_t input_idx = ((b * input_height + in_h) * input_width + in_w) * input_channels + in_c;
                                    size_t weight_idx = ((kh * config.kernel_size + kw) * input_channels + in_c) * output_channels + out_c;

                                    conv_sum += input_data[input_idx] * weight_data[weight_idx];
                                }
                            }
                        }
                    }

                    size_t output_idx = ((b * output_height + out_h) * output_width + out_w) * output_channels + out_c;
                    output_data[output_idx] = conv_sum;
                }
            }
        }
    }

    Shape output_shape({batch_size, output_height, output_width, output_channels});
    Tensor result = Tensor::zeros(output_shape, DataType::FLOAT32);
    // Copy computed data
    std::memcpy(result.data_ptr<float>(), output_data, result.nbytes());
    return result;
}

Tensor CPUAccelerator::cpu_convolution_threaded(const float* input_data,
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
                                               const ConvolutionConfig& config) {

    size_t num_threads = std::min(std::thread::hardware_concurrency(), static_cast<unsigned int>(output_height));
    std::vector<std::future<void>> futures;

    size_t rows_per_thread = output_height / num_threads;

    for (size_t t = 0; t < num_threads; ++t) {
        size_t start_row = t * rows_per_thread;
        size_t end_row = (t == num_threads - 1) ? output_height : (t + 1) * rows_per_thread;

        futures.emplace_back(std::async(std::launch::async, [=]() {
            // Process subset of output rows
            for (size_t b = 0; b < batch_size; ++b) {
                for (size_t out_h = start_row; out_h < end_row; ++out_h) {
                    for (size_t out_w = 0; out_w < output_width; ++out_w) {
                        for (size_t out_c = 0; out_c < output_channels; ++out_c) {

                            float conv_sum = bias_data ? bias_data[out_c] : 0.0f;

                            for (size_t kh = 0; kh < config.kernel_size; ++kh) {
                                for (size_t kw = 0; kw < config.kernel_size; ++kw) {
                                    int in_h = (int)(out_h * config.stride) + (int)kh - (int)config.padding;
                                    int in_w = (int)(out_w * config.stride) + (int)kw - (int)config.padding;

                                    if (in_h >= 0 && in_h < (int)input_height &&
                                        in_w >= 0 && in_w < (int)input_width) {

                                        for (size_t in_c = 0; in_c < input_channels; ++in_c) {
                                            size_t input_idx = ((b * input_height + in_h) * input_width + in_w) * input_channels + in_c;
                                            size_t weight_idx = ((kh * config.kernel_size + kw) * input_channels + in_c) * output_channels + out_c;

                                            conv_sum += input_data[input_idx] * weight_data[weight_idx];
                                        }
                                    }
                                }
                            }

                            size_t output_idx = ((b * output_height + out_h) * output_width + out_w) * output_channels + out_c;
                            output_data[output_idx] = conv_sum;
                        }
                    }
                }
            }
        }));
    }

    // Wait for all threads to complete
    for (auto& future : futures) {
        future.wait();
    }

    Shape output_shape({batch_size, output_height, output_width, output_channels});
    Tensor result = Tensor::zeros(output_shape, DataType::FLOAT32);
    // Copy computed data
    std::memcpy(result.data_ptr<float>(), output_data, result.nbytes());
    return result;
}

void CPUAccelerator::apply_activation_function_simd(float* output_data,
                                                   const float* input_data,
                                                   size_t num_elements,
                                                   ActivationType type) {
    size_t i = 0;

    switch (type) {
        case ActivationType::RELU:
            // Try SIMD ReLU if available, otherwise fallback to scalar
            #ifdef __SSE__
            // SIMD ReLU - process 4 elements at a time
            for (; i + 3 < num_elements; i += 4) {
                __m128 input_vec = _mm_loadu_ps(&input_data[i]);
                __m128 zero_vec = _mm_setzero_ps();
                __m128 result = _mm_max_ps(input_vec, zero_vec);
                _mm_storeu_ps(&output_data[i], result);
            }
            #endif
            // Handle remaining elements
            for (; i < num_elements; ++i) {
                output_data[i] = std::max(0.0f, input_data[i]);
            }
            break;

        case ActivationType::LINEAR:
            // Optimized memcpy for linear activation
            std::memcpy(output_data, input_data, num_elements * sizeof(float));
            break;

        case ActivationType::SIGMOID:
        case ActivationType::TANH:
            // For sigmoid and tanh, use scalar math (SIMD versions are complex)
            for (i = 0; i < num_elements; ++i) {
                if (type == ActivationType::SIGMOID) {
                    output_data[i] = 1.0f / (1.0f + std::exp(-input_data[i]));
                } else {
                    output_data[i] = std::tanh(input_data[i]);
                }
            }
            break;

        default:
            throw std::runtime_error("Unsupported activation type");
    }
}

} // namespace hal
} // namespace neurax
