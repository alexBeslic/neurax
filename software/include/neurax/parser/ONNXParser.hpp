/**
 * @file ONNXParser.hpp
 * @brief ONNX model parser for NEURAX inference engine
 *
 * Parses ONNX models and converts them to NEURAX neural network representation
 * using the graph-based builder API. Supports portable deployment on DE1-SoC.
 *
 * @author NEURAX Development Team
 * @date January 2026
 * @version 1.0
 */

#ifndef NEURAX_PARSER_ONNXPARSER_HPP
#define NEURAX_PARSER_ONNXPARSER_HPP

#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "neurax/core/network/INetwork.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/tensor/DataType.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <stdexcept>

// Forward declarations for ONNX protobuf types
namespace onnx {
    class ModelProto;
    class GraphProto;
    class NodeProto;
    class TensorProto;
    class ValueInfoProto;
    class AttributeProto;
}

namespace neurax {
namespace parser {

/**
 * @brief Exception class for ONNX parsing errors
 */
class ONNXParseException : public std::runtime_error {
public:
    explicit ONNXParseException(const std::string& message)
        : std::runtime_error("ONNX Parse error: " + message) {}
};

/**
 * @brief Supported ONNX operator types
 */
enum class ONNXOpType {
    CONV,
    RELU,
    GEMM,
    MATMUL,
    MAXPOOL,
    AVGPOOL,
    ADD,
    BATCHNORM,
    FLATTEN,
    SOFTMAX,
    SIGMOID,
    TANH,
    RESHAPE,
    UNSUPPORTED
};

/**
 * @brief Parser configuration options
 */
struct ONNXParserConfig {
    hal::AcceleratorType accelerator_type = hal::AcceleratorType::SOFTWARE_FALLBACK;
    int64_t min_opset_version = 11;
    int64_t max_opset_version = 21;
    size_t max_model_size_bytes = 256 * 1024 * 1024;  // 256 MB default limit
    bool strict_mode = false;  // Fail on unsupported ops if true
};

/**
 * @brief ONNX model parser for NEURAX inference engine
 *
 * Parses ONNX models (.onnx files) and builds a NEURAX neural network
 * using the graph-based NeuralNetworkBuilder API. Designed for portable
 * deployment on ARM/FPGA targets like DE1-SoC.
 *
 * Features:
 * - Memory-efficient streaming parser with configurable limits
 * - Zero-copy tensor loading where possible
 * - Static shapes only (FP32 MVP)
 * - ARM endian-safe parsing
 *
 * Supported operators:
 * - Conv, Relu, Gemm (Dense), MaxPool, AvgPool
 * - Add, BatchNormalization, Flatten, Softmax, Sigmoid, Tanh
 *
 * @example
 * @code
 * ONNXParser parser;
 * auto network = parser.LoadModel("model.onnx");
 * Tensor input = Tensor::zeros(Shape({1, 3, 224, 224}));
 * Tensor output = network->execute(input);
 * @endcode
 */
class ONNXParser {
public:
    /**
     * @brief Default constructor with default configuration
     */
    ONNXParser();

    /**
     * @brief Constructor with custom configuration
     * @param config Parser configuration options
     */
    explicit ONNXParser(const ONNXParserConfig& config);

    /**
     * @brief Destructor
     */
    ~ONNXParser();

    // Disable copy
    ONNXParser(const ONNXParser&) = delete;
    ONNXParser& operator=(const ONNXParser&) = delete;

    // Enable move
    ONNXParser(ONNXParser&&) noexcept;
    ONNXParser& operator=(ONNXParser&&) noexcept;

    /**
     * @brief Load and parse an ONNX model file
     *
     * Parses the ONNX model and constructs a NEURAX neural network
     * using the graph-based builder API.
     *
     * @param path Path to the .onnx model file
     * @return Unique pointer to the constructed network
     * @throws ONNXParseException on parse errors
     * @throws tensor::TensorException on tensor creation errors
     */
    std::unique_ptr<core::INetwork> LoadModel(const std::string& path);

    /**
     * @brief Load model from memory buffer
     *
     * @param data Pointer to model data in memory
     * @param size Size of the data in bytes
     * @return Unique pointer to the constructed network
     * @throws ONNXParseException on parse errors
     */
    std::unique_ptr<core::INetwork> LoadModelFromBuffer(const void* data, size_t size);

    /**
     * @brief Get the current parser configuration
     * @return Current configuration
     */
    const ONNXParserConfig& getConfig() const { return config_; }

    /**
     * @brief Set parser configuration
     * @param config New configuration
     */
    void setConfig(const ONNXParserConfig& config) { config_ = config; }

    /**
     * @brief Get model input names after parsing
     * @return Vector of input tensor names
     */
    const std::vector<std::string>& getInputNames() const { return input_names_; }

    /**
     * @brief Get model output names after parsing
     * @return Vector of output tensor names
     */
    const std::vector<std::string>& getOutputNames() const { return output_names_; }

    /**
     * @brief Get the shape of a named tensor
     * @param name Tensor name
     * @return Shape of the tensor
     * @throws std::out_of_range if tensor not found
     */
    const tensor::Shape& getShape(const std::string& name) const {
        return shape_map_.at(name);
    }

    /**
     * @brief Check if a tensor shape is available
     * @param name Tensor name
     * @return true if shape is available
     */
    bool hasShape(const std::string& name) const {
        return shape_map_.count(name) > 0;
    }

