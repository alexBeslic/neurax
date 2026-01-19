/**
 * @file GraphTensor.hpp
 * @brief Metadata wrapper for tensors in the execution graph
 *
 * @author NEURAX Development Team
 * @date January 2026
 * @version 2.0
 */

#ifndef NEURAX_CORE_GRAPH_TENSOR_HPP
#define NEURAX_CORE_GRAPH_TENSOR_HPP

#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/DataType.hpp"
#include <string>

namespace neurax {
namespace core {
namespace graph {

/**
 * @brief Metadata wrapper for tensors in the execution graph
 * 
 * Holds compile-time shape and type information only.
 * Runtime data lives in neurax::tensor::Tensor.
 */
struct GraphTensor {
    std::string name;           ///< Unique tensor identifier
    tensor::Shape shape;        ///< Static shape (compile-time resolved)
    tensor::DataType dtype;     ///< Data type
    bool constant;              ///< true for weights/biases, false for activations

    /**
     * @brief Default constructor
     */
    GraphTensor() 
        : dtype(tensor::DataType::FLOAT32)
        , constant(false) 
    {}
    
    /**
     * @brief Parameterized constructor
     * @param n Tensor name
     * @param s Tensor shape
     * @param dt Data type (default: FLOAT32)
     * @param is_const Whether tensor is constant (default: false)
     */
    GraphTensor(const std::string& n, 
                const tensor::Shape& s, 
                tensor::DataType dt = tensor::DataType::FLOAT32, 
                bool is_const = false)
        : name(n)
        , shape(s)
        , dtype(dt)
        , constant(is_const) 
    {}

    /**
     * @brief Check if tensor metadata is valid
     * @return true if name is not empty and shape has dimensions
     */
    bool isValid() const {
        return !name.empty() && shape.size() > 0;
    }
};

} // namespace graph
} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_GRAPH_TENSOR_HPP
