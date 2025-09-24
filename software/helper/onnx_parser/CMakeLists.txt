
cmake_minimum_required(VERSION 3.16)

# ONNX Parser project
project(onnx_parser VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threads
find_package(Threads REQUIRED)

# Find protobuf (use PkgConfig for compatibility)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF REQUIRED protobuf)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../../include)
include_directories(${CMAKE_SOURCE_DIR}/.)
include_directories(${PROTOBUF_INCLUDE_DIRS})

# Link directories  
link_directories(${CMAKE_SOURCE_DIR}/../../build)
link_directories(${PROTOBUF_LIBRARY_DIRS})

# Add executable
add_executable(onnx_parser 
    onnx.pb.cc
    onnx_parser.cpp
)

# Compiler flags
target_compile_options(onnx_parser PRIVATE ${PROTOBUF_CFLAGS_OTHER})

# Link libraries (matching your g++ command: -lprotobuf)
target_link_libraries(onnx_parser PRIVATE
    protobuf
    Threads::Threads
    neurax_core
    neurax_hal
    neurax_tensor
    neurax_image
    neurax_bsp
)

