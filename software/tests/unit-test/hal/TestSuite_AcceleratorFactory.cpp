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

/* Test the `AcceleratorFactory::create` method with input type out of range
 *****************************************************************************/
TEST(AcceleratorFactoryTest, CreateInvalidAccelerator)
{
    auto accelerator = neurax::hal::AcceleratorFactory::create((neurax::hal::AcceleratorType)0xFF);
    ASSERT_EQ(accelerator, nullptr);
}

/* Test the `AcceleratorFactory::create` method with input type SOFTWARE_FALLBACK
 *****************************************************************************/
TEST(AcceleratorFactoryTest, CreateSoftwareAccelerator)
{
    auto accelerator = neurax::hal::AcceleratorFactory::create(neurax::hal::AcceleratorType::SOFTWARE_FALLBACK);
    ASSERT_NE(accelerator, nullptr);
    EXPECT_EQ(accelerator->get_type(), neurax::hal::AcceleratorType::SOFTWARE_FALLBACK);
    EXPECT_TRUE(accelerator->initialize());
    EXPECT_TRUE(accelerator->is_available());
    accelerator->cleanup();
}

/* Test the `AcceleratorFactory::create` method with input type CPU_OPTIMIZED
 *****************************************************************************/
TEST(AcceleratorFactoryTest, CreateCPUAccelerator)
{
    auto accelerator = neurax::hal::AcceleratorFactory::create(neurax::hal::AcceleratorType::CPU_OPTIMIZED);
    ASSERT_NE(accelerator, nullptr);
    EXPECT_EQ(accelerator->get_type(), neurax::hal::AcceleratorType::CPU_OPTIMIZED);
    EXPECT_TRUE(accelerator->initialize());
    EXPECT_TRUE(accelerator->is_available());
    accelerator->cleanup();
}

/* Test the `AcceleratorFactory::create` method with input type FPGA_DE1SOC
 *****************************************************************************/
TEST(AcceleratorFactoryTest, CreateFPGAAccelerator)
{
    auto accelerator = neurax::hal::AcceleratorFactory::create(neurax::hal::AcceleratorType::FPGA_DE1SOC);
    if (accelerator)
    {
        EXPECT_EQ(accelerator->get_type(), neurax::hal::AcceleratorType::FPGA_DE1SOC);
        EXPECT_FALSE(accelerator->initialize()) << "FPGA initialization should fail not implemented yet";
        EXPECT_FALSE(accelerator->is_available());
        accelerator->cleanup();
    }
    else
    {
        SUCCEED() << "FPGA accelerator not available on this system, as expected.";
    }
}

/* Test the `AcceleratorFactory::get_available_accelerators` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetAvailableAccelerators)
{
    auto available = neurax::hal::AcceleratorFactory::get_available_accelerators();
    EXPECT_FALSE(available.empty());
    EXPECT_NE(std::find(available.begin(), available.end(), neurax::hal::AcceleratorType::SOFTWARE_FALLBACK), available.end());
}

/* Test the `AcceleratorFactory::get_best_available` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetBestAvailable)
{
    auto best = neurax::hal::AcceleratorFactory::get_best_available();
    auto available = neurax::hal::AcceleratorFactory::get_available_accelerators();
    EXPECT_NE(std::find(available.begin(), available.end(), best), available.end());
}

/* Test the `AcceleratorFactory::is_available` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, IsAvailable)
{
    EXPECT_TRUE(neurax::hal::AcceleratorFactory::is_available(neurax::hal::AcceleratorType::SOFTWARE_FALLBACK));
    EXPECT_TRUE(neurax::hal::AcceleratorFactory::is_available(neurax::hal::AcceleratorType::CPU_OPTIMIZED));
}

/* Test the `AcceleratorFactory::get_best_available` method with only SOFTWARE_FALLBACK available
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetBestAvailable_OnlySoftware)
{
    auto best = neurax::hal::AcceleratorFactory::get_best_available();
    if (neurax::hal::AcceleratorFactory::is_available(neurax::hal::AcceleratorType::SOFTWARE_FALLBACK) &&
        !neurax::hal::AcceleratorFactory::is_available(neurax::hal::AcceleratorType::CPU_OPTIMIZED) &&
        !neurax::hal::AcceleratorFactory::is_available(neurax::hal::AcceleratorType::GPU_OPENCL) &&
        !neurax::hal::AcceleratorFactory::is_available(neurax::hal::AcceleratorType::FPGA_DE1SOC))
    {
        EXPECT_EQ(best, neurax::hal::AcceleratorType::SOFTWARE_FALLBACK);
    }
}

/* Test the `AcceleratorFactory::get_accelerator_name` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetAcceleratorName)
{
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_name(neurax::hal::AcceleratorType::SOFTWARE_FALLBACK), "Software Fallback");
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_name(neurax::hal::AcceleratorType::CPU_OPTIMIZED), "CPU Optimized");
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_name(neurax::hal::AcceleratorType::GPU_OPENCL), "GPU OpenCL");
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_name(neurax::hal::AcceleratorType::FPGA_DE1SOC), "FPGA DE1-SoC");
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_name((neurax::hal::AcceleratorType)0xFF), "Unknown");
}

/* Test the `AcceleratorFactory::get_accelerator_info` method
 *****************************************************************************/
TEST(AcceleratorFactoryTest, GetAcceleratorInfo)
{
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_info(neurax::hal::AcceleratorType::SOFTWARE_FALLBACK), "Pure C++ software implementation (always available)");
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_info(neurax::hal::AcceleratorType::CPU_OPTIMIZED), "CPU accelerator with SIMD optimizations and threading");
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_info(neurax::hal::AcceleratorType::GPU_OPENCL), "GPU accelerator using OpenCL for parallel computation");
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_info(neurax::hal::AcceleratorType::FPGA_DE1SOC), "DE1-SoC FPGA accelerator with custom neural network hardware");
    EXPECT_EQ(neurax::hal::AcceleratorFactory::get_accelerator_info((neurax::hal::AcceleratorType)0xFF), "Unknown accelerator type");
}
