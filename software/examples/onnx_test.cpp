/**
 * @file onnx_test.cpp
 * @brief ONNX Parser test application for NEURAX inference engine
 *
 * Demonstrates loading an ONNX model, running inference, and
 * computing argmax for classification tasks.
 *
 * @author NEURAX Development Team
 * @date January 2026
 * @version 1.0
 *
 * Usage:
 *   ./example_onnx_test [model.onnx]
 *
 * If no model path is provided, it will look for "model.onnx" in the
 * current directory or examples/data/ directory.
 */

#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cmath>

#include "neurax/parser/ONNXParser.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

using namespace neurax;

/**
 * @brief Compute argmax of a 1D or flattened tensor
 * @param tensor Output tensor from the network
 * @return Index of maximum element
 */
size_t argmax(const tensor::Tensor& tensor) {
    const float* data = tensor.data_ptr<float>();
    size_t num_elements = tensor.numel();
    
    if (num_elements == 0) {
        return 0;
    }
    
    size_t max_idx = 0;
    float max_val = data[0];
    
    for (size_t i = 1; i < num_elements; ++i) {
        if (data[i] > max_val) {
            max_val = data[i];
            max_idx = i;
        }
    }
    
    return max_idx;
}

/**
 * @brief Compute top-k predictions
 * @param tensor Output tensor from the network
 * @param k Number of top predictions to return
 * @return Vector of (index, probability) pairs
 */
std::vector<std::pair<size_t, float>> topk(const tensor::Tensor& tensor, size_t k) {
    const float* data = tensor.data_ptr<float>();
    size_t num_elements = tensor.numel();
    
    // Create index vector
    std::vector<size_t> indices(num_elements);
    std::iota(indices.begin(), indices.end(), 0);
    
    // Partial sort to get top-k
    k = std::min(k, num_elements);
    std::partial_sort(indices.begin(), indices.begin() + k, indices.end(),
        [data](size_t a, size_t b) { return data[a] > data[b]; });
    
    // Collect results
    std::vector<std::pair<size_t, float>> result;
    for (size_t i = 0; i < k; ++i) {
        result.emplace_back(indices[i], data[indices[i]]);
    }
    
    return result;
}

/**
 * @brief Apply softmax to tensor values (in-place conceptually)
 * @param tensor Input/output tensor
 */
