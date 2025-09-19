/**
 * @file TestSuite_CPUAccelerator.cpp
 * @brief Unit tests for CPUAccelerator
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "CPUAccelerator.hpp"
#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

using ::testing::AtLeast;
using namespace neurax::tensor;
using namespace neurax::hal;

/* Test the `CPUAccelerator' Initialize And Cleanup
 *****************************************************************************/
TEST(CPUAcceleratorTest, InitializeAndCleanup)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    EXPECT_TRUE(cpu_accel.is_available());
    cpu_accel.cleanup();
    EXPECT_TRUE(cpu_accel.initialize()); // CPUAccelerator always initializes successfully
}

/* Test the `CPUAccelerator::GetStatus` method
 *****************************************************************************/
TEST(CPUAcceleratorTest, GetStatus)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    auto status = cpu_accel.get_status();
    EXPECT_EQ(status.active_type, AcceleratorType::CPU_OPTIMIZED);
    EXPECT_EQ(status.requested_type, AcceleratorType::CPU_OPTIMIZED);
    EXPECT_FALSE(status.using_fallback);
    EXPECT_TRUE(status.is_available);
    EXPECT_EQ(status.last_error, "");
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::SetAndGetPrecision` method
 *****************************************************************************/
TEST(CPUAcceleratorTest, SetAndGetPrecision)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    EXPECT_FALSE(cpu_accel.is_16bit_precision());
    EXPECT_TRUE(cpu_accel.set_precision(true));
    EXPECT_TRUE(cpu_accel.is_16bit_precision());
    EXPECT_TRUE(cpu_accel.set_precision(false));
    EXPECT_FALSE(cpu_accel.is_16bit_precision());
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::SetAndGetDebugMode` method
 *****************************************************************************/
TEST(CPUAcceleratorTest, SetAndGetDebugMode)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    EXPECT_FALSE(cpu_accel.is_debug_mode());
    cpu_accel.set_debug_mode(true);
    EXPECT_TRUE(cpu_accel.is_debug_mode());
    cpu_accel.set_debug_mode(false);
    EXPECT_FALSE(cpu_accel.is_debug_mode());
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::Convolution` method with valid inputs
 *****************************************************************************/
TEST(CPUAcceleratorTest, ConvolutionValidInputs)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    Tensor input = Tensor::ones({1, 5, 5, 3}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({3, 3, 3, 2}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({2}, DataType::FLOAT32);
    ConvolutionConfig config = ConvolutionConfig(3, 1, 0, 3, 2);
    Tensor output = cpu_accel.convolution(input, weights, bias, config);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({1, 3, 3, 2}));
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::Convolution` method with invalid input dimensions
 *****************************************************************************/
TEST(CPUAcceleratorTest, ConvolutionInvalidInputDimensions)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    Tensor input = Tensor::ones({1, 5, 5}, DataType::FLOAT32); // Invalid shape
    Tensor weights = Tensor::ones({3, 3, 3, 2}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({2}, DataType::FLOAT32);
    ConvolutionConfig config = ConvolutionConfig(3, 1, 0, 3, 2);
    EXPECT_THROW(cpu_accel.convolution(input, weights, bias, config), std::runtime_error);
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::Convolution` method with invalid weight dimensions
 *****************************************************************************/
TEST(CPUAcceleratorTest, ConvolutionInvalidWeightDimensions)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    Tensor input = Tensor::ones({1, 5, 5, 3}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({3, 3, 2}, DataType::FLOAT32); // Invalid shape
    Tensor bias = Tensor::zeros({2}, DataType::FLOAT32);
    ConvolutionConfig config = ConvolutionConfig(3, 1, 0, 3, 2);
    EXPECT_THROW(cpu_accel.convolution(input, weights, bias, config), std::runtime_error);
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::Convolution` method with mismatched channels
 *****************************************************************************/
TEST(CPUAcceleratorTest, ConvolutionMismatchedChannels)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    Tensor input = Tensor::ones({1, 5, 5, 3}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({3, 3, 4, 2}, DataType::FLOAT32); // Mismatched input channels
    Tensor bias = Tensor::zeros({2}, DataType::FLOAT32);
    ConvolutionConfig config = ConvolutionConfig(3, 1, 0, 3, 2);
    EXPECT_THROW(cpu_accel.convolution(input, weights, bias, config), std::runtime_error);
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::Activation` method with valid inputs
 *****************************************************************************/
TEST(CPUAcceleratorTest, ActivationValidInputs)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    Tensor input = Tensor::ones({2, 3}, DataType::FLOAT32);
    Tensor output = cpu_accel.activation(input, ActivationType::RELU);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({2, 3}));
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::Dense` method with valid inputs
 *****************************************************************************/
TEST(CPUAcceleratorTest, DenseValidInputs)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    Tensor input = Tensor::ones({2, 4}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({4, 3}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({3}, DataType::FLOAT32);
    Tensor output = cpu_accel.dense(input, weights, bias);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({2, 3}));
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::Dense` method with invalid input dimensions
 *****************************************************************************/
TEST(CPUAcceleratorTest, DenseInvalidInputDimensions)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    Tensor input = Tensor::ones({2, 4, 5}, DataType::FLOAT32); // Invalid shape
    Tensor weights = Tensor::ones({4, 3}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({3}, DataType::FLOAT32);
    EXPECT_THROW(cpu_accel.dense(input, weights, bias), std::runtime_error);
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::Dense` method with mismatched dimensions
 *****************************************************************************/
TEST(CPUAcceleratorTest, DenseMismatchedDimensions)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    Tensor input = Tensor::ones({2, 4}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({5, 3}, DataType::FLOAT32); // Mismatched input features
    Tensor bias = Tensor::zeros({3}, DataType::FLOAT32);
    EXPECT_THROW(cpu_accel.dense(input, weights, bias), std::runtime_error);
    cpu_accel.cleanup();
}

/* Test the `CPUAccelerator::Dense` method with invalid bias dimensions
 *****************************************************************************/
TEST(CPUAcceleratorTest, DenseInvalidBiasDimensions)
{
    CPUAccelerator cpu_accel;
    EXPECT_TRUE(cpu_accel.initialize());
    Tensor input = Tensor::ones({2, 4}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({4, 3}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({4}, DataType::FLOAT32); // Invalid bias shape
    EXPECT_THROW(cpu_accel.dense(input, weights, bias), std::runtime_error);
    cpu_accel.cleanup();
}
