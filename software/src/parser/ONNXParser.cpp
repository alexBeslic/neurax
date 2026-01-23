/**
 * @file ONNXParser.cpp
 * @brief Implementation of ONNX model parser for NEURAX inference engine
 *
 * @author NEURAX Development Team
 * @date January 2026
 * @version 1.0
 */

#include "neurax/parser/ONNXParser.hpp"
#include "neurax/core/LayerBuilder.hpp"
#include "neurax/tensor/Tensor.hpp"

// Include generated protobuf headers
#include "onnx-ml.pb.h"

// Protobuf IO headers for memory-efficient parsing
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fstream>
#include <algorithm>
#include <cstring>
#include <set>
#include <map>

namespace neurax {
namespace parser {

// ======================== Constructor / Destructor ========================

ONNXParser::ONNXParser() 
    : config_() 
{}

ONNXParser::ONNXParser(const ONNXParserConfig& config)
    : config_(config)
{}

ONNXParser::~ONNXParser() = default;

ONNXParser::ONNXParser(ONNXParser&&) noexcept = default;
ONNXParser& ONNXParser::operator=(ONNXParser&&) noexcept = default;

// ======================== Public Methods ========================

std::unique_ptr<core::INetwork> ONNXParser::LoadModel(const std::string& path) {
    // Open file with binary mode
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        throw ONNXParseException("Failed to open model file: " + path);
    }

    // Memory-efficient parsing using zero-copy stream
    google::protobuf::io::IstreamInputStream zeroCopy(&input);
    google::protobuf::io::CodedInputStream coded(&zeroCopy);

    // Set size limit for large models (configurable)
    coded.SetTotalBytesLimit(static_cast<int>(config_.max_model_size_bytes));

    // Parse the model
    onnx::ModelProto model;
    if (!model.ParseFromCodedStream(&coded)) {
        throw ONNXParseException("Failed to parse ONNX model from file: " + path);
    }

    // Validate opset version
    validateOpset(model);

    // Extract model metadata
    model_metadata_.clear();
    if (model.has_producer_name()) {
        model_metadata_["producer_name"] = model.producer_name();
    }
    if (model.has_producer_version()) {
        model_metadata_["producer_version"] = model.producer_version();
    }
    if (model.has_ir_version()) {
        model_metadata_["ir_version"] = std::to_string(model.ir_version());
    }

    // Get the graph
    if (!model.has_graph()) {
        throw ONNXParseException("Model does not contain a graph");
    }
    const onnx::GraphProto& graph = model.graph();

    // Clear previous state
    tensor_map_.clear();
    shape_map_.clear();
    dtype_map_.clear();
    input_names_.clear();
    output_names_.clear();

    // Initialize the builder
    core::NeuralNetworkBuilder builder;
    builder.useGraphExecution(true);
    builder.useAccelerator(config_.accelerator_type);

    // Parse initializers (weights, biases) first
    parseInitializers(graph, builder);

    // Parse graph inputs (non-initializer inputs are network inputs)
    parseGraphInputs(graph, builder);

    // Parse graph outputs
    parseGraphOutputs(graph, builder);

    // Parse all nodes in topological order
    for (int i = 0; i < graph.node_size(); ++i) {
        parseNode(graph.node(i), builder);
    }

    // Set the network inputs and outputs
    builder.setInputs(input_names_);
    builder.setOutputs(output_names_);

