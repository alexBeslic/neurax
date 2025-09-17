/**
 * @file Tensor.cpp
 * @brief Implementation of Tensor class
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "neurax/tensor/Tensor.hpp"
#include <cstring>
#include <algorithm>

namespace neurax {
namespace tensor {

Tensor::Tensor(const Shape& shape, DataType dtype)
    : shape_(shape), dtype_(dtype) {
    allocate();
    if (data_) {
        // Initialize to zero
        std::memset(data_.get(), 0, nbytes());
    }
}

Tensor::Tensor(const Shape& shape, void* data, DataType dtype)
    : shape_(shape), dtype_(dtype) {
    if (!data) {
        throw TensorException("Cannot create tensor from null data pointer");
    }

    allocate();
    if (data_) {
        std::memcpy(data_.get(), data, nbytes());
    }
}

Tensor::Tensor(const Tensor& other)
    : shape_(other.shape_), dtype_(other.dtype_) {
    if (other.data_ && other.numel() > 0) {
        allocate();
        std::memcpy(data_.get(), other.data_.get(), nbytes());
    }
}

Tensor::Tensor(Tensor&& other) noexcept
    : shape_(std::move(other.shape_)),
      dtype_(other.dtype_),
      data_(std::move(other.data_)) {
    // Other tensor is now empty
}

Tensor& Tensor::operator=(const Tensor& other) {
    if (this != &other) {
        shape_ = other.shape_;
        dtype_ = other.dtype_;

        if (other.data_ && other.numel() > 0) {
            allocate();
            std::memcpy(data_.get(), other.data_.get(), nbytes());
        } else {
            data_.reset();
        }
    }
    return *this;
}

Tensor& Tensor::operator=(Tensor&& other) noexcept {
    if (this != &other) {
        shape_ = std::move(other.shape_);
        dtype_ = other.dtype_;
        data_ = std::move(other.data_);
    }
    return *this;
}

size_t Tensor::nbytes() const {
    return numel() * sizeof_dtype(dtype_);
}

void Tensor::zero_() {
    if (data_) {
        std::memset(data_.get(), 0, nbytes());
    }
}

void Tensor::copy_from(const void* src, size_t size_bytes) {
    if (!src) {
        throw TensorException("Cannot copy from null pointer");
    }

    if (!data_) {
        throw TensorException("Cannot copy to uninitialized tensor");
    }

    if (size_bytes > nbytes()) {
        throw TensorException("Source data size exceeds tensor capacity");
    }

    std::memcpy(data_.get(), src, size_bytes);
}

void Tensor::copy_to(void* dst, size_t size_bytes) const {
    if (!dst) {
        throw TensorException("Cannot copy to null pointer");
    }

    if (!data_) {
        throw TensorException("Cannot copy from uninitialized tensor");
    }

    if (size_bytes > nbytes()) {
        throw TensorException("Destination buffer too small");
    }

    std::memcpy(dst, data_.get(), size_bytes);
}

void Tensor::allocate() {
    size_t total_bytes = nbytes();
    if (total_bytes > 0) {
        data_ = std::shared_ptr<uint8_t>(new uint8_t[total_bytes], std::default_delete<uint8_t[]>());
    } else {
        data_.reset();
    }
}

// Factory functions

Tensor Tensor::zeros(const Shape& shape, DataType dtype) {
    return Tensor(shape, dtype); // Constructor already zeros the memory
}

Tensor Tensor::ones(const Shape& shape, DataType dtype) {
    Tensor tensor(shape, dtype);

    if (!tensor.data_) {
        return tensor;
    }

    // Fill with ones based on data type
    switch (dtype) {
        case DataType::INT8: {
            int8_t* ptr = tensor.data_ptr<int8_t>();
            std::fill(ptr, ptr + tensor.numel(), int8_t(1));
            break;
        }
        case DataType::INT16: {
            int16_t* ptr = tensor.data_ptr<int16_t>();
            std::fill(ptr, ptr + tensor.numel(), int16_t(1));
            break;
        }
        case DataType::FLOAT32: {
            float* ptr = tensor.data_ptr<float>();
            std::fill(ptr, ptr + tensor.numel(), 1.0f);
            break;
        }
    }

    return tensor;
}

Tensor Tensor::from_blob(void* data, const Shape& shape, DataType dtype) {
    return Tensor(shape, data, dtype);
}

Tensor Tensor::empty(const Shape& shape, DataType dtype) {
    Tensor tensor;
    tensor.shape_ = shape;
    tensor.dtype_ = dtype;
    tensor.allocate();
    // Note: memory is uninitialized (not zeroed)
    return tensor;
}

} // namespace tensor
} // namespace neurax
