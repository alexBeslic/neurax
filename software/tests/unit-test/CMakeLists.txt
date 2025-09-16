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

# Link directories  
link_directories(${CMAKE_SOURCE_DIR}/../../build)

# Add test executable
add_executable(TestSuite_AcceleratorFactory
    hal/TestSuite_AcceleratorFactory.cpp
)

# Link with GoogleTest, pthread, and your library
target_link_libraries(TestSuite_AcceleratorFactory
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
    neurax_hal
)

# Optionally, enable test discovery for CTest
enable_testing()
add_test(NAME AcceleratorFactoryTest COMMAND TestSuite_AcceleratorFactory)

