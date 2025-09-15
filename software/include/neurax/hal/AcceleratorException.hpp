/**
 * @file AcceleratorException.hpp
 * @brief Exception classes for the Hardware Abstraction Layer
 * 
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_HAL_ACCELERATOR_EXCEPTION_HPP
#define NEURAX_HAL_ACCELERATOR_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace neurax {
namespace hal {

/**
 * @brief Base exception for all HAL-related errors
 */
class AcceleratorException : public std::runtime_error {
public:
    explicit AcceleratorException(const std::string& message)
        : std::runtime_error("HAL Error: " + message) {}
};

/**
 * @brief Exception thrown when hardware initialization fails
 */
class HardwareInitializationException : public AcceleratorException {
public:
    explicit HardwareInitializationException(const std::string& message)
        : AcceleratorException("Hardware initialization failed: " + message) {}
};

/**
 * @brief Exception thrown when hardware operation fails
 */
class HardwareOperationException : public AcceleratorException {
public:
    explicit HardwareOperationException(const std::string& message)
        : AcceleratorException("Hardware operation failed: " + message) {}
};

/**
 * @brief Exception thrown when accelerator is not available
 */
class AcceleratorUnavailableException : public AcceleratorException {
public:
    explicit AcceleratorUnavailableException(const std::string& message)
        : AcceleratorException("Accelerator unavailable: " + message) {}
};

/**
 * @brief Exception thrown when DMA operation fails
 */
class DMAException : public AcceleratorException {
public:
    explicit DMAException(const std::string& message)
        : AcceleratorException("DMA operation failed: " + message) {}
};

} // namespace hal
} // namespace neurax

#endif /* NEURAX_HAL_ACCELERATOR_EXCEPTION_HPP */