    /**
     * @brief Get model metadata (producer, version, etc.)
     * @return Map of metadata key-value pairs
     */
    const std::map<std::string, std::string>& getModelMetadata() const { return model_metadata_; }

private:
    // ======================== Private Members ========================

    /// Parser configuration
    ONNXParserConfig config_;

    /// Tensor storage for weights and intermediate values
    std::map<std::string, tensor::Tensor> tensor_map_;

    /// Shape info for all tensors
    std::map<std::string, tensor::Shape> shape_map_;

    /// Data type info for all tensors
    std::map<std::string, tensor::DataType> dtype_map_;

    /// Input tensor names
    std::vector<std::string> input_names_;

    /// Output tensor names
    std::vector<std::string> output_names_;

    /// Model metadata
    std::map<std::string, std::string> model_metadata_;

    // ======================== Helper Methods ========================

    /**
     * @brief Validate model opset version
     * @param model ONNX model proto
     * @throws ONNXParseException if opset is unsupported
     */
    void validateOpset(const onnx::ModelProto& model);

    /**
     * @brief Parse graph inputs and register them
     * @param graph ONNX graph proto
     * @param builder Network builder
     */
    void parseGraphInputs(const onnx::GraphProto& graph, 
                          core::NeuralNetworkBuilder& builder);

    /**
     * @brief Parse graph outputs
     * @param graph ONNX graph proto
     * @param builder Network builder
     */
    void parseGraphOutputs(const onnx::GraphProto& graph,
                           core::NeuralNetworkBuilder& builder);

    /**
     * @brief Parse initializers (weights/biases) and load into tensor_map
     * @param graph ONNX graph proto
     * @param builder Network builder
     */
    void parseInitializers(const onnx::GraphProto& graph,
                           core::NeuralNetworkBuilder& builder);

    /**
     * @brief Parse a single node and add to the network
     * @param node ONNX node proto
     * @param builder Network builder
     */
    void parseNode(const onnx::NodeProto& node,
                   core::NeuralNetworkBuilder& builder);

    /**
     * @brief Map ONNX op_type string to internal enum
     * @param op_type ONNX operator type string
     * @return Corresponding ONNXOpType enum value
     */
    ONNXOpType mapOpType(const std::string& op_type);

    // ======================== Node Parsing Methods ========================

    void parseConvNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseReluNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseGemmNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseMatMulNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseMaxPoolNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseAvgPoolNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseAddNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseBatchNormNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseFlattenNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseReshapeNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseSoftmaxNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseSigmoidNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);
    void parseTanhNode(const onnx::NodeProto& node, core::NeuralNetworkBuilder& builder);

    // ======================== Attribute Helpers ========================

    /**
     * @brief Get integer attribute from node
     * @param node ONNX node proto
     * @param name Attribute name
     * @param default_value Default if not found
     * @return Attribute value or default
     */
    int64_t getIntAttr(const onnx::NodeProto& node, 
                       const std::string& name,
                       int64_t default_value = 0);

    /**
     * @brief Get float attribute from node
     * @param node ONNX node proto
     * @param name Attribute name
     * @param default_value Default if not found
     * @return Attribute value or default
     */
    float getFloatAttr(const onnx::NodeProto& node,
                       const std::string& name,
                       float default_value = 0.0f);

    /**
     * @brief Get string attribute from node
     * @param node ONNX node proto
     * @param name Attribute name
     * @param default_value Default if not found
     * @return Attribute value or default
     */
    std::string getStringAttr(const onnx::NodeProto& node,
                              const std::string& name,
                              const std::string& default_value = "");

    /**
     * @brief Get integer array attribute from node
     * @param node ONNX node proto
     * @param name Attribute name
     * @return Vector of integer values (empty if not found)
     */
    std::vector<int64_t> getIntsAttr(const onnx::NodeProto& node,
                                     const std::string& name);

    /**
     * @brief Get float array attribute from node
     * @param node ONNX node proto
     * @param name Attribute name
     * @return Vector of float values (empty if not found)
     */
    std::vector<float> getFloatsAttr(const onnx::NodeProto& node,
                                     const std::string& name);

    // ======================== Type Conversion Helpers ========================

    /**
     * @brief Convert ONNX data type to NEURAX data type
     * @param onnx_dtype ONNX TensorProto::DataType value
     * @return Corresponding NEURAX DataType
     * @throws ONNXParseException for unsupported types
     */
    tensor::DataType convertDataType(int32_t onnx_dtype);

    /**
     * @brief Convert ONNX tensor to NEURAX tensor
     * @param tensor_proto ONNX tensor proto
     * @return NEURAX tensor with copied data
     */
    tensor::Tensor convertTensor(const onnx::TensorProto& tensor_proto);

    /**
     * @brief Extract shape from ValueInfoProto
     * @param value_info ONNX value info proto
     * @return Tensor shape
     */
    tensor::Shape extractShape(const onnx::ValueInfoProto& value_info);

    /**
     * @brief Extract data type from ValueInfoProto
     * @param value_info ONNX value info proto
     * @return Data type
     */
    tensor::DataType extractDataType(const onnx::ValueInfoProto& value_info);
};

} // namespace parser
} // namespace neurax

#endif // NEURAX_PARSER_ONNXPARSER_HPP
