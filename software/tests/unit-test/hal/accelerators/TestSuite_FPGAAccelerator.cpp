/**
 * @file TestSuite_FPGAAccelerator.cpp
 * @brief Unit tests for FPGAAccelerator
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "FPGAAccelerator.hpp"
#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

using ::testing::AtLeast;
using namespace neurax::tensor;

/* Test the `FPGAAccelerator' not yet implemented behavior
 *****************************************************************************/
TEST(FPGAAcceleratorTest, NotYetImplemented)
{
    EXPECT_THROW(neurax::hal::FPGAAccelerator fpga_accel, std::runtime_error);
}