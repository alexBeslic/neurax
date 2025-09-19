cmake_minimum_required(VERSION 3.16)

# Examples project
project(NEURAX_Examples VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find parent project (NEURAX library)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../include)

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/../build)

add_executable(example_hal_image_blur
    hal_image_blur.cpp
)

target_link_libraries(example_hal_image_blur
    PRIVATE
        neurax_hal
        neurax_tensor
        neurax_image
        Threads::Threads
)

# Core example bluring
add_executable(example_core_image_blur
    core_image_blur.cpp
)

target_link_libraries(example_core_image_blur
    PRIVATE
        neurax_tensor
        neurax_image
        neurax_core
        Threads::Threads
)

# Core example convolution with custom kernel
add_executable(example_core_load_kernel
    core_load_kernel.cpp
)

target_link_libraries(example_core_load_kernel
    PRIVATE
        neurax_tensor
        neurax_image
        neurax_core
        Threads::Threads
)

# Set runtime library path for development
if(UNIX AND NOT APPLE)
endif()

# Print configuration
message(STATUS "NEURAX Examples Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  NEURAX Library: ${CMAKE_SOURCE_DIR}/../build/libneurax.so")
