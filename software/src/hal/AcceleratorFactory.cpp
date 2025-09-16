/**
 * @file AcceleratorFactory.cpp
 * @brief Implementation of AcceleratorFactory
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "neurax/hal/AcceleratorFactory.hpp"
#include "accelerators/SoftwareAccelerator.hpp"
#include "accelerators/FPGAAccelerator.hpp"
#include "accelerators/CPUAccelerator.hpp"
#include "neurax/tensor/DataType.hpp"
#include <iostream>
#include <algorithm>

namespace neurax {
namespace hal {

using neurax::tensor::Tensor;
using neurax::tensor::Shape;
using neurax::tensor::DataType;

std::unique_ptr<IAccelerator> AcceleratorFactory::create(AcceleratorType type) {
    switch (type) {
        case AcceleratorType::SOFTWARE_FALLBACK:
            return std::make_unique<SoftwareAccelerator>();

        case AcceleratorType::FPGA_DE1SOC:
            try {
                return std::make_unique<FPGAAccelerator>();
            } catch (const std::exception& e) {
                std::cerr << "Failed to create FPGA accelerator: " << e.what() << std::endl;
                return nullptr;
            }

        case AcceleratorType::GPU_OPENCL:
            // TODO: Implement GPUAccelerator
            std::cerr << "GPU accelerator not yet implemented, falling back to software\n";
            return nullptr;

        case AcceleratorType::CPU_OPTIMIZED:
            return std::make_unique<CPUAccelerator>();

        default:
            std::cerr << "Unknown accelerator type, falling back to software\n";
            return nullptr;
    }
}

std::vector<AcceleratorType> AcceleratorFactory::get_available_accelerators() {
    std::vector<AcceleratorType> available;

    // Test each accelerator type
    std::vector<AcceleratorType> all_types = {
        AcceleratorType::FPGA_DE1SOC,
        AcceleratorType::GPU_OPENCL,
        AcceleratorType::CPU_OPTIMIZED,
        AcceleratorType::SOFTWARE_FALLBACK
    };

    for (auto type : all_types) {
        if (is_available(type)) {
            available.push_back(type);
        }
    }

    return available;
}

AcceleratorType AcceleratorFactory::get_best_available() {
    // Priority order: FPGA > GPU > CPU > Software
    std::vector<AcceleratorType> priority_order = {
        AcceleratorType::FPGA_DE1SOC,
        AcceleratorType::GPU_OPENCL,
        AcceleratorType::CPU_OPTIMIZED,
        AcceleratorType::SOFTWARE_FALLBACK
    };

    for (auto type : priority_order) {
        if (is_available(type)) {
            return type;
        }
    }

    // This should never happen since SOFTWARE_FALLBACK is always available
    return AcceleratorType::SOFTWARE_FALLBACK;
}

bool AcceleratorFactory::is_available(AcceleratorType type) {
    auto accelerator = try_create(type);
    if (!accelerator) {
        return false;
    }

    return test_accelerator(accelerator.get());
}

std::string AcceleratorFactory::get_accelerator_name(AcceleratorType type) {
    switch (type) {
        case AcceleratorType::FPGA_DE1SOC:
            return "FPGA DE1-SoC";
        case AcceleratorType::GPU_OPENCL:
            return "GPU OpenCL";
        case AcceleratorType::CPU_OPTIMIZED:
            return "CPU Optimized";
        case AcceleratorType::SOFTWARE_FALLBACK:
            return "Software Fallback";
        default:
            return "Unknown";
    }
}

std::string AcceleratorFactory::get_accelerator_info(AcceleratorType type) {
    switch (type) {
        case AcceleratorType::FPGA_DE1SOC:
            return "DE1-SoC FPGA accelerator with custom neural network hardware";

        case AcceleratorType::GPU_OPENCL:
            return "GPU accelerator using OpenCL for parallel computation";

        case AcceleratorType::CPU_OPTIMIZED:
            return "CPU accelerator with SIMD optimizations and threading";

        case AcceleratorType::SOFTWARE_FALLBACK:
            return "Pure C++ software implementation (always available)";

        default:
            return "Unknown accelerator type";
    }
}

// Private helper methods

std::unique_ptr<IAccelerator> AcceleratorFactory::try_create(AcceleratorType type) {
    try {
        return create(type);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create accelerator " << get_accelerator_name(type)
                  << ": " << e.what() << std::endl;
        return nullptr;
    }
}

bool AcceleratorFactory::test_accelerator(IAccelerator* accelerator) {
    if (!accelerator) {
        return false;
    }

    try {
        // Test initialization
        if (!accelerator->initialize()) {
            return false;
        }

        // Test availability
        if (!accelerator->is_available()) {
            accelerator->cleanup();
            return false;
        }

        // Test basic functionality with a small tensor
        Shape test_shape({1, 2, 2, 1});  // Small 2x2 tensor
        Tensor test_input = Tensor::ones(test_shape, DataType::FLOAT32);

        // Test activation function (simplest operation)
        auto result = accelerator->activation(test_input, ActivationType::RELU);

        // If we get here without exception, accelerator works
        accelerator->cleanup();
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Accelerator test failed: " << e.what() << std::endl;
        try {
            accelerator->cleanup();
        } catch (...) {
            // Ignore cleanup errors during test failure
        }
        return false;
    }
}

} // namespace hal
} // namespace neurax
