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
# FetchContent for dependencies
# -------------------------
include(FetchContent)

# -------------------------
# Protobuf (static for DE1-SoC cross-compilation)
# -------------------------
option(BUILD_ONNX_PARSER "Build ONNX parser with Protobuf support" ON)

if(BUILD_ONNX_PARSER)
    # Try to find system protobuf first, otherwise fetch it
    find_package(Protobuf QUIET)
    
    if(NOT Protobuf_FOUND)
        message(STATUS "System Protobuf not found, fetching via FetchContent...")
        
        set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
        set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
        set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
        
        FetchContent_Declare(
            protobuf
            GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
            GIT_TAG        v3.21.12
            GIT_SHALLOW    TRUE
            SOURCE_SUBDIR  cmake
        )
        FetchContent_MakeAvailable(protobuf)
        
        # Set protobuf variables for downstream use
        set(Protobuf_INCLUDE_DIRS ${protobuf_SOURCE_DIR}/src)
        set(Protobuf_LIBRARIES libprotobuf)
        set(Protobuf_PROTOC_EXECUTABLE $<TARGET_FILE:protoc>)
        set(PROTOBUF_FETCHED TRUE)
    else()
        message(STATUS "Found system Protobuf: ${Protobuf_VERSION}")
        set(PROTOBUF_FETCHED FALSE)
    endif()
endif()

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
    src/core/layers/DenseLayer.cpp
    src/core/layers/FlattenLayer.cpp
    src/core/layers/BatchNormLayer.cpp
    src/core/network/NeuralNetwork.cpp
    src/core/execution/GraphExecutor.cpp
    src/core/NeuralNetworkBuilder.cpp
    src/core/LayerBuilder.cpp
    src/core/Conv2dBuilder.cpp
    src/core/PoolingBuilder.cpp
    src/core/ActivationBuilder.cpp
    src/core/DenseBuilder.cpp
    src/core/FlattenBuilder.cpp
    src/core/BatchNormBuilder.cpp
    src/core/BiasAddBuilder.cpp
    src/core/layers/BiasAddLayer.cpp
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
# Parser library (ONNX)
# -------------------------
if(BUILD_ONNX_PARSER)
    # Define ONNX proto files
    # Note: Use only onnx-ml.proto as it's a superset containing all ONNX definitions plus ML extensions
    set(ONNX_PROTO_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/onnx/onnx-ml.proto
    )
    
    # Set output directory for generated files
    set(ONNX_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
    file(MAKE_DIRECTORY ${ONNX_PROTO_GEN_DIR})
    
    # Generate protobuf sources
    if(PROTOBUF_FETCHED)
        # Use custom command when protobuf was fetched
        set(ONNX_PROTO_HDRS
            ${ONNX_PROTO_GEN_DIR}/onnx-ml.pb.h
        )
        set(ONNX_PROTO_CPPS
            ${ONNX_PROTO_GEN_DIR}/onnx-ml.pb.cc
        )
        
        add_custom_command(
            OUTPUT ${ONNX_PROTO_HDRS} ${ONNX_PROTO_CPPS}
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                --cpp_out=${ONNX_PROTO_GEN_DIR}
                --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/src/parser/onnx
                ${ONNX_PROTO_SRCS}
            DEPENDS ${ONNX_PROTO_SRCS} protoc
            COMMENT "Generating ONNX protobuf sources"
            VERBATIM
        )
    else()
        # Use built-in protobuf_generate_cpp when system protobuf is available
        protobuf_generate_cpp(ONNX_PROTO_CPPS ONNX_PROTO_HDRS
            ${ONNX_PROTO_SRCS}
        )
        # Move generated files to our preferred location
        set(PROTO_CPP_OUT ${ONNX_PROTO_CPPS})
        set(PROTO_H_OUT ${ONNX_PROTO_HDRS})
    endif()
    
    # Create parser library
    add_library(neurax_parser STATIC
        src/parser/ONNXParser.cpp
        ${ONNX_PROTO_CPPS}
    )
    
    target_include_directories(neurax_parser PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${ONNX_PROTO_GEN_DIR}
    )
    
    if(PROTOBUF_FETCHED)
        target_include_directories(neurax_parser PRIVATE
            ${Protobuf_INCLUDE_DIRS}
        )
        target_link_libraries(neurax_parser
            PUBLIC neurax_core
            PUBLIC neurax_tensor
            PRIVATE ${Protobuf_LIBRARIES}
        )
    else()
        target_include_directories(neurax_parser PRIVATE
            ${Protobuf_INCLUDE_DIRS}
        )
        target_link_libraries(neurax_parser
            PUBLIC neurax_core
            PUBLIC neurax_tensor
            PRIVATE ${Protobuf_LIBRARIES}
        )
    endif()
    
    # Ensure protobuf links statically for DE1-SoC portability
    if(CMAKE_CROSSCOMPILING OR DE1_SOC_TARGET)
        set_target_properties(neurax_parser PROPERTIES
            LINK_FLAGS "-static-libstdc++ -static-libgcc"
        )
    endif()
    
    message(STATUS "ONNX Parser enabled with Protobuf support")
endif()

# 3pp libraries (commented out if not present)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3pp/cnpy")
    add_subdirectory(3pp/cnpy)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3pp/json")
    add_subdirectory(3pp/json)
endif()

# -------------------------
# Installation rules
# -------------------------
set(INSTALL_TARGETS neurax_hal neurax_core neurax_image neurax_tensor neurax_bsp)
if(BUILD_ONNX_PARSER)
    list(APPEND INSTALL_TARGETS neurax_parser)
endif()

install(TARGETS ${INSTALL_TARGETS}
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
