/**
 * @file TestSuite_NeuralNetworkBuilder.cpp
 * @brief Unit tests for NeuralNetworkBuilder
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/hal/AcceleratorFactory.hpp"
#include "neurax/core/LayerBuilder.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

using ::testing::AtLeast;
using namespace neurax::core;
using namespace neurax::tensor;
using namespace neurax::hal;

/* Test the `NeuralNetworkBuilder::addLayer` method
 *****************************************************************************/
TEST(NeuralNetworkBuilderTest, AddLayer)
{
    NeuralNetworkBuilder builder;
    auto layer = LayerBuilder().conv2d().build();
    builder.addLayer(layer);
    auto network = builder.build();
    EXPECT_EQ(network->getLayerCount(), 1);
}

/* Test the `NeuralNetworkBuilder::useAccelerator` method
 *****************************************************************************/
TEST(NeuralNetworkBuilderTest, UseAccelerator)
{
    NeuralNetworkBuilder builder;
    builder.useAccelerator(AcceleratorType::SOFTWARE_FALLBACK);
    auto layer = LayerBuilder().conv2d().build();
    builder.addLayer(layer);
    auto network = builder.build();
    EXPECT_NE(network->getAccelerator(), nullptr);
    EXPECT_EQ(network->getAccelerator()->get_status().active_type, AcceleratorType::SOFTWARE_FALLBACK);
}

/* Test the `NeuralNetworkBuilder::build` method without layers
 *****************************************************************************/
TEST(NeuralNetworkBuilderTest, BuildWithoutLayers)
{
    NeuralNetworkBuilder builder;
    builder.useAccelerator(AcceleratorType::CPU_OPTIMIZED);
    auto network = builder.build();
    EXPECT_EQ(network->getLayerCount(), 0);
    EXPECT_NE(network->getAccelerator(), nullptr);
    EXPECT_EQ(network->getAccelerator()->get_status().active_type, AcceleratorType::CPU_OPTIMIZED);
}

/* Test the `NeuralNetworkBuilder::build` method without specifying an accelerator
 *****************************************************************************/
TEST(NeuralNetworkBuilderTest, BuildWithoutAccelerator)
{
    NeuralNetworkBuilder builder;
    auto layer = LayerBuilder().conv2d().build();
    builder.addLayer(layer);
    auto network = builder.build();
    EXPECT_EQ(network->getLayerCount(), 1);
    EXPECT_NE(network->getAccelerator(), nullptr);
    EXPECT_EQ(network->getAccelerator()->get_status().active_type, AcceleratorType::SOFTWARE_FALLBACK);
}

/* Test the `NeuralNetworkBuilder::build` method with invalid accelerator type
 *****************************************************************************/
TEST(NeuralNetworkBuilderTest, BuildWithInvalidAccelerator)
{
    NeuralNetworkBuilder builder;
    builder.useAccelerator(static_cast<AcceleratorType>(0xFF));
    auto layer = LayerBuilder().conv2d().build();
    builder.addLayer(layer);
    auto network = builder.build();
    EXPECT_EQ(network->getLayerCount(), 1);
    EXPECT_NE(network->getAccelerator(), nullptr);
    EXPECT_EQ(network->getAccelerator()->get_status().active_type, AcceleratorType::SOFTWARE_FALLBACK);
}

/* Test the `NeuralNetworkBuilder::build` method with multiple layers and accelerators
 *****************************************************************************/
TEST(NeuralNetworkBuilderTest, BuildWithMultipleLayersAndAccelerators)
{
    size_t layer_count = 100;
    NeuralNetworkBuilder builder;
    builder.useAccelerator(AcceleratorType::CPU_OPTIMIZED);
    for (size_t i = 0; i < layer_count; i++)
    {
        auto layer = LayerBuilder().conv2d().build();
        builder.addLayer(layer);
    }
    auto network = builder.build();
    EXPECT_EQ(network->getLayerCount(), layer_count);
    EXPECT_NE(network->getAccelerator(), nullptr);
    EXPECT_EQ(network->getAccelerator()->get_status().active_type, AcceleratorType::CPU_OPTIMIZED);
}

/* Test the `NeuralNetworkBuilder::removeLayer` method with nullptr
 *****************************************************************************/
TEST(NeuralNetworkBuilderTest, RemoveLayerWithNullptr)
{
    NeuralNetworkBuilder builder;
    auto layer1 = LayerBuilder().conv2d().build();
    auto layer2 = LayerBuilder().conv2d().build();
    builder.addLayer(layer1);
    builder.addLayer(layer2);
    auto network = builder.build();
    EXPECT_EQ(network->getLayerCount(), 2);
    network->removeLayer(1);
    EXPECT_EQ(network->getLayerCount(), 1);
    network->removeLayer(0);
    EXPECT_EQ(network->getLayerCount(), 0);
    network->removeLayer(0);
    EXPECT_EQ(network->getLayerCount(), 0);
}

/* Test the `NeuralNetworkBuilder::infer` method with one layer and accelerator
 *****************************************************************************/
TEST(NeuralNetworkBuilderTest, InferWithOneLayerAndAccelerator)
{
    NeuralNetworkBuilder builder;
    builder.useAccelerator(AcceleratorType::CPU_OPTIMIZED);
    // TODO: Update test once LayerBuilder added the ability to configure convolution parameters
    auto layer = LayerBuilder().conv2d().addWeights(Tensor::ones({3, 3, 3, 3}), Tensor::zeros({1, 3, 3, 1})).build();
    builder.addLayer(layer);
    auto network = builder.build();
    Tensor input = Tensor::ones({1, 3, 32, 32});
    Tensor output;
    network->infer(input, output);
    EXPECT_EQ(output.getShape(), std::vector<size_t>({1, 16, 30, 30})); // Assuming output shape is same as input for this test
}
