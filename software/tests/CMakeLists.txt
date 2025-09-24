cmake_minimum_required(VERSION 3.16)

# Examples project
project(NEURAX_tests VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find parent project (NEURAX library)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../include)

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/../build)

add_subdirectory(core/python_tensorflow_comparison)