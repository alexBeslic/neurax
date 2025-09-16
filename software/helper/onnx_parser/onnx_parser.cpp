#include "onnx.pb.h"
#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message_lite.h>
#include <iostream>

/*
Prepare the onnx.pb.cc and onnx.pb.h files using the protobuf compiler with the ONNX proto file:
        protoc --cpp_out=. onnx.proto

How to compile:
        g++ -std=c++17 onnx_parser.cpp onnx.pb.cc -lprotobuf -o onnx_parser
*/

enum class InstructionType
{
    CONV2D,
    POOLING,
    ACTIVATION,
    ADD,
    SUB,
    CLIP,
    CONCAT,
    CONSTANT,
    GATHER,
    GEMM,
    RESHAPE,
    SHAPE,
    UNSQUEEZE,
    OTHER
};

enum class PoolingType
{
    MAX,
    AVERAGE,
    GLOBAL_AVERAGE,
    UNKNOWN
};

enum class ActivationType
{
    RELU,
    SIGMOID,
    TANH,
    UNKNOWN
};

struct LayerInstruction
{
    InstructionType type;
    int index;
    std::string name;
    std::string op_type;

    // Conv2D
    std::vector<int> kernel_shape;
    std::vector<int> strides;
    std::vector<int> pads;
    int group = 1;
    std::vector<int> dilations;
    float *weights = nullptr;
    float *bias = nullptr;

    // Pooling
    PoolingType pool_type = PoolingType::UNKNOWN;

    // Activation
    ActivationType activation_type = ActivationType::UNKNOWN;

    // Expanded fields
    int input_channels = 0;
    int output_channels = 0;
    std::vector<int> input_shape;
    std::vector<int> output_shape;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;

    // Clip
    float clip_min = 0.0f;
    float clip_max = 0.0f;

    // Concat
    int concat_axis = 0;

    // Gather
    int gather_axis = 0;

    // Gemm
    float gemm_alpha = 1.0f;
    float gemm_beta = 1.0f;
    int gemm_transA = 0;
    int gemm_transB = 0;

    // Reshape
    std::vector<int> reshape_shape;

    // Unsqueeze
    std::vector<int> unsqueeze_axes;

    // Constant
    std::vector<float> constant_value;
};