    // Build and return the network
    return builder.build();
}

std::unique_ptr<core::INetwork> ONNXParser::LoadModelFromBuffer(const void* data, size_t size) {
    if (!data || size == 0) {
        throw ONNXParseException("Invalid buffer: null or empty");
    }

    // Use ArrayInputStream for buffer parsing
    google::protobuf::io::ArrayInputStream arrayStream(data, static_cast<int>(size));
    google::protobuf::io::CodedInputStream coded(&arrayStream);
    coded.SetTotalBytesLimit(static_cast<int>(config_.max_model_size_bytes));

    onnx::ModelProto model;
    if (!model.ParseFromCodedStream(&coded)) {
        throw ONNXParseException("Failed to parse ONNX model from buffer");
    }

    // Validate and parse (same as file-based)
    validateOpset(model);

    if (!model.has_graph()) {
        throw ONNXParseException("Model does not contain a graph");
    }
    const onnx::GraphProto& graph = model.graph();

    tensor_map_.clear();
    shape_map_.clear();
    dtype_map_.clear();
    input_names_.clear();
    output_names_.clear();

    core::NeuralNetworkBuilder builder;
    builder.useGraphExecution(true);
    builder.useAccelerator(config_.accelerator_type);

    parseInitializers(graph, builder);
    parseGraphInputs(graph, builder);
    parseGraphOutputs(graph, builder);

    // Parse all nodes in topological order
    for (int i = 0; i < graph.node_size(); ++i) {
        parseNode(graph.node(i), builder);
    }

    builder.setInputs(input_names_);
    builder.setOutputs(output_names_);

    return builder.build();
}

// ======================== Validation ========================

void ONNXParser::validateOpset(const onnx::ModelProto& model) {
    int64_t opset_version = 0;

    // Find the default (no domain or 'ai.onnx') opset import
    for (int i = 0; i < model.opset_import_size(); ++i) {
        const auto& opset = model.opset_import(i);
        if (opset.domain().empty() || opset.domain() == "ai.onnx") {
            opset_version = opset.version();
            break;
        }
    }

    if (opset_version == 0) {
        // No opset found, assume default
        opset_version = 11;
    }

    if (opset_version < config_.min_opset_version) {
        throw ONNXParseException(
            "Unsupported opset version " + std::to_string(opset_version) +
            ". Minimum required: " + std::to_string(config_.min_opset_version));
    }

    if (opset_version > config_.max_opset_version) {
        throw ONNXParseException(
            "Unsupported opset version " + std::to_string(opset_version) +
            ". Maximum supported: " + std::to_string(config_.max_opset_version));
    }

    model_metadata_["opset_version"] = std::to_string(opset_version);
}

// ======================== Graph Parsing ========================

void ONNXParser::parseGraphInputs(const onnx::GraphProto& graph,
                                   core::NeuralNetworkBuilder& builder) {
    // Collect initializer names (these are constants, not runtime inputs)
    std::set<std::string> initializer_names;
    for (int i = 0; i < graph.initializer_size(); ++i) {
        initializer_names.insert(graph.initializer(i).name());
    }

    // Parse graph inputs (excluding initializers)
    for (int i = 0; i < graph.input_size(); ++i) {
        const auto& input = graph.input(i);
        const std::string& name = input.name();

        // Skip if this is an initializer (weight/bias)
        if (initializer_names.count(name) > 0) {
            continue;
        }

        // Extract shape and dtype
        tensor::Shape shape = extractShape(input);
        tensor::DataType dtype = extractDataType(input);

        // Register as non-constant tensor (network input)
        builder.registerTensor(name, shape, dtype, false);

        // Store metadata
        shape_map_[name] = shape;
        dtype_map_[name] = dtype;
        input_names_.push_back(name);
    }
}

void ONNXParser::parseGraphOutputs(const onnx::GraphProto& graph,
                                    core::NeuralNetworkBuilder& builder) {
    (void)builder;  // Not needed for output registration, just recording names
    
    for (int i = 0; i < graph.output_size(); ++i) {
        const auto& output = graph.output(i);
        const std::string& name = output.name();

        // Extract shape and dtype if available
        if (output.has_type() && output.type().has_tensor_type()) {
            tensor::Shape shape = extractShape(output);
            tensor::DataType dtype = extractDataType(output);
            shape_map_[name] = shape;
            dtype_map_[name] = dtype;
        }

        output_names_.push_back(name);
    }
}

void ONNXParser::parseInitializers(const onnx::GraphProto& graph,
                                    core::NeuralNetworkBuilder& builder) {
    for (int i = 0; i < graph.initializer_size(); ++i) {
        const onnx::TensorProto& tensor_proto = graph.initializer(i);
        const std::string& name = tensor_proto.name();

        // Convert ONNX tensor to NEURAX tensor
        tensor::Tensor tensor = convertTensor(tensor_proto);

        // Store in tensor map for later reference
        tensor_map_[name] = std::move(tensor);

        // Also register shape/dtype
        const tensor::Tensor& stored = tensor_map_[name];
        shape_map_[name] = stored.shape();
        dtype_map_[name] = stored.dtype();

        // Register as constant tensor in the graph
        builder.registerTensor(name, stored.shape(), stored.dtype(), true);
    }
}

// ======================== Node Parsing ========================

void ONNXParser::parseNode(const onnx::NodeProto& node,
                           core::NeuralNetworkBuilder& builder) {
    ONNXOpType op_type = mapOpType(node.op_type());

    switch (op_type) {
        case ONNXOpType::CONV:
            parseConvNode(node, builder);
            break;
        case ONNXOpType::RELU:
            parseReluNode(node, builder);
            break;
        case ONNXOpType::GEMM:
            parseGemmNode(node, builder);
            break;
        case ONNXOpType::MATMUL:
            parseMatMulNode(node, builder);
            break;
        case ONNXOpType::MAXPOOL:
            parseMaxPoolNode(node, builder);
            break;
        case ONNXOpType::AVGPOOL:
            parseAvgPoolNode(node, builder);
            break;
        case ONNXOpType::ADD:
            parseAddNode(node, builder);
            break;
        case ONNXOpType::BATCHNORM:
            parseBatchNormNode(node, builder);
            break;
        case ONNXOpType::FLATTEN:
            parseFlattenNode(node, builder);
            break;
        case ONNXOpType::RESHAPE:
            parseReshapeNode(node, builder);
            break;
        case ONNXOpType::SOFTMAX:
            parseSoftmaxNode(node, builder);
            break;
        case ONNXOpType::SIGMOID:
            parseSigmoidNode(node, builder);
            break;
        case ONNXOpType::TANH:
            parseTanhNode(node, builder);
            break;
        case ONNXOpType::UNSUPPORTED:
        default:
            if (config_.strict_mode) {
                throw ONNXParseException("Unsupported operator: " + node.op_type());
            }
            // In non-strict mode, skip unsupported ops with a warning
            // (In production, you might want to log this)
            break;
    }
}

ONNXOpType ONNXParser::mapOpType(const std::string& op_type) {
    static const std::map<std::string, ONNXOpType> op_map = {
        {"Conv", ONNXOpType::CONV},
        {"Relu", ONNXOpType::RELU},
        {"Gemm", ONNXOpType::GEMM},
        {"MatMul", ONNXOpType::MATMUL},
        {"MaxPool", ONNXOpType::MAXPOOL},
        {"AveragePool", ONNXOpType::AVGPOOL},
        {"GlobalAveragePool", ONNXOpType::AVGPOOL},
        {"Add", ONNXOpType::ADD},
        {"BatchNormalization", ONNXOpType::BATCHNORM},
        {"Flatten", ONNXOpType::FLATTEN},
        {"Reshape", ONNXOpType::RESHAPE},
        {"Softmax", ONNXOpType::SOFTMAX},
        {"Sigmoid", ONNXOpType::SIGMOID},
        {"Tanh", ONNXOpType::TANH}
    };

    auto it = op_map.find(op_type);
    return (it != op_map.end()) ? it->second : ONNXOpType::UNSUPPORTED;
}

// ======================== Individual Node Parsers ========================

// Helper function to transpose Conv weights from ONNX format (OIHW) to NEURAX format (HWIO)
static tensor::Tensor transposeConvWeights(const tensor::Tensor& onnx_weights) {
    const auto& shape = onnx_weights.shape();
    if (shape.size() != 4) {
        throw ONNXParseException("Conv weights must be 4D");
    }
    
    // ONNX format: [O, I, H, W] (out_channels, in_channels, kernel_height, kernel_width)
    // NEURAX format: [H, W, I, O] (kernel_height, kernel_width, in_channels, out_channels)
    size_t O = shape[0];
    size_t I = shape[1];
    size_t H = shape[2];
    size_t W = shape[3];
    
    tensor::Shape new_shape({H, W, I, O});
    tensor::Tensor result(new_shape, tensor::DataType::FLOAT32);
    
    const float* src = onnx_weights.data_ptr<float>();
    float* dst = result.data_ptr<float>();
    
    // Transpose: src[o,i,h,w] -> dst[h,w,i,o]
    for (size_t o = 0; o < O; ++o) {
        for (size_t i = 0; i < I; ++i) {
            for (size_t h = 0; h < H; ++h) {
                for (size_t w = 0; w < W; ++w) {
                    // Source index: o*I*H*W + i*H*W + h*W + w
                    size_t src_idx = o * I * H * W + i * H * W + h * W + w;
                    // Dest index: h*W*I*O + w*I*O + i*O + o
                    size_t dst_idx = h * W * I * O + w * I * O + i * O + o;
                    dst[dst_idx] = src[src_idx];
                }
            }
        }
    }
    
    return result;
}

void ONNXParser::parseConvNode(const onnx::NodeProto& node,
                                core::NeuralNetworkBuilder& builder) {
    // Inputs: X (data), W (weights), B (bias, optional)
    if (node.input_size() < 2) {
        throw ONNXParseException("Conv node requires at least 2 inputs (data, weights)");
    }

    const std::string& weights_name = node.input(1);

    // Check if weights exist in tensor_map
    auto weights_it = tensor_map_.find(weights_name);
    if (weights_it == tensor_map_.end()) {
        throw ONNXParseException("Conv weights not found: " + weights_name);
    }
    const tensor::Tensor& onnx_weights = weights_it->second;

    // Transpose weights from ONNX format (OIHW) to NEURAX format (HWIO)
    tensor::Tensor weights = transposeConvWeights(onnx_weights);

    // Extract attributes
    std::vector<int64_t> kernel_shape = getIntsAttr(node, "kernel_shape");
    std::vector<int64_t> strides = getIntsAttr(node, "strides");
    std::vector<int64_t> pads = getIntsAttr(node, "pads");
    std::vector<int64_t> dilations = getIntsAttr(node, "dilations");
    std::string auto_pad = getStringAttr(node, "auto_pad", "NOTSET");
    int64_t group = getIntAttr(node, "group", 1);
    (void)dilations;  // Suppress unused warning
    (void)group;

    // Default values if not specified
    // After transpose, weights shape is [H, W, I, O]
    const auto& ws = weights.shape();
    if (kernel_shape.empty()) {
        if (ws.size() >= 4) {
            kernel_shape = {static_cast<int64_t>(ws[0]), static_cast<int64_t>(ws[1])};
        }
    }
    if (strides.empty()) {
        strides = {1, 1};
    }
    
    // Handle auto_pad
    if (pads.empty()) {
        if (auto_pad == "SAME_UPPER" || auto_pad == "SAME_LOWER") {
            // Calculate "same" padding: output_size = ceil(input_size / stride)
            // For stride=1: output_size = input_size, so pad = (kernel_size - 1) / 2
            int64_t kh = kernel_shape[0];
            int64_t kw = kernel_shape[1];
            int64_t pad_h = (kh - 1) / 2;
            int64_t pad_w = (kw - 1) / 2;
            if (auto_pad == "SAME_UPPER") {
                // Extra padding goes to bottom/right
                int64_t pad_h_extra = (kh - 1) - pad_h;
                int64_t pad_w_extra = (kw - 1) - pad_w;
                pads = {pad_h, pad_w, pad_h_extra, pad_w_extra};  // [top, left, bottom, right]
            } else {
                // SAME_LOWER: extra padding goes to top/left
                int64_t pad_h_extra = (kh - 1) - pad_h;
                int64_t pad_w_extra = (kw - 1) - pad_w;
                pads = {pad_h_extra, pad_w_extra, pad_h, pad_w};
            }
        } else {
            pads = {0, 0, 0, 0};  // NOTSET or VALID: no padding
        }
    }

    // Get weight shape after transpose: [kH, kW, in_channels, out_channels]
    size_t in_channels = ws[2];
    size_t out_channels = ws[3];
    if (group > 1) {
        in_channels *= static_cast<size_t>(group);
    }

    // Get optional bias
    tensor::Tensor bias;
    if (node.input_size() > 2 && !node.input(2).empty()) {
        const std::string& bias_name = node.input(2);
        auto bias_it = tensor_map_.find(bias_name);
        if (bias_it != tensor_map_.end()) {
            bias = bias_it->second;
        } else {
            bias = tensor::Tensor::zeros(tensor::Shape({out_channels}), tensor::DataType::FLOAT32);
        }
    } else {
        bias = tensor::Tensor::zeros(tensor::Shape({out_channels}), tensor::DataType::FLOAT32);
    }

    // Build Conv2d layer - chain directly on the temporary returned by conv2d()
    core::ILayer* layer = core::LayerBuilder::conv2d()
        .inputChanels(in_channels)
        .outputChanels(out_channels)
        .kernelSize(static_cast<size_t>(kernel_shape[0]))
        .stride(static_cast<size_t>(strides[0]))
        .padding(static_cast<size_t>(pads[0]))
        .addWeights(weights, bias)
        .build();

    // IMPORTANT: Only include data tensor as input, NOT weights/bias
    // Weights and bias are already stored in the layer via addWeights()
    std::vector<std::string> inputs;
    inputs.push_back(node.input(0));  // Only the data input

    std::vector<std::string> outputs;
    for (int i = 0; i < node.output_size(); ++i) {
        outputs.push_back(node.output(i));
    }

    // Add node to graph
    std::string node_name = node.has_name() ? node.name() : "";
    builder.addNode(layer, inputs, outputs, node_name);
}

void ONNXParser::parseReluNode(const onnx::NodeProto& node,
                                core::NeuralNetworkBuilder& builder) {
    core::ILayer* layer = core::LayerBuilder::activation()
        .type(hal::ActivationType::RELU)
        .build();

    std::vector<std::string> inputs(node.input().begin(), node.input().end());
    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseGemmNode(const onnx::NodeProto& node,
                                core::NeuralNetworkBuilder& builder) {
    // GEMM: Y = alpha * A' * B' + beta * C
    // Typically used for Dense/Linear layers
    
    if (node.input_size() < 2) {
        throw ONNXParseException("Gemm node requires at least 2 inputs");
    }

    const std::string& b_name = node.input(1);

    // Get attributes (for documentation, not all used in simple dense layer)
    int64_t transB = getIntAttr(node, "transB", 0);

    // Get weights from tensor_map
    auto weights_it = tensor_map_.find(b_name);
    if (weights_it == tensor_map_.end()) {
        throw ONNXParseException("Gemm weights not found: " + b_name);
    }
    const tensor::Tensor& weights = weights_it->second;

    // Determine output units from weights shape
    // If transB=1: weights shape is [K, N], output units = N
    // If transB=0: weights shape is [N, K], output units = N
    size_t units = transB ? weights.shape()[1] : weights.shape()[0];

    // Get optional bias
    tensor::Tensor bias;
    if (node.input_size() > 2 && !node.input(2).empty()) {
        const std::string& bias_name = node.input(2);
        auto bias_it = tensor_map_.find(bias_name);
        if (bias_it != tensor_map_.end()) {
            bias = bias_it->second;
        } else {
            bias = tensor::Tensor::zeros(tensor::Shape({units}), tensor::DataType::FLOAT32);
        }
    } else {
        bias = tensor::Tensor::zeros(tensor::Shape({units}), tensor::DataType::FLOAT32);
    }

    // Build Dense layer - chain directly
    core::ILayer* layer = core::LayerBuilder::dense()
        .units(units)
        .addWeights(weights, bias)
        .build();

    // IMPORTANT: Only include data tensor as input, NOT weights/bias
    // Weights and bias are already stored in the layer via addWeights()
    std::vector<std::string> inputs;
    inputs.push_back(node.input(0));  // Only the data input
    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseMatMulNode(const onnx::NodeProto& node,
                                  core::NeuralNetworkBuilder& builder) {
    // MatMul: Y = A * B
    // Used for matrix multiplication (without bias)
    // Typically followed by an Add node for bias addition
    
    if (node.input_size() < 2) {
        throw ONNXParseException("MatMul node requires 2 inputs");
    }

    const std::string& a_name = node.input(0);
    const std::string& b_name = node.input(1);

    // Check if B (weights) is in tensor_map (i.e., it's a constant weight)
    auto weights_it = tensor_map_.find(b_name);
    if (weights_it != tensor_map_.end()) {
        // B is constant weights - this is a Dense layer without bias
        const tensor::Tensor& weights = weights_it->second;

        // For MatMul, weights shape is typically [in_features, out_features]
        // Output units = weights.shape()[1]
        size_t units = weights.shape()[weights.shape().size() - 1];

        // Create zero bias since MatMul doesn't have bias
        tensor::Tensor bias = tensor::Tensor::zeros(tensor::Shape({units}), tensor::DataType::FLOAT32);

        // Build Dense layer
        core::ILayer* layer = core::LayerBuilder::dense()
            .units(units)
            .addWeights(weights, bias)
            .build();

        // Only include data tensor as input
        std::vector<std::string> inputs;
        inputs.push_back(a_name);

        std::vector<std::string> outputs(node.output().begin(), node.output().end());
        builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
    } else {
        // Both inputs are activations - dynamic MatMul (not supported yet)
        if (config_.strict_mode) {
            throw ONNXParseException("Dynamic MatMul (both inputs are activations) not yet supported");
        }
        // Skip in non-strict mode
    }
}

void ONNXParser::parseMaxPoolNode(const onnx::NodeProto& node,
                                   core::NeuralNetworkBuilder& builder) {
    std::vector<int64_t> kernel_shape = getIntsAttr(node, "kernel_shape");
    std::vector<int64_t> strides = getIntsAttr(node, "strides");
    std::vector<int64_t> pads = getIntsAttr(node, "pads");
    (void)pads;  // Suppress unused warning

    if (kernel_shape.empty()) {
        throw ONNXParseException("MaxPool requires kernel_shape attribute");
    }
    if (strides.empty()) {
        strides = kernel_shape;  // Default stride = kernel size
    }

    core::ILayer* layer = core::LayerBuilder::pool()
        .poolSize(static_cast<size_t>(kernel_shape[0]))
        .stride(static_cast<size_t>(strides[0]))
        .type(hal::PoolingType::MAX)
        .build();

    std::vector<std::string> inputs(node.input().begin(), node.input().end());
    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseAvgPoolNode(const onnx::NodeProto& node,
                                   core::NeuralNetworkBuilder& builder) {
    std::vector<int64_t> kernel_shape = getIntsAttr(node, "kernel_shape");
    std::vector<int64_t> strides = getIntsAttr(node, "strides");

    size_t pool_size = 1;
    size_t stride = 1;

    if (!kernel_shape.empty()) {
        pool_size = static_cast<size_t>(kernel_shape[0]);
        stride = strides.empty() ? pool_size : static_cast<size_t>(strides[0]);
    }

    core::ILayer* layer = core::LayerBuilder::pool()
        .poolSize(pool_size)
        .stride(stride)
        .type(hal::PoolingType::AVERAGE)
        .build();

    std::vector<std::string> inputs(node.input().begin(), node.input().end());
    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseAddNode(const onnx::NodeProto& node,
                               core::NeuralNetworkBuilder& builder) {
    // Add is an element-wise operation
    // Handle the common case where one input is a constant (bias) tensor
    if (node.input_size() < 2) {
        throw ONNXParseException("Add node requires 2 inputs");
    }

    const std::string& input0 = node.input(0);
    const std::string& input1 = node.input(1);

    // Check which input is constant (bias) - it should be in tensor_map_
    bool input0_is_const = tensor_map_.count(input0) > 0;
    bool input1_is_const = tensor_map_.count(input1) > 0;

    std::string activation_input;
    tensor::Tensor bias;

    if (input1_is_const && !input0_is_const) {
        // Common case: input0 is activation, input1 is bias constant
        activation_input = input0;
        bias = tensor_map_.at(input1);
    } else if (input0_is_const && !input1_is_const) {
        // Reversed case: input0 is bias constant, input1 is activation
        activation_input = input1;
        bias = tensor_map_.at(input0);
    } else if (input0_is_const && input1_is_const) {
        // Both are constants - skip or handle as constant folding
        if (config_.strict_mode) {
            throw ONNXParseException("Add with two constant inputs not supported");
        }
        return;  // Skip in non-strict mode
    } else {
        // Neither is constant - element-wise add of two activations
        // This would need a proper element-wise Add layer
        if (config_.strict_mode) {
            throw ONNXParseException("Add of two activation tensors not yet supported");
        }
        return;  // Skip in non-strict mode
    }

    // Build BiasAdd layer
    core::ILayer* layer = core::LayerBuilder::biasAdd()
        .setBias(bias)
        .build();

    // Input is just the activation tensor, bias is stored in the layer
    std::vector<std::string> inputs;
    inputs.push_back(activation_input);

    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseBatchNormNode(const onnx::NodeProto& node,
                                     core::NeuralNetworkBuilder& builder) {
    // BatchNormalization inputs: X, scale, B, input_mean, input_var
    if (node.input_size() < 5) {
        throw ONNXParseException("BatchNormalization requires 5 inputs");
    }

    // Note: Full BatchNorm implementation would use these parameters
    // Currently using a simplified builder

    auto bn_builder = core::LayerBuilder::batchnorm();
    core::ILayer* layer = bn_builder.build();

    // Only include data tensor as input, not the BN parameters
    std::vector<std::string> inputs;
    inputs.push_back(node.input(0));  // Only the data input
    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseFlattenNode(const onnx::NodeProto& node,
                                   core::NeuralNetworkBuilder& builder) {
    // axis attribute is typically 1 for standard flattening
    // Note: Actual implementation might need to use axis

    auto flatten_builder = core::LayerBuilder::flatten();
    core::ILayer* layer = flatten_builder.build();

    std::vector<std::string> inputs(node.input().begin(), node.input().end());
    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseReshapeNode(const onnx::NodeProto& node,
                                   core::NeuralNetworkBuilder& builder) {
    // Reshape: Reshapes a tensor to a new shape
    if (node.input_size() < 1) {
        throw ONNXParseException("Reshape requires at least 1 input");
    }

    const std::string& data_name = node.input(0);

    // Check if the input is a weight/constant tensor (already in tensor_map_)
    auto data_it = tensor_map_.find(data_name);
    if (data_it != tensor_map_.end()) {
        // Input is a constant tensor - this is a weight reshape
        const tensor::Tensor& input_tensor = data_it->second;
        
        // Get the target shape if available
        if (node.input_size() >= 2) {
            const std::string& shape_name = node.input(1);
            auto shape_it = tensor_map_.find(shape_name);
            if (shape_it != tensor_map_.end()) {
                // Get shape tensor data
                // Note: INT64 shape data was converted to FLOAT32 during parsing
                const tensor::Tensor& shape_tensor = shape_it->second;
                const float* shape_data = reinterpret_cast<const float*>(shape_tensor.data());
                size_t num_dims = shape_tensor.shape()[0];
                
                std::vector<size_t> new_shape;
                for (size_t i = 0; i < num_dims; ++i) {
                    int64_t dim_val = static_cast<int64_t>(shape_data[i]);
                    if (dim_val == -1) {
                        // -1 means infer this dimension
                        // Calculate based on total element count
                        size_t total = 1;
                        for (size_t d = 0; d < input_tensor.shape().size(); ++d) {
                            total *= input_tensor.shape()[d];
                        }
                        size_t known = 1;
                        for (size_t j = 0; j < num_dims; ++j) {
                            int64_t other_dim = static_cast<int64_t>(shape_data[j]);
                            if (other_dim > 0) known *= static_cast<size_t>(other_dim);
                        }
                        new_shape.push_back(total / known);
                    } else {
                        new_shape.push_back(static_cast<size_t>(dim_val));
                    }
                }
                
                // Create reshaped tensor and store with output name
                void* data_ptr = const_cast<void*>(input_tensor.data());
                tensor::Tensor reshaped_tensor = tensor::Tensor::from_blob(
                    data_ptr,
                    tensor::Shape(new_shape),
                    input_tensor.dtype()
                );
                
                // Store with the output name so it can be found later
                if (!node.output().empty()) {
                    tensor_map_[node.output(0)] = reshaped_tensor;
                }
                return;  // No layer to add - this is a compile-time reshape
            }
        }
        
        // No shape info, just copy with new name
        if (!node.output().empty()) {
            tensor_map_[node.output(0)] = input_tensor;
        }
        return;  // No layer to add
    }
    
    // Input is an activation tensor - use Flatten layer for runtime reshape
    // This is common for transitioning from Conv to Dense layers
    auto flatten_builder = core::LayerBuilder::flatten();
    core::ILayer* layer = flatten_builder.build();

    std::vector<std::string> inputs;
    inputs.push_back(data_name);
    std::vector<std::string> outputs(node.output().begin(), node.output().end());
    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseSoftmaxNode(const onnx::NodeProto& node,
                                   core::NeuralNetworkBuilder& builder) {
    core::ILayer* layer = core::LayerBuilder::activation()
        .type(hal::ActivationType::SOFTMAX)
        .build();

    std::vector<std::string> inputs(node.input().begin(), node.input().end());
    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseSigmoidNode(const onnx::NodeProto& node,
                                   core::NeuralNetworkBuilder& builder) {
    core::ILayer* layer = core::LayerBuilder::activation()
        .type(hal::ActivationType::SIGMOID)
        .build();

    std::vector<std::string> inputs(node.input().begin(), node.input().end());
    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

void ONNXParser::parseTanhNode(const onnx::NodeProto& node,
                                core::NeuralNetworkBuilder& builder) {
    core::ILayer* layer = core::LayerBuilder::activation()
        .type(hal::ActivationType::TANH)
        .build();

    std::vector<std::string> inputs(node.input().begin(), node.input().end());
    std::vector<std::string> outputs(node.output().begin(), node.output().end());

    builder.addNode(layer, inputs, outputs, node.has_name() ? node.name() : "");
}

// ======================== Attribute Helpers ========================

int64_t ONNXParser::getIntAttr(const onnx::NodeProto& node,
                                const std::string& name,
                                int64_t default_value) {
    for (int i = 0; i < node.attribute_size(); ++i) {
        const auto& attr = node.attribute(i);
        if (attr.name() == name && attr.type() == onnx::AttributeProto::INT) {
            return attr.i();
        }
    }
    return default_value;
}

float ONNXParser::getFloatAttr(const onnx::NodeProto& node,
                                const std::string& name,
                                float default_value) {
    for (int i = 0; i < node.attribute_size(); ++i) {
        const auto& attr = node.attribute(i);
        if (attr.name() == name && attr.type() == onnx::AttributeProto::FLOAT) {
            return attr.f();
        }
    }
    return default_value;
}

std::string ONNXParser::getStringAttr(const onnx::NodeProto& node,
                                       const std::string& name,
                                       const std::string& default_value) {
    for (int i = 0; i < node.attribute_size(); ++i) {
        const auto& attr = node.attribute(i);
        if (attr.name() == name && attr.type() == onnx::AttributeProto::STRING) {
            return attr.s();
        }
    }
    return default_value;
}

std::vector<int64_t> ONNXParser::getIntsAttr(const onnx::NodeProto& node,
                                              const std::string& name) {
    for (int i = 0; i < node.attribute_size(); ++i) {
        const auto& attr = node.attribute(i);
        if (attr.name() == name && attr.type() == onnx::AttributeProto::INTS) {
            return std::vector<int64_t>(attr.ints().begin(), attr.ints().end());
        }
    }
    return {};
}

std::vector<float> ONNXParser::getFloatsAttr(const onnx::NodeProto& node,
                                              const std::string& name) {
    for (int i = 0; i < node.attribute_size(); ++i) {
        const auto& attr = node.attribute(i);
        if (attr.name() == name && attr.type() == onnx::AttributeProto::FLOATS) {
            return std::vector<float>(attr.floats().begin(), attr.floats().end());
        }
    }
    return {};
}

// ======================== Type Conversion Helpers ========================

tensor::DataType ONNXParser::convertDataType(int32_t onnx_dtype) {
    // ONNX TensorProto::DataType values
    // 1 = FLOAT, 2 = UINT8, 3 = INT8, 4 = UINT16, 5 = INT16,
    // 6 = INT32, 7 = INT64, 10 = FLOAT16, 11 = DOUBLE, etc.
    
    switch (onnx_dtype) {
        case 1:  // FLOAT
            return tensor::DataType::FLOAT32;
        case 3:  // INT8
            return tensor::DataType::INT8;
        case 5:  // INT16
            return tensor::DataType::INT16;
        case 6:  // INT32 - map to FLOAT32 (convert during loading)
        case 7:  // INT64 - map to FLOAT32 (convert during loading)
            // Note: NEURAX doesn't support INT32/INT64 natively yet
            // For shape tensors, we'll store as FLOAT32 and convert as needed
            return tensor::DataType::FLOAT32;
        default:
            throw ONNXParseException(
                "Unsupported ONNX data type: " + std::to_string(onnx_dtype) +
                ". Only FLOAT32, INT8, INT16 are currently supported.");
    }
}

tensor::Tensor ONNXParser::convertTensor(const onnx::TensorProto& tensor_proto) {
    // Extract dimensions
    std::vector<size_t> dims;
    for (int i = 0; i < tensor_proto.dims_size(); ++i) {
        dims.push_back(static_cast<size_t>(tensor_proto.dims(i)));
    }
    tensor::Shape shape(dims);

    // Get original data type
    int32_t onnx_dtype = tensor_proto.data_type();
    
    // Convert data type (may map INT64 to FLOAT32)
    tensor::DataType dtype = convertDataType(onnx_dtype);

    // Create tensor
    tensor::Tensor tensor(shape, dtype);

    // Copy data based on storage format and original data type
    if (tensor_proto.has_raw_data()) {
        // Raw data - ARM endian-safe (data is stored in little-endian)
        const std::string& raw = tensor_proto.raw_data();
        
        // Handle data type conversion for raw data
        if (onnx_dtype == 7) {  // INT64 -> FLOAT32
            // Convert INT64 raw data to FLOAT32
            size_t num_elements = tensor.numel();
            if (raw.size() != num_elements * sizeof(int64_t)) {
                throw ONNXParseException(
                    "Raw data size mismatch for INT64 tensor '" + tensor_proto.name() + "'");
            }
            const int64_t* src = reinterpret_cast<const int64_t*>(raw.data());
            float* dst = tensor.data_ptr<float>();
            for (size_t i = 0; i < num_elements; ++i) {
                dst[i] = static_cast<float>(src[i]);
            }
        } else if (onnx_dtype == 6) {  // INT32 -> FLOAT32
            size_t num_elements = tensor.numel();
            if (raw.size() != num_elements * sizeof(int32_t)) {
                throw ONNXParseException(
                    "Raw data size mismatch for INT32 tensor '" + tensor_proto.name() + "'");
            }
            const int32_t* src = reinterpret_cast<const int32_t*>(raw.data());
            float* dst = tensor.data_ptr<float>();
            for (size_t i = 0; i < num_elements; ++i) {
                dst[i] = static_cast<float>(src[i]);
            }
        } else {
            // Direct copy for matching types
            if (raw.size() != tensor.nbytes()) {
                throw ONNXParseException(
                    "Raw data size mismatch for tensor '" + tensor_proto.name() +
                    "': expected " + std::to_string(tensor.nbytes()) +
                    ", got " + std::to_string(raw.size()));
            }
            std::memcpy(tensor.data(), raw.data(), raw.size());
        }
    } else if (tensor_proto.float_data_size() > 0) {
        // Float data stored separately
        float* dst = tensor.data_ptr<float>();
        for (int i = 0; i < tensor_proto.float_data_size(); ++i) {
            dst[i] = tensor_proto.float_data(i);
        }
    } else if (tensor_proto.int64_data_size() > 0) {
        // INT64 data stored separately - convert to FLOAT32
        float* dst = tensor.data_ptr<float>();
        for (int i = 0; i < tensor_proto.int64_data_size(); ++i) {
            dst[i] = static_cast<float>(tensor_proto.int64_data(i));
        }
    } else if (tensor_proto.int32_data_size() > 0) {
        // Int32 data - convert to appropriate type
        if (dtype == tensor::DataType::INT8) {
            int8_t* dst = tensor.data_ptr<int8_t>();
            for (int i = 0; i < tensor_proto.int32_data_size(); ++i) {
                dst[i] = static_cast<int8_t>(tensor_proto.int32_data(i));
            }
        } else if (dtype == tensor::DataType::INT16) {
            int16_t* dst = tensor.data_ptr<int16_t>();
            for (int i = 0; i < tensor_proto.int32_data_size(); ++i) {
                dst[i] = static_cast<int16_t>(tensor_proto.int32_data(i));
            }
        } else {
            // Convert to FLOAT32
            float* dst = tensor.data_ptr<float>();
            for (int i = 0; i < tensor_proto.int32_data_size(); ++i) {
                dst[i] = static_cast<float>(tensor_proto.int32_data(i));
            }
        }
    }

    return tensor;
}

tensor::Shape ONNXParser::extractShape(const onnx::ValueInfoProto& value_info) {
    std::vector<size_t> dims;

    if (value_info.has_type() && value_info.type().has_tensor_type()) {
        const auto& tensor_type = value_info.type().tensor_type();
        if (tensor_type.has_shape()) {
            for (int i = 0; i < tensor_type.shape().dim_size(); ++i) {
                const auto& dim = tensor_type.shape().dim(i);
                if (dim.has_dim_value()) {
                    dims.push_back(static_cast<size_t>(dim.dim_value()));
                } else {
                    // Dynamic dimension - use 1 as placeholder for static shapes MVP
                    dims.push_back(1);
                }
            }
        }
    }

    return tensor::Shape(dims);
}

tensor::DataType ONNXParser::extractDataType(const onnx::ValueInfoProto& value_info) {
    if (value_info.has_type() && value_info.type().has_tensor_type()) {
        return convertDataType(value_info.type().tensor_type().elem_type());
    }
    return tensor::DataType::FLOAT32;  // Default
}

} // namespace parser
} // namespace neurax