void softmax_inplace(tensor::Tensor& tensor) {
    float* data = tensor.data_ptr<float>();
    size_t num_elements = tensor.numel();
    
    // Find max for numerical stability
    float max_val = *std::max_element(data, data + num_elements);
    
    // Compute exp and sum
    float sum = 0.0f;
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = std::exp(data[i] - max_val);
        sum += data[i];
    }
    
    // Normalize
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] /= sum;
    }
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [model.onnx]\n"
              << "\nOptions:\n"
              << "  -h, --help       Show this help message\n"
              << "  -v, --verbose    Verbose output\n"
              << "  -a, --accelerator TYPE  Accelerator type (software, fpga, cpu)\n"
              << "  -b, --batch SIZE Batch size (default: 1)\n"
              << "\nExample:\n"
              << "  " << program_name << " resnet18.onnx\n"
              << "  " << program_name << " -a fpga mobilenet_v2.onnx\n";
}

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "  NEURAX ONNX Parser Test Application\n";
    std::cout << "========================================\n\n";

    // Default values
    std::string model_path = "model.onnx";
    bool verbose = false;
    hal::AcceleratorType accel_type = hal::AcceleratorType::SOFTWARE_FALLBACK;
    size_t batch_size = 1;

    // Simple argument parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if ((arg == "-a" || arg == "--accelerator") && i + 1 < argc) {
            std::string accel = argv[++i];
            if (accel == "fpga") {
                accel_type = hal::AcceleratorType::FPGA_DE1SOC;
            } else if (accel == "cpu") {
                accel_type = hal::AcceleratorType::CPU_OPTIMIZED;
            } else {
                accel_type = hal::AcceleratorType::SOFTWARE_FALLBACK;
            }
        } else if ((arg == "-b" || arg == "--batch") && i + 1 < argc) {
            batch_size = std::stoul(argv[++i]);
        } else if (arg[0] != '-') {
            model_path = arg;
        }
    }

    // Try alternate paths if default not found
    std::vector<std::string> search_paths = {
        model_path,
        "examples/data/" + model_path,
        "../examples/data/" + model_path,
        "data/" + model_path
    };

    std::string found_path;
    for (const auto& path : search_paths) {
        std::ifstream test(path);
        if (test.good()) {
            found_path = path;
            break;
        }
    }

    if (found_path.empty()) {
        std::cerr << "Error: Model file not found: " << model_path << "\n";
        std::cerr << "Please provide a valid ONNX model file.\n";
        return 1;
    }

    std::cout << "Loading model: " << found_path << "\n";

    try {
        // Configure parser
        parser::ONNXParserConfig config;
        config.accelerator_type = accel_type;
        config.strict_mode = false;  // Allow skipping unsupported ops
        
        // Create parser and load model
        parser::ONNXParser parser(config);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto network = parser.LoadModel(found_path);
        auto load_time = std::chrono::high_resolution_clock::now();
        
        auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            load_time - start_time).count();
        
        std::cout << "Model loaded successfully in " << load_duration << " ms\n\n";

        // Print model info
        if (verbose) {
            std::cout << "Model Metadata:\n";
            for (const auto& [key, value] : parser.getModelMetadata()) {
                std::cout << "  " << key << ": " << value << "\n";
            }
            std::cout << "\n";
        }

        // Print input/output info
        std::cout << "Model Inputs:\n";
        for (const auto& name : parser.getInputNames()) {
            std::cout << "  - " << name;
            if (parser.hasShape(name)) {
                const auto& onnx_shape = parser.getShape(name);
                std::cout << " (ONNX shape: [";
                for (size_t i = 0; i < onnx_shape.size(); ++i) {
                    std::cout << onnx_shape[i];
                    if (i < onnx_shape.size() - 1) std::cout << ", ";
                }
                std::cout << "])";
            }
            std::cout << "\n";
        }
        
        std::cout << "\nModel Outputs:\n";
        for (const auto& name : parser.getOutputNames()) {
            std::cout << "  - " << name << "\n";
        }
        std::cout << "\n";

        // Get input shape from the model
        // ONNX uses NCHW format [batch, channels, height, width]
        // NEURAX uses NHWC format [batch, height, width, channels]
        const auto& input_name = parser.getInputNames().front();
        tensor::Shape onnx_shape = parser.getShape(input_name);
        
        // Convert from NCHW to NHWC
        tensor::Shape input_shape;
        if (onnx_shape.size() == 4) {
            // NCHW -> NHWC: [N, C, H, W] -> [N, H, W, C]
            input_shape = tensor::Shape({
                onnx_shape[0] > 0 ? onnx_shape[0] : batch_size,  // N (use batch_size if dynamic)
                onnx_shape[2],   // H
                onnx_shape[3],   // W
                onnx_shape[1]    // C
            });
        } else {
            // Use ONNX shape directly for non-4D tensors
            std::vector<size_t> dims;
            for (size_t i = 0; i < onnx_shape.size(); ++i) {
                dims.push_back(onnx_shape[i]);
            }
            input_shape = tensor::Shape(dims);
        }

        tensor::Tensor input = tensor::Tensor::zeros(input_shape, tensor::DataType::FLOAT32);
        
        // Fill with some test data (normalized random-ish values)
        float* input_data = input.data_ptr<float>();
        for (size_t i = 0; i < input.numel(); ++i) {
            // Simple pattern for reproducible testing
            input_data[i] = static_cast<float>((i % 256) - 128) / 255.0f;
        }

        std::cout << "Input tensor shape: [";
        for (size_t i = 0; i < input_shape.size(); ++i) {
            std::cout << input_shape[i];
            if (i < input_shape.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
        std::cout << "Input tensor size: " << input.nbytes() << " bytes\n\n";

        // Run inference
        std::cout << "Running inference...\n";
        
        auto infer_start = std::chrono::high_resolution_clock::now();
        tensor::Tensor output = network->infer(input);
        auto infer_end = std::chrono::high_resolution_clock::now();
        
        auto infer_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            infer_end - infer_start).count();

        std::cout << "Inference completed in " << infer_duration << " us\n\n";

        // Print output info
        std::cout << "Output tensor shape: [";
        for (size_t i = 0; i < output.shape().size(); ++i) {
            std::cout << output.shape()[i];
            if (i < output.shape().size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
        std::cout << "Output tensor size: " << output.nbytes() << " bytes\n\n";

        // Compute classification results
        // Apply softmax if output doesn't seem to be probabilities
        tensor::Tensor probs = output;  // Copy for softmax
        const float* out_data = output.data_ptr<float>();
        float out_sum = 0.0f;
        for (size_t i = 0; i < std::min(size_t(10), output.numel()); ++i) {
            out_sum += out_data[i];
        }
        
        // If values don't look like probabilities, apply softmax
        if (out_sum < 0.5f || out_sum > 1.5f) {
            softmax_inplace(probs);
        }

        // Compute argmax
        size_t predicted_class = argmax(probs);
        std::cout << "Predicted class index: " << predicted_class << "\n";

        // Show top-5 predictions
        std::cout << "\nTop-5 Predictions:\n";
        auto top5 = topk(probs, 5);
        for (size_t i = 0; i < top5.size(); ++i) {
            std::cout << "  " << (i + 1) << ". Class " << std::setw(4) << top5[i].first
                      << " - Confidence: " << std::fixed << std::setprecision(4)
                      << (top5[i].second * 100.0f) << "%\n";
        }

        // Benchmark multiple runs
        std::cout << "\n--- Benchmark (10 runs) ---\n";
        
        std::vector<long long> timings;
        for (int run = 0; run < 10; ++run) {
            auto bench_start = std::chrono::high_resolution_clock::now();
            tensor::Tensor bench_output = network->infer(input);
            auto bench_end = std::chrono::high_resolution_clock::now();
            (void)bench_output;  // Suppress unused warning
            
            timings.push_back(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    bench_end - bench_start).count());
        }
        
        // Compute statistics
        std::sort(timings.begin(), timings.end());
        long long total = std::accumulate(timings.begin(), timings.end(), 0LL);
        double mean = static_cast<double>(total) / timings.size();
        double median = static_cast<double>(timings[timings.size() / 2]);
        
        std::cout << "  Mean:   " << std::fixed << std::setprecision(2) << mean << " us\n";
        std::cout << "  Median: " << median << " us\n";
        std::cout << "  Min:    " << timings.front() << " us\n";
        std::cout << "  Max:    " << timings.back() << " us\n";
        std::cout << "  Throughput: " << std::setprecision(1)
                  << (1000000.0 / mean) << " inferences/sec\n";

        std::cout << "\n========================================\n";
        std::cout << "  Test completed successfully!\n";
        std::cout << "========================================\n";

        return 0;

    } catch (const parser::ONNXParseException& e) {
        std::cerr << "ONNX Parse Error: " << e.what() << "\n";
        return 1;
    } catch (const tensor::TensorException& e) {
        std::cerr << "Tensor Error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
