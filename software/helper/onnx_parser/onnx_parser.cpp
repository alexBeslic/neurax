#include "onnx.pb.h"
#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message_lite.h>
#include <iostream>

#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/image/ImageProcessor.hpp"
#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "neurax/core/LayerBuilder.hpp"
#include "neurax/core/Conv2dBuilder.hpp"
#include "neurax/core/ActivationBuilder.hpp"
#include "neurax/core/PoolingBuilder.hpp"
#include "neurax/core/DenseBuilder.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

using namespace neurax;
using namespace neurax::hal;
using namespace neurax::image;
using namespace neurax::core;

/*
Prepare the onnx.pb.cc and onnx.pb.h files using the protobuf compiler with the ONNX proto file:
        protoc --cpp_out=. onnx.proto

How to compile:
        g++ -std=c++17 onnx_parser.cpp onnx.pb.cc -lprotobuf -o onnx_parser
*/

int parse_onnx_model(std::string model_path, NeuralNetworkBuilder &builder)
{
    // Note: This parser assumes NHWC format for all tensor operations
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::ifstream input(model_path, std::ios::binary);
    if (!input)
    {
        std::cerr << "ONNX file not found or cannot be opened." << std::endl;
        return 1;
    }

    onnx::ModelProto model;
    google::protobuf::io::IstreamInputStream zero_copy_input(&input);
    if (!model.ParseFromZeroCopyStream(&zero_copy_input))
    {
        std::cerr << "Failed to parse ONNX model." << std::endl;
        return 1;
    }

    const auto &graph = model.graph();

    // Map initializers (weights/bias) by name and their tensor data
    std::map<std::string, std::pair<float *, std::vector<int64_t>>> weights_map;
    for (const auto &init : graph.initializer())
    {
        size_t num_elements = 1;
        std::vector<int64_t> dims;
        for (int i = 0; i < init.dims_size(); ++i)
        {
            dims.push_back(init.dims(i));
            num_elements *= init.dims(i);
        }

        float *data = new float[num_elements];
        memcpy(data, init.raw_data().data(), num_elements * sizeof(float));
        weights_map[init.name()] = std::make_pair(data, dims);
    }

    for (const auto &node : graph.node())
    {
        try
        {
            if (node.op_type() == "Conv")
            {
                // Extract Conv2D parameters (only regular convolution supported, group = 1)
                std::vector<int> kernel_shape;
                std::vector<int> strides = {1, 1};
                std::vector<int> pads = {0, 0, 0, 0};
                int group = 1;

                for (const auto &attr : node.attribute())
                {
                    if (attr.name() == "kernel_shape")
                        kernel_shape.assign(attr.ints().begin(), attr.ints().end());
                    else if (attr.name() == "strides")
                        strides.assign(attr.ints().begin(), attr.ints().end());
                    else if (attr.name() == "pads")
                        pads.assign(attr.ints().begin(), attr.ints().end());
                    else if (attr.name() == "group")
                    {
                        group = attr.i();
                        // if (group != 1)
                        // {
                        //     std::cerr << "Warning: Grouped convolution (group=" << group << ") not supported, skipping layer: " << node.name() << std::endl;
                        //     continue; // Skip this layer
                        // }
                    }
                }

                // Get weights and bias tensors
                if (node.input_size() > 1 && weights_map.count(node.input(1)))
                {
                    auto &weight_data = weights_map[node.input(1)];
                    auto &weight_dims = weight_data.second;

                    // Create weight tensor (using NHWC format: [height, width, input_channels, output_channels])
                    Shape weight_shape({(size_t)weight_dims[2], (size_t)weight_dims[3],
                                        (size_t)weight_dims[1], (size_t)weight_dims[0]});
                    tensor::Tensor weights(weight_shape, weight_data.first, DataType::FLOAT32);

                    tensor::Tensor bias;
                    if (node.input_size() > 2 && weights_map.count(node.input(2)))
                    {
                        auto &bias_data = weights_map[node.input(2)];
                        Shape bias_shape({(size_t)bias_data.second[0]});
                        bias = tensor::Tensor(bias_shape, bias_data.first, DataType::FLOAT32);
                    }
                    else
                    {
                        // Create zero bias if not provided
                        Shape bias_shape({(size_t)weight_dims[0]});
                        float *zero_bias = new float[weight_dims[0]]();
                        bias = tensor::Tensor(bias_shape, zero_bias, DataType::FLOAT32);
                    }

                    // Build Conv2D layer (regular convolution only)
                    auto layer = LayerBuilder::conv2d()
                                     .inputChanels(weight_dims[1] * group)
                                     .outputChanels(weight_dims[0])
                                     .kernelSize(kernel_shape.empty() ? 3 : kernel_shape[0])
                                     .stride(strides.empty() ? 1 : strides[0])
                                     .padding(pads.empty() ? 0 : pads[0])
                                     .addWeights(weights, bias)
                                     .build();

                    builder.addLayer(layer);
                }
            }
            else if (node.op_type() == "MaxPool")
            {
                // Extract MaxPool parameters
                std::vector<int> kernel_shape = {2, 2};
                std::vector<int> strides = {2, 2};

                for (const auto &attr : node.attribute())
                {
                    if (attr.name() == "kernel_shape")
                        kernel_shape.assign(attr.ints().begin(), attr.ints().end());
                    else if (attr.name() == "strides")
                        strides.assign(attr.ints().begin(), attr.ints().end());
                }

                auto layer = LayerBuilder::pool()
                                 .poolSize(kernel_shape.empty() ? 2 : kernel_shape[0])
                                 .stride(strides.empty() ? 2 : strides[0])
                                 .type(neurax::hal::PoolingType::MAX)
                                 .build();

                builder.addLayer(layer);
            }
            else if (node.op_type() == "AveragePool")
            {
                // Extract AveragePool parameters
                std::vector<int> kernel_shape = {2, 2};
                std::vector<int> strides = {2, 2};

                for (const auto &attr : node.attribute())
                {
                    if (attr.name() == "kernel_shape")
                        kernel_shape.assign(attr.ints().begin(), attr.ints().end());
                    else if (attr.name() == "strides")
                        strides.assign(attr.ints().begin(), attr.ints().end());
                }

                auto layer = LayerBuilder::pool()
                                 .poolSize(kernel_shape.empty() ? 2 : kernel_shape[0])
                                 .stride(strides.empty() ? 2 : strides[0])
                                 .type(neurax::hal::PoolingType::AVERAGE)
                                 .build();

                builder.addLayer(layer);
            }
            else if (node.op_type() == "Relu")
            {
                auto layer = LayerBuilder::activation()
                                 .type(neurax::hal::ActivationType::RELU)
                                 .build();

                builder.addLayer(layer);
            }
            else if (node.op_type() == "Sigmoid")
            {
                auto layer = LayerBuilder::activation()
                                 .type(neurax::hal::ActivationType::SIGMOID)
                                 .build();

                builder.addLayer(layer);
            }
            else if (node.op_type() == "Tanh")
            {
                auto layer = LayerBuilder::activation()
                                 .type(neurax::hal::ActivationType::TANH)
                                 .build();

                builder.addLayer(layer);
            }
            else if (node.op_type() == "Softmax")
            {
                auto layer = LayerBuilder::activation()
                                 .type(neurax::hal::ActivationType::SOFTMAX)
                                 .build();

                builder.addLayer(layer);
            }
            else if (node.op_type() == "Gemm")
            {
                // Gemm is typically used for fully connected layers
                if (node.input_size() > 1 && weights_map.count(node.input(1)))
                {
                    auto &weight_data = weights_map[node.input(1)];
                    auto &weight_dims = weight_data.second;

                    // Create weight tensor for dense layer (ONNX format: [input_features, output_features])
                    Shape weight_shape({(size_t)weight_dims[0], (size_t)weight_dims[1]});
                    tensor::Tensor weights(weight_shape, weight_data.first, DataType::FLOAT32);

                    tensor::Tensor bias;
                    if (node.input_size() > 2 && weights_map.count(node.input(2)))
                    {
                        auto &bias_data = weights_map[node.input(2)];
                        Shape bias_shape({(size_t)bias_data.second[0]});
                        bias = tensor::Tensor(bias_shape, bias_data.first, DataType::FLOAT32);
                    }
                    else
                    {
                        // Create zero bias if not provided
                        Shape bias_shape({(size_t)weight_dims[0]});
                        float *zero_bias = new float[weight_dims[0]]();
                        bias = tensor::Tensor(bias_shape, zero_bias, DataType::FLOAT32);
                    }

                    auto layer = LayerBuilder::dense()
                                     .units(weight_dims[0])
                                     .addWeights(weights, bias)
                                     .build();

                    builder.addLayer(layer);
                }
            }
            else if (node.op_type() == "Flatten" || node.op_type() == "Reshape")
            {
                // Add flatten layer for reshape operations that flatten the input
                // Note: Assumes NHWC format for proper flattening order
                auto layer = LayerBuilder::flatten().build();
                builder.addLayer(layer);
            }
            else if (node.op_type() == "Dropout")
            {
                // Skip dropout layers during inference - they're pass-through
                std::cout << "Skipping Dropout layer (not needed for inference): " << node.name() << std::endl;
                continue;
            }
            else if (node.op_type() == "BatchNormalization")
            {
                float epsilon = 1e-5f; // default
                float momentum = 0.1f; // default

                // Extract attributes
                for (const auto &attr : node.attribute())
                {
                    if (attr.name() == "epsilon")
                        epsilon = attr.f();
                    else if (attr.name() == "momentum")
                        momentum = attr.f();
                }

                // Build BatchNorm layer with your parameters
                auto layer = LayerBuilder::batchnorm()
                                 .epsilon(epsilon)
                                 .momentum(momentum)
                                 .build();

                builder.addLayer(layer);
            }
            else if (node.op_type() == "Dropout")
            {
                // During inference, dropout is just a pass-through (identity operation)
                std::cout << "Skipping Dropout layer (pass-through during inference): "
                          << node.name() << std::endl;
                // No layer needed - input flows directly to next layer
            }
            else
            {
                std::cout << "Skipping unsupported layer type: " << node.op_type()
                          << " (name: " << node.name() << ")" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error processing node " << node.name() << " (" << node.op_type()
                      << "): " << e.what() << std::endl;
            // Continue processing other nodes
        }
    }

    // Clean up allocated memory
    for (auto &pair : weights_map)
    {
        delete[] pair.second.first;
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}

int main()
{
    NeuralNetworkBuilder builder;
    std::string model_path = "bvlcalexnet-12.onnx"; // specify your model path here

    std::cout << "Parsing ONNX model: " << model_path << std::endl;

    if (parse_onnx_model(model_path, builder) != 0)
    {
        std::cerr << "Failed to parse ONNX model." << std::endl;
        return 1;
    }
    else
    {
        std::cout << "Successfully parsed ONNX model and built neural network." << std::endl;

        // Build the final network
        try
        {
            auto network = builder.build();
            std::cout << "Neural network successfully built!" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error building network: " << e.what() << std::endl;
            return 1;
        }
    }

    return 0;
}