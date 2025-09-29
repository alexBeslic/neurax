
cmake_minimum_required(VERSION 3.16)

# Examples project
project(NEURAX_Examples VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find parent project (NEURAX library)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/software/include)
include_directories(${CMAKE_SOURCE_DIR}/software)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Source dir: ${CMAKE_SOURCE_DIR}")
# Link directories
link_directories(${CMAKE_SOURCE_DIR}/../build)

add_executable(core_test_conv2d
    core_conv_test.cpp
)

target_link_libraries(core_test_conv2d
    PRIVATE
        neurax_tensor
        neurax_image
        neurax_core
        Threads::Threads
)


add_executable(core_test_pool
    core_pool_test.cpp
)

target_link_libraries(core_test_pool
    PRIVATE
        neurax_tensor
        neurax_image
        neurax_core
        Threads::Threads
)

add_executable(core_test_activation
    core_activation_test.cpp
)

target_link_libraries(core_test_activation
    PRIVATE
        neurax_tensor
        neurax_image
        neurax_core
        Threads::Threads
)

add_executable(core_test_dense
    core_dense_test.cpp
)

target_link_libraries(core_test_dense
    PRIVATE
        neurax_tensor
        neurax_image
        neurax_core
        Threads::Threads
)

add_executable(tf_deserializer
    tf_deserializer.cpp
)

target_link_libraries(tf_deserializer
    PRIVATE
        neurax_tensor
        neurax_image
        neurax_core
        Threads::Threads
        cnpy
        nlohmann_json::nlohmann_json
)