/**
 * @file Shape.cpp
 * @brief Implementation of Shape class
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "neurax/tensor/Shape.hpp"
#include <sstream>

namespace neurax {
namespace tensor {

size_t Shape::numel() const {
    if (dims_.empty()) {
        return 0;
    }

    size_t total = 1;
    for (size_t dim : dims_) {
        total *= dim;
    }
    return total;
}

std::string Shape::str() const {
    if (dims_.empty()) {
        return "[]";
    }

    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < dims_.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << dims_[i];
    }
    oss << "]";
    return oss.str();
}

} // namespace tensor
} // namespace neurax
