cmake_minimum_required(VERSION 3.16)

project(NEURAX VERSION 1.0.0 LANGUAGES C CXX)

# -------------------------
# C / C++ Standard
# -------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -------------------------
# Default build type
# -------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# -------------------------
# Global compiler flags
# -------------------------
# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -g")

# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native -Wall")

# -------------------------
# Include directories
# -------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# -------------------------
# Find packages
# -------------------------
find_package(Threads REQUIRED)

# -------------------------
# Tensor library
# -------------------------
add_library(neurax_tensor STATIC
    src/tensor/DataType.cpp
    src/tensor/Shape.cpp
    src/tensor/Tensor.cpp
)

target_include_directories(neurax_tensor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# -------------------------
# BSP library (private)
# -------------------------
add_library(neurax_bsp STATIC
    src/bsp/neurax_bsp.c
)

target_include_directories(neurax_bsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(neurax_bsp
    Threads::Threads
)

# -------------------------
# HAL library
# -------------------------
add_library(neurax_hal STATIC
    src/hal/AcceleratorFactory.cpp
    src/hal/accelerators/SoftwareAccelerator.cpp
    src/hal/accelerators/FPGAAccelerator.cpp
    src/hal/accelerators/CPUAccelerator.cpp
)

target_include_directories(neurax_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(neurax_hal
    PUBLIC neurax_tensor
    PRIVATE neurax_bsp
    Threads::Threads
)

# -------------------------
# Core library
# -------------------------
add_library(neurax_core STATIC
    src/core/layers/Conv2dLayer.cpp
    src/core/layers/PoolingLayer.cpp
    src/core/layers/ActivationLayer.cpp
    src/core/network/NeuralNetwork.cpp
    src/core/NeuralNetworkBuilder.cpp
    src/core/LayerBuilder.cpp
    src/core/Conv2dBuilder.cpp
    src/core/PoolingBuilder.cpp
    src/core/ActivationBuilder.cpp
)

target_include_directories(neurax_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(neurax_core
    PUBLIC neurax_hal
    PUBLIC neurax_tensor
)

# -------------------------
# Image library
# -------------------------
add_library(neurax_image STATIC
    src/image/ImageProcessor.cpp
)

target_include_directories(neurax_image PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(neurax_image
    PUBLIC neurax_tensor
)


# -------------------------
# Installation rules
# -------------------------
install(TARGETS neurax_hal neurax_core neurax_image neurax_tensor neurax_bsp
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# -------------------------
# Doxygen documentation
# -------------------------
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
                   ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

    add_custom_target(docs
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# -------------------------
# Build info
# -------------------------
message(STATUS "NEURAX Neural Network Accelerator Build Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")

# -------------------------
# Optional examples and tests
# -------------------------
option(BUILD_EXAMPLES "Build example programs" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# -------------------------
# Custom clean target
# -------------------------
add_custom_target(clean_all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/docs
    COMMENT "Cleaning all build artifacts"
)
