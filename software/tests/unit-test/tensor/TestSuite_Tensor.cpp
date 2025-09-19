/**
 * @file TestSuite_Tensor.cpp
 * @brief Unit tests for Tensor
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#include <gtest/gtest.h>

#include "neurax/tensor/Tensor.hpp"

using namespace neurax::tensor;

/* Test the `Tensor` default constructor
 *****************************************************************************/
TEST(TensorTest, DefaultConstructor)
{
    Tensor tensor;
    EXPECT_EQ(tensor.shape().size(), 0);
    EXPECT_NE(tensor.dtype(), DataType::FLOAT32);
    EXPECT_EQ(tensor.numel(), 0);
    EXPECT_EQ(tensor.nbytes(), 0);
    EXPECT_EQ(tensor.data(), nullptr);
    EXPECT_TRUE(tensor.empty());
}

/* Test the `Tensor` constructor with shape and dtype
 *****************************************************************************/
TEST(TensorTest, ConstructorWithShapeAndDtype)
{
    Shape shape({2, 3, 4});
    Tensor tensor(shape, DataType::INT16);
    EXPECT_EQ(tensor.shape(), shape);
    EXPECT_EQ(tensor.dtype(), DataType::INT16);
    EXPECT_EQ(tensor.numel(), 24); // 2 * 3 * 4
    EXPECT_EQ(tensor.nbytes(), 24 * sizeof(int16_t));
    EXPECT_NE(tensor.data(), nullptr);
    EXPECT_FALSE(tensor.empty());
}

/* Test the `Tensor` constructor with shape, data, and dtype
 *****************************************************************************/
TEST(TensorTest, ConstructorWithShapeDataAndDtype)
{
    Shape shape({2, 2});
    int8_t data[4] = {1, 2, 3, 4};
    Tensor tensor(shape, data, DataType::INT8);
    EXPECT_EQ(tensor.shape(), shape);
    EXPECT_EQ(tensor.dtype(), DataType::INT8);
    EXPECT_EQ(tensor.numel(), 4);
    EXPECT_EQ(tensor.nbytes(), 4 * sizeof(int8_t));
    EXPECT_NE(tensor.data(), nullptr);
    EXPECT_FALSE(tensor.empty());
    const int8_t *tensor_data = tensor.data_ptr<int8_t>();
    for (size_t i = 0; i < 4; i++)
    {
        EXPECT_EQ(tensor_data[i], data[i]);
    }
}

/* Test the `Tensor` copy constructor
 *****************************************************************************/
TEST(TensorTest, CopyConstructor)
{
    Shape shape({2, 2});
    Tensor tensor1(shape, DataType::INT16);
    tensor1.zero_();
    Tensor tensor2(tensor1);
    EXPECT_EQ(tensor2.shape(), tensor1.shape());
    EXPECT_EQ(tensor2.dtype(), tensor1.dtype());
    EXPECT_EQ(tensor2.numel(), tensor1.numel());
    EXPECT_EQ(tensor2.nbytes(), tensor1.nbytes());
    EXPECT_NE(tensor2.data(), nullptr);
    EXPECT_FALSE(tensor2.empty());
    const int16_t *data1 = tensor1.data_ptr<int16_t>();
    const int16_t *data2 = tensor2.data_ptr<int16_t>();
    for (size_t i = 0; i < tensor1.numel(); ++i)
    {
        EXPECT_EQ(data1[i], data2[i]);
    }
}

/* Test the `Tensor` move constructor
 *****************************************************************************/
TEST(TensorTest, MoveConstructor)
{
    Shape shape({2, 2});
    Tensor tensor1(shape, DataType::FLOAT32);
    tensor1.zero_();
    float *data1 = tensor1.data_ptr<float>();
    Tensor tensor2(std::move(tensor1));
    EXPECT_EQ(tensor2.shape(), shape);
    EXPECT_EQ(tensor2.dtype(), DataType::FLOAT32);
    EXPECT_EQ(tensor2.numel(), 4);
    EXPECT_EQ(tensor2.nbytes(), 4 * sizeof(float));
    EXPECT_NE(tensor2.data(), nullptr);
    EXPECT_FALSE(tensor2.empty());
    float *data2 = tensor2.data_ptr<float>();
    for (size_t i = 0; i < tensor2.numel(); ++i)
    {
        EXPECT_EQ(data2[i], 0.0f);
    }
    EXPECT_EQ(tensor1.data(), nullptr); // Moved-from tensor should be empty
    EXPECT_TRUE(tensor1.empty());
}

/* Test the `Tensor` copy assignment operator
 *****************************************************************************/
