/**
 * @file TestSuite_Shape.cpp
 * @brief Unit tests for Shape
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#include <gtest/gtest.h>

#include "neurax/tensor/Shape.hpp"

using namespace neurax::tensor;

/* Test the `Shape` constructor and `getDimensions` method
 *****************************************************************************/
TEST(ShapeTest, ConstructorAndGetDimensions)
{
    Shape shape({2, 3, 4});
    EXPECT_EQ(shape.getDimensions(), std::vector<size_t>({2, 3, 4}));
}

/* Test the `Shape` constructor with vector and `size` method
 *****************************************************************************/
TEST(ShapeTest, ConstructorWithVectorAndSize)
{
    Shape shape(std::vector<size_t>({5, 6, 7, 8}));
    EXPECT_EQ(shape.size(), 4);
}

/* Test the `Shape::numel` method
 *****************************************************************************/
TEST(ShapeTest, Numel)
{
    Shape shape({2, 3, 4});
    EXPECT_EQ(shape.numel(), 24); // 2 * 3 * 4
}

/* Test the `Shape::str` method
 *****************************************************************************/
TEST(ShapeTest, Str)
{
    Shape shape({1, 2, 3});
    EXPECT_EQ(shape.str(), "[1, 2, 3]");
}

/* Test the `Shape` equality and inequality operators
 *****************************************************************************/
TEST(ShapeTest, EqualityAndInequality)
{
    Shape shape1({2, 3, 4});
    Shape shape2({2, 3, 4});
    Shape shape3({3, 4, 5});
    EXPECT_TRUE(shape1 == shape2);
    EXPECT_FALSE(shape1 != shape2);
    EXPECT_TRUE(shape1 != shape3);
    EXPECT_FALSE(shape1 == shape3);
}

/* Test the `Shape::empty` method
 *****************************************************************************/
TEST(ShapeTest, Empty)
{
    Shape shape1;
    Shape shape2({1, 2, 3});
    EXPECT_TRUE(shape1.empty());
    EXPECT_FALSE(shape2.empty());
}

/* Test the `Shape` indexing operator
 *****************************************************************************/
TEST(ShapeTest, IndexingOperator)
{
    Shape shape({10, 20, 30});
    EXPECT_EQ(shape[0], 10);
    EXPECT_EQ(shape[1], 20);
    EXPECT_EQ(shape[2], 30);
    shape[1] = 25;
    EXPECT_EQ(shape[1], 25);
}

/* Test the `Shape` with single dimension
 *****************************************************************************/
TEST(ShapeTest, SingleDimension)
{
    Shape shape({42});
    EXPECT_EQ(shape.size(), 1);
    EXPECT_EQ(shape.numel(), 42);
    EXPECT_EQ(shape.str(), "[42]");
    EXPECT_FALSE(shape.empty());
}
