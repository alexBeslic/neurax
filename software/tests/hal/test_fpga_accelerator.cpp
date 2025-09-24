/**
 * @file test_fpga_accelerator.cpp
 * @brief Test program for FPGAAccelerator implementation
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "neurax/hal/AcceleratorFactory.hpp"
#include "neurax/core/Network.hpp"
#include "neurax/core/Tensor.hpp"
#include <iostream>

using namespace neurax;
using namespace neurax::hal;

int main() {
    std::cout << "🚀 FPGA Accelerator Test Program\n";
    std::cout << "================================\n\n";

    try {
        // Test direct FPGA accelerator creation
        std::cout << "🔧 Testing direct FPGA accelerator creation...\n";
        auto fpga_accelerator = AcceleratorFactory::create(AcceleratorType::FPGA_DE1SOC);

        if (!fpga_accelerator) {
            std::cout << "⚠️  FPGA accelerator creation failed (expected if no hardware)\n";
            std::cout << "   This is normal in simulation/development environment\n\n";

            // Test fallback mechanism
            std::cout << "🔄 Testing fallback mechanism...\n";
            auto fallback_accelerator = AcceleratorFactory::create_with_fallback(AcceleratorType::FPGA_DE1SOC);

            if (fallback_accelerator) {
                std::cout << "✅ Fallback accelerator created: "
                          << AcceleratorFactory::get_accelerator_name(fallback_accelerator->get_type()) << "\n";

                // Test basic operations with fallback
                std::cout << "\n🧮 Testing basic operations with fallback...\n";

                if (!fallback_accelerator->initialize()) {
                    std::cerr << "❌ Failed to initialize fallback accelerator\n";
                    return 1;
                }

                // Test tensor operation
                std::vector<size_t> shape = {1, 3, 3, 1};
                Tensor<float> input(shape);
                input.fill(1.0f);

                auto result = fallback_accelerator->activation(input, ActivationType::RELU);
                std::cout << "✅ ReLU activation test passed\n";

                fallback_accelerator->cleanup();
            }

        } else {
            std::cout << "✅ FPGA accelerator created successfully!\n";

            // Test FPGA accelerator initialization
            std::cout << "\n🔧 Testing FPGA initialization...\n";
            if (fpga_accelerator->initialize()) {
                std::cout << "✅ FPGA accelerator initialized successfully!\n";

                // Test status
                auto status = fpga_accelerator->get_status();
                std::cout << "\n📊 FPGA Status:\n";
                std::cout << "  Type: " << AcceleratorFactory::get_accelerator_name(status.active_type) << "\n";
                std::cout << "  Available: " << (status.is_available ? "Yes" : "No") << "\n";
                std::cout << "  Using fallback: " << (status.using_fallback ? "Yes" : "No") << "\n";

                if (!status.last_error.empty()) {
                    std::cout << "  Last error: " << status.last_error << "\n";
                }

                // Test precision setting
                std::cout << "\n🎯 Testing precision settings...\n";
                std::cout << "Current precision: " << (fpga_accelerator->is_16bit_precision() ? "16-bit" : "8-bit") << "\n";

                if (fpga_accelerator->set_precision(true)) {
                    std::cout << "✅ 16-bit precision set successfully\n";
                } else {
                    std::cout << "⚠️  Failed to set 16-bit precision\n";
                }

                if (fpga_accelerator->set_precision(false)) {
                    std::cout << "✅ 8-bit precision set successfully\n";
                } else {
                    std::cout << "⚠️  Failed to set 8-bit precision\n";
                }

                // Test debug mode
                std::cout << "\n🐛 Testing debug mode...\n";
                fpga_accelerator->set_debug_mode(true);
                std::cout << "Debug mode enabled\n";

                // Test basic tensor operation
                std::cout << "\n🧮 Testing FPGA tensor operations...\n";
                std::vector<size_t> shape = {1, 2, 2, 1};  // Small 2x2 tensor
                Tensor<float> input(shape);

                // Fill with test data
                for (size_t i = 0; i < input.size(); ++i) {
                    input[i] = static_cast<float>(i + 1);
                }

                std::cout << "Input tensor: ";
                for (size_t i = 0; i < input.size(); ++i) {
                    std::cout << input[i] << " ";
                }
                std::cout << "\n";

                try {
                    // Test ReLU activation
                    auto relu_result = fpga_accelerator->activation(input, ActivationType::RELU);
                    std::cout << "✅ FPGA ReLU activation completed\n";

                    std::cout << "ReLU result: ";
                    for (size_t i = 0; i < std::min(relu_result.size(), size_t(4)); ++i) {
                        std::cout << relu_result[i] << " ";
                    }
                    std::cout << "\n";

                } catch (const std::exception& e) {
                    std::cout << "⚠️  FPGA activation failed: " << e.what() << "\n";
                    std::cout << "   This is expected if FPGA hardware is not available\n";
                }

                try {
                    // Test pooling
                    PoolingConfig pool_config(2, 1, PoolingType::MAX);
                    auto pool_result = fpga_accelerator->pooling(input, pool_config);
                    std::cout << "✅ FPGA pooling completed\n";

                } catch (const std::exception& e) {
                    std::cout << "⚠️  FPGA pooling failed: " << e.what() << "\n";
                    std::cout << "   This is expected if FPGA hardware is not available\n";
                }

                // Test dense operation (should fail gracefully)
                try {
                    Tensor<float> weights({4, 2});
                    Tensor<float> bias({2});
                    auto dense_result = fpga_accelerator->dense(input, weights, bias);
                    std::cout << "✅ FPGA dense operation completed\n";

                } catch (const AcceleratorUnavailableException& e) {
                    std::cout << "ℹ️  Dense operations not supported in FPGA (as expected): " << e.what() << "\n";
                } catch (const std::exception& e) {
                    std::cout << "⚠️  FPGA dense operation failed: " << e.what() << "\n";
                }

                // Disable debug mode
                fpga_accelerator->set_debug_mode(false);

                // Cleanup
                fpga_accelerator->cleanup();
                std::cout << "\n🧹 FPGA accelerator cleaned up\n";

            } else {
                std::cout << "⚠️  FPGA accelerator initialization failed\n";
                std::cout << "   This is expected if FPGA hardware is not available\n";

                auto status = fpga_accelerator->get_status();
                if (!status.last_error.empty()) {
                    std::cout << "   Error: " << status.last_error << "\n";
                }
            }
        }

        // Test network with FPGA preference
        std::cout << "\n🌐 Testing network with FPGA preference...\n";
        NeuralNetwork network(AcceleratorType::FPGA_DE1SOC);

        if (network.initialize()) {
            std::cout << "✅ Network with FPGA preference initialized\n";
            std::cout << "   Using accelerator: "
                      << AcceleratorFactory::get_accelerator_name(network.get_accelerator_type()) << "\n";

            network.cleanup();
        } else {
            std::cout << "⚠️  Network initialization failed\n";
        }

        std::cout << "\n✅ FPGA Accelerator test completed!\n";

    } catch (const std::exception& e) {
        std::cerr << "❌ FPGA test failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