TEST(TensorTest, CopyAssignment)
{
    Shape shape({2, 2});
    Tensor tensor1(shape, DataType::INT8);
    tensor1.zero_();
    Tensor tensor2;
    tensor2 = tensor1;
    EXPECT_EQ(tensor2.shape(), tensor1.shape());
    EXPECT_EQ(tensor2.dtype(), tensor1.dtype());
    EXPECT_EQ(tensor2.numel(), tensor1.numel());
    EXPECT_EQ(tensor2.nbytes(), tensor1.nbytes());
    EXPECT_NE(tensor2.data(), nullptr);
    EXPECT_FALSE(tensor2.empty());
    const int8_t *data1 = tensor1.data_ptr<int8_t>();
    const int8_t *data2 = tensor2.data_ptr<int8_t>();
    for (size_t i = 0; i < tensor1.numel(); ++i)
    {
        EXPECT_EQ(data1[i], data2[i]);
    }
}

/* Test the `Tensor` move assignment operator
 *****************************************************************************/
TEST(TensorTest, MoveAssignment)
{
    Shape shape({2, 2});
    Tensor tensor1(shape, DataType::INT16);
    tensor1.zero_();
    Tensor tensor2;
    tensor2 = std::move(tensor1);
    EXPECT_EQ(tensor2.shape(), shape);
    EXPECT_EQ(tensor2.dtype(), DataType::INT16);
    EXPECT_EQ(tensor2.numel(), 4);
    EXPECT_EQ(tensor2.nbytes(), 4 * sizeof(int16_t));
    EXPECT_NE(tensor2.data(), nullptr);
    EXPECT_FALSE(tensor2.empty());
    const int16_t *data2 = tensor2.data_ptr<int16_t>();
    for (size_t i = 0; i < tensor2.numel(); ++i)
    {
        EXPECT_EQ(data2[i], 0);
    }
    EXPECT_EQ(tensor1.data(), nullptr); // Moved-from tensor should be empty
    EXPECT_TRUE(tensor1.empty());
}

/* Test the `Tensor::zero_` method
 *****************************************************************************/
TEST(TensorTest, ZeroMethod)
{
    Shape shape({3, 3});
    Tensor tensor(shape, DataType::FLOAT32);
    float *data = tensor.data_ptr<float>();
    for (size_t i = 0; i < tensor.numel(); ++i)
    {
        data[i] = static_cast<float>(i + 1); // Fill with non-zero values
    }
    tensor.zero_();
    for (size_t i = 0; i < tensor.numel(); ++i)
    {
        EXPECT_EQ(data[i], 0.0f);
    }
}

/* Test the `Tensor::copy_from` and `Tensor::copy_to` methods
 *****************************************************************************/
TEST(TensorTest, CopyFromAndTo)
{
    Shape shape({2, 2});
    Tensor tensor(shape, DataType::INT8);
    int8_t src_data[4] = {10, 20, 30, 40};
    tensor.copy_from(src_data, sizeof(src_data));
    int8_t dst_data[4] = {0};
    tensor.copy_to(dst_data, sizeof(dst_data));
    for (size_t i = 0; i < 4; ++i)
    {
        EXPECT_EQ(dst_data[i], src_data[i]);
    }
}

/* Test the `Tensor` factory methods
 *****************************************************************************/
TEST(TensorTest, FactoryMethods)
{
    Shape shape({2, 2});
    Tensor zeros_tensor = Tensor::zeros(shape, DataType::INT16);
    EXPECT_EQ(zeros_tensor.shape(), shape);
    EXPECT_EQ(zeros_tensor.dtype(), DataType::INT16);
    const int16_t *zeros_data = zeros_tensor.data_ptr<int16_t>();
    for (size_t i = 0; i < zeros_tensor.numel(); ++i)
    {
        EXPECT_EQ(zeros_data[i], 0);
    }

    Tensor ones_tensor = Tensor::ones(shape, DataType::INT8);
    EXPECT_EQ(ones_tensor.shape(), shape);
    EXPECT_EQ(ones_tensor.dtype(), DataType::INT8);
    const int8_t *ones_data = ones_tensor.data_ptr<int8_t>();
    for (size_t i = 0; i < ones_tensor.numel(); ++i)
    {
        EXPECT_EQ(ones_data[i], 1);
    }

    int8_t blob_data[4] = {5, 6, 7, 8};
    Tensor blob_tensor = Tensor::from_blob(blob_data, shape, DataType::INT8);
    EXPECT_EQ(blob_tensor.shape(), shape);
    EXPECT_EQ(blob_tensor.dtype(), DataType::INT8);
    const int8_t *blob_tensor_data = blob_tensor.data_ptr<int8_t>();
    for (size_t i = 0; i < blob_tensor.numel(); ++i)
    {
        EXPECT_EQ(blob_tensor_data[i], blob_data[i]);
    }

    Tensor empty_tensor = Tensor::empty(shape, DataType::FLOAT32);
    EXPECT_EQ(empty_tensor.shape(), shape);
    EXPECT_EQ(empty_tensor.dtype(), DataType::FLOAT32);
    EXPECT_NE(empty_tensor.data(), nullptr);
    EXPECT_FALSE(empty_tensor.empty());
}
