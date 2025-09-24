/**
 * @file test_refactored_network.cpp
 * @brief Test for refactored Network class using HAL
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "neurax/core/Network.hpp"
#include "neurax/core/Tensor.hpp"
#include "neurax/hal/AcceleratorFactory.hpp"
#include <iostream>

using namespace neurax;

int main() {
    std::cout << "🚀 Refactored Network Test\n";
    std::cout << "==========================\n\n";

    try {
        // Create network with preferred FPGA accelerator
        std::cout << "🔧 Creating network with FPGA preference...\n";
        NeuralNetwork network(hal::AcceleratorType::FPGA_DE1SOC);

        // Initialize network
        std::cout << "🔧 Initializing network...\n";
        if (!network.initialize()) {
            std::cerr << "❌ Failed to initialize network\n";
            return 1;
        }

        std::cout << "✅ Network initialized successfully\n\n";

        // Display network summary
        std::cout << "📊 Network Summary:\n";
        std::cout << network.get_summary() << "\n";

        // Test accelerator switching
        std::cout << "🔄 Testing accelerator switching...\n";

        auto original_type = network.get_accelerator_type();
        std::cout << "Original accelerator: "
                  << hal::AcceleratorFactory::get_accelerator_name(original_type) << "\n";

        // Try to switch to CPU (should fallback to software)
        bool switched = network.set_accelerator_type(hal::AcceleratorType::CPU_OPTIMIZED);
        auto new_type = network.get_accelerator_type();

        std::cout << "Attempted to switch to CPU, got: "
                  << hal::AcceleratorFactory::get_accelerator_name(new_type)
                  << " (success: " << (switched ? "Yes" : "No") << ")\n";

        // Test precision setting
        std::cout << "\n🎯 Testing precision settings...\n";
        std::cout << "Current precision: " << (network.is_16bit_precision() ? "16-bit" : "8-bit") << "\n";

        network.set_precision(true);
        std::cout << "After setting 16-bit: " << (network.is_16bit_precision() ? "16-bit" : "8-bit") << "\n";

        network.set_precision(false);
        std::cout << "After setting 8-bit: " << (network.is_16bit_precision() ? "16-bit" : "8-bit") << "\n";

        // Test debug mode
        std::cout << "\n🐛 Testing debug mode...\n";
        network.set_debug_mode(true);
        std::cout << "Debug mode enabled\n";

        network.set_debug_mode(false);
        std::cout << "Debug mode disabled\n";

        // Test accelerator status
        std::cout << "\n📊 Accelerator Status:\n";
        auto status = network.get_accelerator_status();
        std::cout << "  Active: " << hal::AcceleratorFactory::get_accelerator_name(status.active_type) << "\n";
        std::cout << "  Requested: " << hal::AcceleratorFactory::get_accelerator_name(status.requested_type) << "\n";
        std::cout << "  Using fallback: " << (status.using_fallback ? "Yes" : "No") << "\n";
        std::cout << "  Available: " << (status.is_available ? "Yes" : "No") << "\n";

        if (!status.last_error.empty()) {
            std::cout << "  Last error: " << status.last_error << "\n";
        }

        // Test available accelerators
        std::cout << "\n🏗️ Available accelerators on this system:\n";
        auto available = NeuralNetwork::get_available_accelerators();
        for (auto type : available) {
            std::cout << "  - " << hal::AcceleratorFactory::get_accelerator_name(type) << "\n";
        }

        // Cleanup
        network.cleanup();
        std::cout << "\n✅ Refactored Network test completed successfully!\n";

    } catch (const std::exception& e) {
        std::cerr << "❌ Network test failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
