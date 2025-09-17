/**
 * @file test_hal.cpp
 * @brief Simple test program for HAL functionality
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "neurax/hal/AcceleratorFactory.hpp"
#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/Tensor.hpp"
#include <iostream>
#include <vector>

using namespace neurax;
using namespace neurax::hal;
using namespace neurax::tensor;

int main() {
    std::cout << "🚀 NEURAX HAL Test Program\n";
    std::cout << "=========================\n\n";

    try {
        // Test available accelerators
        std::cout << "📋 Available accelerators:\n";
        auto available = AcceleratorFactory::get_available_accelerators();
        for (auto type : available) {
            std::cout << "  - " << AcceleratorFactory::get_accelerator_name(type)
                      << ": " << AcceleratorFactory::get_accelerator_info(type) << "\n";
        }
        std::cout << "\n";

        // Create accelerator with fallback
        std::cout << "🔧 Creating accelerator with fallback...\n";
        auto accelerator = AcceleratorFactory::create(AcceleratorType::CPU_OPTIMIZED);

        if (!accelerator) {
            std::cerr << "❌ Failed to create any accelerator!\n";
            return 1;
        }

        std::cout << "✅ Created accelerator: "
                  << AcceleratorFactory::get_accelerator_name(accelerator->get_type()) << "\n\n";

        // Initialize the accelerator
        std::cout << "🔧 Initializing accelerator...\n";
        if (!accelerator->initialize()) {
            std::cerr << "❌ Failed to initialize accelerator!\n";
            return 1;
        }
        std::cout << "✅ Accelerator initialized successfully\n\n";

        // Test basic tensor operations
        std::cout << "🧮 Testing tensor operations...\n";

        // Create test tensor
        std::vector<size_t> shape = {1, 3, 3, 1};  // 3x3 single channel
        Tensor input = Tensor(shape, DataType::FLOAT32);
        // Fill with test data
        float* data = input.data_ptr<float>();
        for (size_t i = 0; i < input.numel(); ++i) {
            data[i] = static_cast<float>(i + 1);
        }
        std::cout << "Input Tensor:\n";
        for (size_t i = 0; i < input.numel(); ++i) {
            std::cout << data[i] << " ";
        }
        Tensor output = accelerator->applyActivation(input, ActivationType::SIGMOID);
        float* out_data = output.data_ptr<float>();
        std::cout << "\nOutput Tensor after ReLU:\n";
        for (size_t i = 0; i < input.numel(); ++i) {
            std::cout << out_data[i] << " ";
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ HAL test failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
