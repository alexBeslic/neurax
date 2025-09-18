/**
 * @file DataType.hpp
 * @brief Data type definitions for NEURAX tensor operations
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_TENSOR_DATATYPE_HPP
#define NEURAX_TENSOR_DATATYPE_HPP

#include <cstddef>

namespace neurax {
namespace tensor {

/**
 * @brief Supported data types for tensor elements
 */
enum class DataType {
    INT8 = 0,      ///< 8-bit signed integer
    INT16 = 1,     ///< 16-bit signed integer
    FLOAT32 = 2    ///< 32-bit floating point
};

/**
 * @brief Get the size in bytes of a data type
 * @param dtype The data type
 * @return Size in bytes
 */
size_t sizeof_dtype(DataType dtype);

/**
 * @brief Get the string name of a data type
 * @param dtype The data type
 * @return String representation
 */
const char* dtype_name(DataType dtype);

/**
 * @brief Check if data type is floating point
 * @param dtype The data type
 * @return true if floating point, false otherwise
 */
bool is_floating_point(DataType dtype);

/**
 * @brief Check if data type is integer
 * @param dtype The data type
 * @return true if integer, false otherwise
 */
bool is_integer(DataType dtype);

} // namespace tensor
} // namespace neurax

#endif // NEURAX_TENSOR_DATATYPE_HPP