int parse_onnx_model(std::string model_path, std::vector<LayerInstruction> &layer_instructions)
{
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

    // Map initializers (weights/bias) by name
    std::map<std::string, float *> weights_map;
    for (const auto &init : graph.initializer())
    {
        size_t num_elements = 1;
        for (int i = 0; i < init.dims_size(); ++i)
            num_elements *= init.dims(i);

        float *data = new float[num_elements];
        memcpy(data, init.raw_data().data(), num_elements * sizeof(float));
        weights_map[init.name()] = data;
    }

    std::vector<LayerInstruction> instructions;
    int layer_idx = 0;

    for (const auto &node : graph.node())
    {
        LayerInstruction instr;
        instr.index = layer_idx++;
        instr.name = node.name();
        instr.op_type = node.op_type();

        // Store input/output tensor names
        for (const auto &in : node.input())
            instr.inputs.push_back(in);
        for (const auto &out : node.output())
            instr.outputs.push_back(out);

        if (node.op_type() == "Conv")
        {
            instr.type = InstructionType::CONV2D;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "kernel_shape")
                    instr.kernel_shape.assign(attr.ints().begin(), attr.ints().end());
                else if (attr.name() == "strides")
                    instr.strides.assign(attr.ints().begin(), attr.ints().end());
                else if (attr.name() == "pads")
                    instr.pads.assign(attr.ints().begin(), attr.ints().end());
                else if (attr.name() == "group")
                    instr.group = attr.i();
                else if (attr.name() == "dilations")
                    instr.dilations.assign(attr.ints().begin(), attr.ints().end());
            }
            // Get weights and bias
            instr.weights = weights_map.count(node.input(1)) ? weights_map[node.input(1)] : nullptr;
            instr.bias = node.input_size() > 2 && weights_map.count(node.input(2)) ? weights_map[node.input(2)] : nullptr;

            // Get input/output channels from weights initializer
            auto it = graph.initializer().begin();
            for (; it != graph.initializer().end(); ++it)
            {
                if (it->name() == node.input(1))
                    break;
            }
            if (it != graph.initializer().end() && it->dims_size() >= 2)
            {
                instr.output_channels = it->dims(0);
                instr.input_channels = it->dims(1) * instr.group;
            }
        }
        else if (node.op_type() == "MaxPool")
        {
            instr.type = InstructionType::POOLING;
            instr.pool_type = PoolingType::MAX;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "kernel_shape")
                    instr.kernel_shape.assign(attr.ints().begin(), attr.ints().end());
                else if (attr.name() == "strides")
                    instr.strides.assign(attr.ints().begin(), attr.ints().end());
                else if (attr.name() == "pads")
                    instr.pads.assign(attr.ints().begin(), attr.ints().end());
            }
        }
        else if (node.op_type() == "AveragePool")
        {
            instr.type = InstructionType::POOLING;
            instr.pool_type = PoolingType::AVERAGE;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "kernel_shape")
                    instr.kernel_shape.assign(attr.ints().begin(), attr.ints().end());
                else if (attr.name() == "strides")
                    instr.strides.assign(attr.ints().begin(), attr.ints().end());
                else if (attr.name() == "pads")
                    instr.pads.assign(attr.ints().begin(), attr.ints().end());
            }
        }
        else if (node.op_type() == "Relu")
        {
            instr.type = InstructionType::ACTIVATION;
            instr.activation_type = ActivationType::RELU;
        }
        else if (node.op_type() == "Sigmoid")
        {
            instr.type = InstructionType::ACTIVATION;
            instr.activation_type = ActivationType::SIGMOID;
        }
        else if (node.op_type() == "Tanh")
        {
            instr.type = InstructionType::ACTIVATION;
            instr.activation_type = ActivationType::TANH;
        }
        else if (node.op_type() == "Add")
        {
            instr.type = InstructionType::ADD;
        }
        else if (node.op_type() == "Sub")
        {
            instr.type = InstructionType::SUB;
        }
        else if (node.op_type() == "Clip")
        {
            instr.type = InstructionType::CLIP;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "min")
                    instr.clip_min = attr.f();
                else if (attr.name() == "max")
                    instr.clip_max = attr.f();
            }
        }
        else if (node.op_type() == "Concat")
        {
            instr.type = InstructionType::CONCAT;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "axis")
                    instr.concat_axis = attr.i();
            }
        }
        else if (node.op_type() == "Gather")
        {
            instr.type = InstructionType::GATHER;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "axis")
                    instr.gather_axis = attr.i();
            }
        }
        else if (node.op_type() == "Gemm")
        {
            instr.type = InstructionType::GEMM;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "alpha")
                    instr.gemm_alpha = attr.f();
                else if (attr.name() == "beta")
                    instr.gemm_beta = attr.f();
                else if (attr.name() == "transA")
                    instr.gemm_transA = attr.i();
                else if (attr.name() == "transB")
                    instr.gemm_transB = attr.i();
            }
        }
        else if (node.op_type() == "Reshape")
        {
            instr.type = InstructionType::RESHAPE;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "shape")
                    instr.reshape_shape.assign(attr.ints().begin(), attr.ints().end());
            }
        }
        else if (node.op_type() == "Shape")
        {
            instr.type = InstructionType::SHAPE;
        }
        else if (node.op_type() == "Unsqueeze")
        {
            instr.type = InstructionType::UNSQUEEZE;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "axes")
                    instr.unsqueeze_axes.assign(attr.ints().begin(), attr.ints().end());
            }
        }
        else if (node.op_type() == "Constant")
        {
            instr.type = InstructionType::CONSTANT;
            for (const auto &attr : node.attribute())
            {
                if (attr.name() == "value" && attr.has_t())
                {
                    const auto &tensor = attr.t();
                    if (tensor.data_type() == onnx::TensorProto_DataType_FLOAT)
                    {
                        size_t num_elements = 1;
                        for (int i = 0; i < tensor.dims_size(); ++i)
                            num_elements *= tensor.dims(i);
                        instr.constant_value.resize(num_elements);
                        memcpy(instr.constant_value.data(), tensor.raw_data().data(), num_elements * sizeof(float));
                    }
                }
            }
        }
        else
        {
            instr.type = InstructionType::OTHER;
        }

        // Optionally: Parse input/output shapes from value_info
        for (const auto &value : graph.value_info())
        {
            if (!instr.inputs.empty() && value.name() == instr.inputs[0])
            {
                for (const auto &dim : value.type().tensor_type().shape().dim())
                    instr.input_shape.push_back(dim.dim_value());
            }
            if (!instr.outputs.empty() && value.name() == instr.outputs[0])
            {
                for (const auto &dim : value.type().tensor_type().shape().dim())
                    instr.output_shape.push_back(dim.dim_value());
            }
        }

        instructions.push_back(instr);
    }

    layer_instructions = instructions;

    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

int main()
{
    std::vector<LayerInstruction> instructions;
    std::string model_path = "mobilenetv2-10.onnx"; // specify your model path here
    if (parse_onnx_model(model_path, instructions) != 0)
    {
        std::cerr << "Failed to parse ONNX model." << std::endl;
        return 1;
    }
    else
    {
        // Example: print summary
        for (const auto &instr : instructions)
        {
            std::cout << "Layer " << instr.index << ": " << instr.name << " (" << instr.op_type << ")\n";
            std::cout << "  Inputs: ";
            for (const auto &in : instr.inputs)
                std::cout << in << " ";
            std::cout << "\n  Outputs: ";
            for (const auto &out : instr.outputs)
                std::cout << out << " ";
            std::cout << "\n  Input shape: ";
            for (const auto &s : instr.input_shape)
                std::cout << s << " ";
            std::cout << "\n  Output shape: ";
            for (const auto &s : instr.output_shape)
                std::cout << s << " ";
            std::cout << "\n  Input channels: " << instr.input_channels << " Output channels: " << instr.output_channels << "\n";
        }

        std::cout << instructions[0].kernel_shape[0] << std::endl; // Example access to kernel shape
        for (const auto &s : instructions[0].strides)
            std::cout << s << " "; // Example access to strides
        std::cout << std::endl;
        for (const auto &p : instructions[0].pads)
            std::cout << p << " "; // Example access to pads
        std::cout << std::endl;
        std::cout << instructions[0].group << std::endl; // Example access to group
        for (const auto &d : instructions[0].dilations)
            std::cout << d << " "; // Example access to dilations
        std::cout << std::endl;
    }

    return 0;
}