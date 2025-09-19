/**
 * @file TestSuite_SoftwareAccelerator.cpp
 * @brief Unit tests for SoftwareAccelerator
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "SoftwareAccelerator.hpp"
#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

using ::testing::AtLeast;
using namespace neurax::tensor;
using namespace neurax::hal;

/* Test the `SoftwareAccelerator` Initialize And Cleanup method
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, InitializeAndCleanup)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    EXPECT_TRUE(sw_accel.is_available());
    sw_accel.cleanup();
    EXPECT_TRUE(sw_accel.initialize()); // SoftwareAccelerator always initializes successfully
}

/* Test the `SoftwareAccelerator::GetStatus` method
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, GetStatus)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    auto status = sw_accel.get_status();
    EXPECT_EQ(status.active_type, AcceleratorType::SOFTWARE_FALLBACK);
    EXPECT_EQ(status.requested_type, AcceleratorType::SOFTWARE_FALLBACK);
    EXPECT_TRUE(status.using_fallback);
    EXPECT_TRUE(status.is_available);
    EXPECT_EQ(status.last_error, "");
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::SetAndGetPrecision` method
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, SetAndGetPrecision)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    EXPECT_FALSE(sw_accel.is_16bit_precision());
    EXPECT_TRUE(sw_accel.set_precision(true));
    EXPECT_TRUE(sw_accel.is_16bit_precision());
    EXPECT_TRUE(sw_accel.set_precision(false));
    EXPECT_FALSE(sw_accel.is_16bit_precision());
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::SetAndGetDebugMode` method
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, SetAndGetDebugMode)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    EXPECT_FALSE(sw_accel.is_debug_mode());
    sw_accel.set_debug_mode(true);
    EXPECT_TRUE(sw_accel.is_debug_mode());
    sw_accel.set_debug_mode(false);
    EXPECT_FALSE(sw_accel.is_debug_mode());
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Convolution` method with valid inputs
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, ConvolutionValidInputs)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({1, 5, 5, 3}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({3, 3, 3, 2}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({2}, DataType::FLOAT32);
    ConvolutionConfig config = ConvolutionConfig(3, 1, 0, 3, 2);
    Tensor output = sw_accel.convolution(input, weights, bias, config);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({1, 3, 3, 2}));
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Convolution` method with invalid input dimensions
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, ConvolutionInvalidInputDimensions)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({1, 5, 5}, DataType::FLOAT32); // Invalid shape
    Tensor weights = Tensor::ones({3, 3, 3, 2}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({2}, DataType::FLOAT32);
    ConvolutionConfig config = ConvolutionConfig(3, 1, 0, 3, 2);
    EXPECT_THROW(sw_accel.convolution(input, weights, bias, config), std::runtime_error);
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Convolution` method with invalid weight dimensions
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, ConvolutionInvalidWeightDimensions)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({1, 5, 5, 3}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({3, 3, 2}, DataType::FLOAT32); // Invalid shape
    Tensor bias = Tensor::zeros({2}, DataType::FLOAT32);
    ConvolutionConfig config = ConvolutionConfig(3, 1, 0, 3, 2);
    EXPECT_THROW(sw_accel.convolution(input, weights, bias, config), std::runtime_error);
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Convolution` method with mismatched channels
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, ConvolutionMismatchedChannels)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({1, 5, 5, 3}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({3, 3, 4, 2}, DataType::FLOAT32); // Mismatched input channels
    Tensor bias = Tensor::zeros({2}, DataType::FLOAT32);
    ConvolutionConfig config = ConvolutionConfig(3, 1, 0, 3, 2);
    EXPECT_THROW(sw_accel.convolution(input, weights, bias, config), std::runtime_error);
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Convolution` method with nullptr inputs
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, ConvolutionNullptrInputs)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input;             // Empty tensor
    Tensor weights;           // Empty tensor
    Tensor bias;              // Empty tensor
    ConvolutionConfig config; // Default config
    EXPECT_THROW(sw_accel.convolution(input, weights, bias, config), std::runtime_error);

    input = Tensor::ones({1, 5, 5, 3}, DataType::FLOAT32);
    EXPECT_THROW(sw_accel.convolution(input, weights, bias, config), std::runtime_error);

    weights = Tensor::ones({3, 3, 3, 2}, DataType::FLOAT32);
    EXPECT_THROW(sw_accel.convolution(input, weights, bias, config), std::runtime_error);

    bias = Tensor::zeros({2}, DataType::FLOAT32);
    EXPECT_THROW(sw_accel.convolution(input, weights, bias, config), std::runtime_error);
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Activation` method with valid inputs
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, ActivationValidInputs)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({2, 3}, DataType::FLOAT32);

    Tensor output = sw_accel.activation(input, ActivationType::RELU);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({2, 3}));
    const float *output_data = output.data_ptr<float>();
    for (size_t i = 0; i < output.numel(); ++i)
    {
        EXPECT_EQ(output_data[i], 1.0f);
    }

    output = sw_accel.activation(input, ActivationType::SIGMOID);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({2, 3}));
    output_data = output.data_ptr<float>();
    for (size_t i = 0; i < output.numel(); ++i)
    {
        EXPECT_NEAR(output_data[i], 0.7310586f, 1e-6);
    }

    output = sw_accel.activation(input, ActivationType::TANH);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({2, 3}));
    output_data = output.data_ptr<float>();
    for (size_t i = 0; i < output.numel(); ++i)
    {
        EXPECT_NEAR(output_data[i], 0.7615942f, 1e-6);
    }

    output = sw_accel.activation(input, ActivationType::LINEAR);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({2, 3}));
    output_data = output.data_ptr<float>();
    for (size_t i = 0; i < output.numel(); ++i)
    {
        EXPECT_EQ(output_data[i], 1.0f);
    }

    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Activation` method with invalid inputs
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, ActivationInvalidInputs)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({2, 3}, DataType::FLOAT32);
    EXPECT_THROW(sw_accel.activation(input, static_cast<ActivationType>(0xFF)), std::runtime_error);
}

/* Test the `SoftwareAccelerator::pooling` method with valid inputs
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, PoolingValidInputs)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({1, 4, 4, 1}, DataType::FLOAT32);

    PoolingConfig config = PoolingConfig(3, 3, PoolingType::MAX);
    Tensor output = sw_accel.pooling(input, config);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({1, 1, 1, 1}));
    const float *output_data = output.data_ptr<float>();
    for (size_t i = 0; i < output.numel(); ++i)
    {
        EXPECT_EQ(output_data[i], 1.0f);
    }

    config = PoolingConfig(2, 2, PoolingType::AVERAGE);
    output = sw_accel.pooling(input, config);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({1, 2, 2, 1}));
    output_data = output.data_ptr<float>();
    for (size_t i = 0; i < output.numel(); ++i)
    {
        EXPECT_EQ(output_data[i], 1.0f);
    }

    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Dense` method with valid inputs
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, DenseValidInputs)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({2, 4}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({4, 3}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({3}, DataType::FLOAT32);
    Tensor output = sw_accel.dense(input, weights, bias);
    EXPECT_EQ(output.shape().dims(), std::vector<size_t>({2, 3}));
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Dense` method with invalid input dimensions
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, DenseInvalidInputDimensions)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({2, 4, 5}, DataType::FLOAT32); // Invalid shape
    Tensor weights = Tensor::ones({4, 3}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({3}, DataType::FLOAT32);
    EXPECT_THROW(sw_accel.dense(input, weights, bias), std::runtime_error);
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Dense` method with mismatched dimensions
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, DenseMismatchedDimensions)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({2, 4}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({5, 3}, DataType::FLOAT32); // Mismatched input features
    Tensor bias = Tensor::zeros({3}, DataType::FLOAT32);
    EXPECT_THROW(sw_accel.dense(input, weights, bias), std::runtime_error);
    sw_accel.cleanup();
}

/* Test the `SoftwareAccelerator::Dense` method with invalid bias dimensions
 *****************************************************************************/
TEST(SoftwareAcceleratorTest, DenseInvalidBiasDimensions)
{
    SoftwareAccelerator sw_accel;
    EXPECT_TRUE(sw_accel.initialize());
    Tensor input = Tensor::ones({2, 4}, DataType::FLOAT32);
    Tensor weights = Tensor::ones({4, 3}, DataType::FLOAT32);
    Tensor bias = Tensor::zeros({4}, DataType::FLOAT32); // Invalid bias shape
    EXPECT_THROW(sw_accel.dense(input, weights, bias), std::runtime_error);
    sw_accel.cleanup();
}
