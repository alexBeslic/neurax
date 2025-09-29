/**
 * @file Tensor.hpp
 * @brief Main Tensor class for NEURAX neural network operations
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_TENSOR_TENSOR_HPP
#define NEURAX_TENSOR_TENSOR_HPP

#include "DataType.hpp"
#include "Shape.hpp"
#include <memory>
#include <stdexcept>

namespace neurax {
namespace tensor {

/**
 * @brief Exception class for tensor operations
 */
class TensorException : public std::runtime_error {
public:
    explicit TensorException(const std::string& message)
        : std::runtime_error("Tensor error: " + message) {}
};

/**
 * @brief Main tensor class for neural network data
 *
 * This is a simplified but practical tensor implementation that provides
 * essential functionality for neural network operations without the complexity
 * of a full PyTorch-like implementation.
 */
class Tensor {
public:
    /**
     * @brief Default constructor - creates empty tensor
     */
    Tensor() = default;

    /**
     * @brief Constructor with shape and data type
     * @param shape Tensor shape
     * @param dtype Data type (default: FLOAT32)
     */
    Tensor(const Shape& shape, DataType dtype = DataType::FLOAT32);

    /**
     * @brief Constructor with shape, data, and data type
     * @param shape Tensor shape
     * @param data Pointer to external data (will be copied)
     * @param dtype Data type
     */
    Tensor(const Shape& shape, void* data, DataType dtype);

    /**
     * @brief Copy constructor
     */
    Tensor(const Tensor& other);

    /**
     * @brief Move constructor
     */
    Tensor(Tensor&& other) noexcept;

    /**
     * @brief Copy assignment operator
     */
    Tensor& operator=(const Tensor& other);

    /**
     * @brief Move assignment operator
     */
    Tensor& operator=(Tensor&& other) noexcept;

    /**
     * @brief Get tensor shape
     * @return Const reference to shape
     */
    const Shape& shape() const { return shape_; }

    /**
     * @brief Get tensor shape (alias for shape)
     * @return Const reference to shape
     */
    const Shape& getShape() const { return shape_; }

    /**
     * @brief Get data type
     * @return Data type
     */
    DataType dtype() const { return dtype_; }

    /**
     * @brief Get total number of elements
     * @return Number of elements
     */
    size_t numel() const { return shape_.numel(); }

    /**
     * @brief Get size in bytes
     * @return Total size in bytes
     */
    size_t nbytes() const;

        /**
     * @brief Get mutable data pointer
     * @return Pointer to tensor data
     */
    void* data() { return data_.get(); }

    /**
     * @brief Get mutable data pointer (alias for data)
     * @return Pointer to tensor data
     */
    void* getData() { return data_.get(); }

    /**
     * @brief Get const data pointer
     * @return Const pointer to tensor data
     */
    const void* data() const { return data_.get(); }

    /**
     * @brief Get const data pointer (alias for data)
     * @return Const pointer to tensor data
     */
    const void* getData() const { return data_.get(); }

    /**
     * @brief Get typed data pointer
     * @tparam T Type to cast to
     * @return Typed pointer to data
     */
    template<typename T>
    T* data_ptr() { return static_cast<T*>(data()); }

    /**
     * @brief Get typed data pointer (const)
     * @tparam T Type to cast to
     * @return Const typed pointer to data
     */
    template<typename T>
    const T* data_ptr() const { return static_cast<const T*>(data()); }

    /**
     * @brief Check if tensor is empty
     * @return true if empty, false otherwise
     */
    bool empty() const { return !data_ || numel() == 0; }

    /**
     * @brief Fill tensor with zeros
     */
    void zero_();

    /**
     * @brief Copy data from external source
     * @param src Source pointer
     * @param size_bytes Number of bytes to copy
     */
    void copy_from(const void* src, size_t size_bytes);

    /**
     * @brief Copy data to external destination
     * @param dst Destination pointer
     * @param size_bytes Number of bytes to copy
     */
    void copy_to(void* dst, size_t size_bytes) const;

    // Factory functions

    /**
     * @brief Create tensor filled with zeros
     * @param shape Tensor shape
     * @param dtype Data type (default: FLOAT32)
     * @return New tensor filled with zeros
     */
    static Tensor zeros(const Shape& shape, DataType dtype = DataType::FLOAT32);

    /**
     * @brief Create tensor filled with ones
     * @param shape Tensor shape
     * @param dtype Data type (default: FLOAT32)
     * @return New tensor filled with ones
     */
    static Tensor ones(const Shape& shape, DataType dtype = DataType::FLOAT32);

    /**
     * @brief Create tensor from external data blob
     * @param data External data pointer (will be copied)
     * @param shape Tensor shape
     * @param dtype Data type
     * @return New tensor with copied data
     */
    static Tensor from_blob(void* data, const Shape& shape, DataType dtype);

    /**
     * @brief Create empty tensor with given shape
     * @param shape Tensor shape
     * @param dtype Data type (default: FLOAT32)
     * @return New uninitialized tensor
     */
    static Tensor empty(const Shape& shape, DataType dtype = DataType::FLOAT32);

    /**
     * @brief Reshape tensor to new shape without copying data when possible.
     * @param new_shape Desired shape. Must have the same number of elements as the current tensor.
     * @return A new Tensor that shares the underlying buffer when shapes are compatible.
     * @throws TensorException if numel differs.
     */
    Tensor reshape(const Shape& new_shape) const;

private:
    Shape shape_;
    DataType dtype_;
    std::shared_ptr<uint8_t> data_;

    /**
     * @brief Allocate memory for tensor data
     */
    void allocate();
};

} // namespace tensor
} // namespace neurax

#endif // NEURAX_TENSOR_TENSOR_HPP
