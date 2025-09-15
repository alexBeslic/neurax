/**
 * @file DataType.cpp
 * @brief Implementation of data type utilities
 * 
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#include "neurax/tensor/DataType.hpp"
#include <cstdint>

namespace neurax {
namespace tensor {

size_t sizeof_dtype(DataType dtype) {
    switch (dtype) {
        case DataType::INT8:    return sizeof(int8_t);   // 1 byte
        case DataType::INT16:   return sizeof(int16_t);  // 2 bytes
        case DataType::FLOAT32: return sizeof(float);    // 4 bytes
        default:
            return 0;
    }
}

const char* dtype_name(DataType dtype) {
    switch (dtype) {
        case DataType::INT8:    return "int8";
        case DataType::INT16:   return "int16";
        case DataType::FLOAT32: return "float32";
        default:
            return "unknown";
    }
}

bool is_floating_point(DataType dtype) {
    switch (dtype) {
        case DataType::FLOAT32:
            return true;
        case DataType::INT8:
        case DataType::INT16:
        default:
            return false;
    }
}

bool is_integer(DataType dtype) {
    switch (dtype) {
        case DataType::INT8:
        case DataType::INT16:
            return true;
        case DataType::FLOAT32:
        default:
            return false;
    }
}

} // namespace tensor
} // namespace neurax
