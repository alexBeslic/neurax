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

add_executable(test_hal
    hal/test_hal.cpp
)

target_link_libraries(test_hal
    PRIVATE
        neurax_hal
        neurax_tensor
        Threads::Threads
)


# Set runtime library path for development
if(UNIX AND NOT APPLE)
endif()

# Print configuration
message(STATUS "NEURAX Tests Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  NEURAX Library: ${CMAKE_SOURCE_DIR}/../build/libneurax.so")
