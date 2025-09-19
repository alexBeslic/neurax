/**
 * @file TestSuite_AcceleratorFactory.cpp
 * @brief Unit tests for AcceleratorFactory
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "neurax/hal/AcceleratorFactory.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

using ::testing::AtLeast;
using namespace neurax::hal;

/* Test the `AcceleratorFactory::create` method with input type out of range
 *****************************************************************************/
TEST(AcceleratorFactoryTest, CreateInvalidAccelerator)
{
    auto accelerator = AcceleratorFactory::create(static_cast<AcceleratorType>(0xFF));
    ASSERT_NE(accelerator, nullptr);
    EXPECT_EQ(accelerator->get_type(), AcceleratorType::SOFTWARE_FALLBACK);
    EXPECT_TRUE(accelerator->initialize());
    EXPECT_TRUE(accelerator->is_available());
    accelerator->cleanup();
}

/* Test the `AcceleratorFactory::create` method with input type SOFTWARE_FALLBACK
 *****************************************************************************/
TEST(AcceleratorFactoryTest, CreateSoftwareAccelerator)
{
    auto accelerator = AcceleratorFactory::create(AcceleratorType::SOFTWARE_FALLBACK);
    ASSERT_NE(accelerator, nullptr);
    EXPECT_EQ(accelerator->get_type(), AcceleratorType::SOFTWARE_FALLBACK);
    EXPECT_TRUE(accelerator->initialize());
    EXPECT_TRUE(accelerator->is_available());
    accelerator->cleanup();
    EXPECT_TRUE(accelerator->initialize()); // SoftwareAccelerator always initializes successfully
}

/* Test the `AcceleratorFactory::create` method with input type CPU_OPTIMIZED
 *****************************************************************************/
TEST(AcceleratorFactoryTest, CreateCPUAccelerator)
{
    auto accelerator = AcceleratorFactory::create(AcceleratorType::CPU_OPTIMIZED);
    ASSERT_NE(accelerator, nullptr);
    EXPECT_EQ(accelerator->get_type(), AcceleratorType::CPU_OPTIMIZED);
    EXPECT_TRUE(accelerator->initialize());
    EXPECT_TRUE(accelerator->is_available());
    accelerator->cleanup();
    EXPECT_TRUE(accelerator->initialize()); // CPUAccelerator always initializes successfully
}

/* Test the `AcceleratorFactory::create` method with input type FPGA_DE1SOC
 *****************************************************************************/
TEST(AcceleratorFactoryTest, CreateFPGAAccelerator)
{
    auto accelerator = AcceleratorFactory::create(AcceleratorType::FPGA_DE1SOC);
    ASSERT_NE(accelerator, nullptr);
    EXPECT_EQ(accelerator->get_type(), AcceleratorType::FPGA_DE1SOC);
    EXPECT_TRUE(accelerator->initialize());
    EXPECT_TRUE(accelerator->is_available());
    accelerator->cleanup();
}

/* Test the `AcceleratorFactory::get_available_accelerators` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetAvailableAccelerators)
{
    auto available = AcceleratorFactory::get_available_accelerators();
    EXPECT_FALSE(available.empty());
    EXPECT_NE(std::find(available.begin(), available.end(), AcceleratorType::SOFTWARE_FALLBACK), available.end());
}

/* Test the `AcceleratorFactory::get_best_available` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetBestAvailable)
{
    auto best = AcceleratorFactory::get_best_available();
    auto available = AcceleratorFactory::get_available_accelerators();
    EXPECT_NE(std::find(available.begin(), available.end(), best), available.end());
}

/* Test the `AcceleratorFactory::is_available` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, IsAvailable)
{
    EXPECT_TRUE(AcceleratorFactory::is_available(AcceleratorType::SOFTWARE_FALLBACK));
    EXPECT_TRUE(AcceleratorFactory::is_available(AcceleratorType::CPU_OPTIMIZED));
}

/* Test the `AcceleratorFactory::get_best_available` method with only SOFTWARE_FALLBACK available
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetBestAvailable_OnlySoftware)
{
    auto best = AcceleratorFactory::get_best_available();
    if (AcceleratorFactory::is_available(AcceleratorType::SOFTWARE_FALLBACK) &&
        !AcceleratorFactory::is_available(AcceleratorType::CPU_OPTIMIZED) &&
        !AcceleratorFactory::is_available(AcceleratorType::GPU_OPENCL) &&
        !AcceleratorFactory::is_available(AcceleratorType::FPGA_DE1SOC))
    {
        EXPECT_EQ(best, AcceleratorType::SOFTWARE_FALLBACK);
    }
}

/* Test the `AcceleratorFactory::get_accelerator_name` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetAcceleratorName)
{
    EXPECT_EQ(AcceleratorFactory::get_accelerator_name(AcceleratorType::SOFTWARE_FALLBACK), "Software Fallback");
    EXPECT_EQ(AcceleratorFactory::get_accelerator_name(AcceleratorType::CPU_OPTIMIZED), "CPU Optimized");
    EXPECT_EQ(AcceleratorFactory::get_accelerator_name(AcceleratorType::GPU_OPENCL), "GPU OpenCL");
    EXPECT_EQ(AcceleratorFactory::get_accelerator_name(AcceleratorType::FPGA_DE1SOC), "FPGA DE1-SoC");
    EXPECT_EQ(AcceleratorFactory::get_accelerator_name(static_cast<AcceleratorType>(0xFF)), "Unknown");
}

/* Test the `AcceleratorFactory::get_accelerator_info` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetAcceleratorInfo)
{
    EXPECT_EQ(AcceleratorFactory::get_accelerator_info(AcceleratorType::SOFTWARE_FALLBACK), "Pure C++ software implementation (always available)");
    EXPECT_EQ(AcceleratorFactory::get_accelerator_info(AcceleratorType::CPU_OPTIMIZED), "CPU accelerator with SIMD optimizations and threading");
    EXPECT_EQ(AcceleratorFactory::get_accelerator_info(AcceleratorType::GPU_OPENCL), "GPU accelerator using OpenCL for parallel computation");
    EXPECT_EQ(AcceleratorFactory::get_accelerator_info(AcceleratorType::FPGA_DE1SOC), "DE1-SoC FPGA accelerator with custom neural network hardware");
    EXPECT_EQ(AcceleratorFactory::get_accelerator_info(static_cast<AcceleratorType>(0xFF)), "Unknown accelerator type");
}
