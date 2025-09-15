/**
 * @file Shape.hpp
 * @brief Shape class for tensor dimensions
 * 
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_TENSOR_SHAPE_HPP
#define NEURAX_TENSOR_SHAPE_HPP

#include <vector>
#include <initializer_list>
#include <string>

namespace neurax {
namespace tensor {

/**
 * @brief Represents the shape (dimensions) of a tensor
 */
class Shape {
public:
    /**
     * @brief Default constructor - creates empty shape
     */
    Shape() = default;
    
    /**
     * @brief Constructor with initializer list
     * @param dims Dimensions as initializer list
     */
    Shape(std::initializer_list<size_t> dims) : dims_(dims) {}
    
    /**
     * @brief Constructor with vector
     * @param dims Dimensions as vector
     */
    Shape(const std::vector<size_t>& dims) : dims_(dims) {}
    
    /**
     * @brief Get dimension at index
     * @param i Index
     * @return Dimension size
     */
    size_t operator[](size_t i) const { return dims_[i]; }
    
    /**
     * @brief Get dimension at index (non-const)
     * @param i Index
     * @return Reference to dimension size
     */
    size_t& operator[](size_t i) { return dims_[i]; }
    
    /**
     * @brief Get number of dimensions
     * @return Number of dimensions
     */
    size_t size() const { return dims_.size(); }
    
    /**
     * @brief Get total number of elements
     * @return Total number of elements (product of all dimensions)
     */
    size_t numel() const;
    
    /**
     * @brief Get total number of elements (alias for numel)
     * @return Total number of elements (product of all dimensions)
     */
    size_t getSize() const { return numel(); }
    
    /**
     * @brief Get dimensions vector (alias for dims)
     * @return Const reference to dimensions vector
     */
    const std::vector<size_t>& getDimensions() const { return dims_; }
    
    /**
     * @brief Get dimensions vector
     * @return Const reference to dimensions vector
     */
    const std::vector<size_t>& dims() const { return dims_; }
    
    /**
     * @brief Check if shapes are equal
     * @param other Other shape
     * @return true if equal, false otherwise
     */
    bool operator==(const Shape& other) const { return dims_ == other.dims_; }
    
    /**
     * @brief Check if shapes are not equal
     * @param other Other shape
     * @return true if not equal, false otherwise
     */
    bool operator!=(const Shape& other) const { return !(*this == other); }
    
    /**
     * @brief Get string representation
     * @return String representation like "[N, H, W, C]"
     */
    std::string str() const;
    
    /**
     * @brief Get string representation (alias for str)
     * @return String representation like "[N, H, W, C]"
     */
    std::string toString() const { return str(); }
    
    /**
     * @brief Check if shape is empty
     * @return true if no dimensions, false otherwise
     */
    bool empty() const { return dims_.empty(); }

private:
    std::vector<size_t> dims_;
};

} // namespace tensor
} // namespace neurax

#endif // NEURAX_TENSOR_SHAPE_HPP
