cmake_minimum_required(VERSION 3.16)

# Examples project
project(NEURAX_Examples VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find parent project (NEURAX library)
find_package(Threads REQUIRED)

# Find GoogleTest
find_package(GTest REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../../include)
include_directories(${CMAKE_SOURCE_DIR}/../../src/hal/accelerators)

# Link directories  
link_directories(${CMAKE_SOURCE_DIR}/../../build)

# Add test executable
add_executable(unit_test_neurax_accelerator
    tensor/TestSuite_DataType.cpp
    tensor/TestSuite_Shape.cpp
    tensor/TestSuite_Tensor.cpp
    hal/accelerators/TestSuite_CPUAccelerator.cpp
    hal/accelerators/TestSuite_FPGAAccelerator.cpp
    hal/accelerators/TestSuite_SoftwareAccelerator.cpp
    hal/TestSuite_AcceleratorFactory.cpp
    core/TestSuite_NeuralNetworkBuilder.cpp
)

# Link with GoogleTest, pthread, and your library
target_link_libraries(unit_test_neurax_accelerator
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
    neurax_hal
    neurax_core
    neurax_tensor
)

# Optionally, enable test discovery for CTest
enable_testing()
add_test(NAME unit_test_neurax_accelerator COMMAND unit_test_neurax_accelerator)
gtest_discover_tests(unit_test_neurax_accelerator)