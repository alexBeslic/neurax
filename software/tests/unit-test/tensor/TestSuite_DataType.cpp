/**
 * @file TestSuite_DataType.cpp
 * @brief Unit tests for DataType
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "neurax/tensor/DataType.hpp"

using namespace neurax::tensor;

/* Test the `sizeof_dtype` function
 *****************************************************************************/
TEST(DataTypeTest, SizeofDtype)
{
    EXPECT_EQ(sizeof_dtype(DataType::INT8), 1);
    EXPECT_EQ(sizeof_dtype(DataType::INT16), 2);
    EXPECT_EQ(sizeof_dtype(DataType::FLOAT32), 4);
}

/* Test the `dtype_name` function
 *****************************************************************************/
TEST(DataTypeTest, DtypeName)
{
    EXPECT_STREQ(dtype_name(DataType::INT8), "int8");
    EXPECT_STREQ(dtype_name(DataType::INT16), "int16");
    EXPECT_STREQ(dtype_name(DataType::FLOAT32), "float32");
}

/* Test the `is_floating_point` function
 *****************************************************************************/
TEST(DataTypeTest, IsFloatingPoint)
{
    EXPECT_FALSE(is_floating_point(DataType::INT8));
    EXPECT_FALSE(is_floating_point(DataType::INT16));
    EXPECT_TRUE(is_floating_point(DataType::FLOAT32));
}

/* Test the `is_integer` function
 *****************************************************************************/
TEST(DataTypeTest, IsInteger)
{
    EXPECT_TRUE(is_integer(DataType::INT8));
    EXPECT_TRUE(is_integer(DataType::INT16));
    EXPECT_FALSE(is_integer(DataType::FLOAT32));
}
