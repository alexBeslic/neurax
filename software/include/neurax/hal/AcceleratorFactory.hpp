/**
 * @file AcceleratorFactory.hpp
 * @brief Factory for creating and managing accelerator instances
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_HAL_ACCELERATOR_FACTORY_HPP
#define NEURAX_HAL_ACCELERATOR_FACTORY_HPP

#include "IAccelerator.hpp"
#include "AcceleratorTypes.hpp"
#include <memory>
#include <vector>

namespace neurax {
namespace hal {

/**
 * @brief Factory class for creating accelerator instances
 *
 * This factory provides methods to create accelerator instances,
 * query available accelerators, and implement fallback strategies.
 */
class AcceleratorFactory {
public:
    /**
     * @brief Get singleton instance of the factory
     * @return Reference to the singleton instance
     */
    static AcceleratorFactory& getInstance() {
        static AcceleratorFactory instance;
        return instance;
    }

    /**
     * @brief Create an accelerator instance (non-static version)
     * @param type Desired accelerator type
     * @return Unique pointer to accelerator, or nullptr if creation failed
     */
    std::unique_ptr<IAccelerator> createAccelerator(AcceleratorType type) {
        return create(type);
    }

    /**
     * @brief Create an accelerator instance of specified type
     * if available, otherwise fallback to next best option.
     * At worst, returns a SoftwareAccelerator.
     * @param type Desired accelerator type
     * @return Unique pointer to accelerator
     */
    static std::unique_ptr<IAccelerator> create(AcceleratorType type);

    /**
     * @brief Get list of available accelerator types
     *
     * Tests each accelerator type and returns only those that are
     * actually available on the current system.
     *
     * @return Vector of available accelerator types
     */
    static std::vector<AcceleratorType> get_available_accelerators();

    /**
     * @brief Get the best available accelerator type
     *
     * Returns the highest-performance accelerator that is available
     * on the current system, in order of preference:
     * FPGA_DE1SOC > GPU_OPENCL > CPU_OPTIMIZED > SOFTWARE_FALLBACK
     *
     * @return Best available accelerator type
     */
    static AcceleratorType get_best_available();

    /**
     * @brief Check if specific accelerator type is available
     * @param type Accelerator type to check
     * @return true if accelerator is available and can be created
     */
    static bool is_available(AcceleratorType type);

    /**
     * @brief Get human-readable name for accelerator type
     * @param type Accelerator type
     * @return String description of the accelerator
     */
    static std::string get_accelerator_name(AcceleratorType type);

    /**
     * @brief Get detailed information about accelerator capabilities
     * @param type Accelerator type
     * @return String with detailed capability information
     */
    static std::string get_accelerator_info(AcceleratorType type);

private:
    /**
     * @brief Private constructor for singleton pattern
     */
    AcceleratorFactory() = default;

    /**
     * @brief Deleted copy constructor
     */
    AcceleratorFactory(const AcceleratorFactory&) = delete;

    /**
     * @brief Deleted assignment operator
     */
    AcceleratorFactory& operator=(const AcceleratorFactory&) = delete;

    /**
     * @brief Internal helper to attempt accelerator creation
     * @param type Accelerator type to create
     * @return Unique pointer to accelerator, or nullptr if failed
     */
    static std::unique_ptr<IAccelerator> try_create(AcceleratorType type);

    /**
     * @brief Test if accelerator can be initialized
     * @param accelerator Accelerator instance to test
     * @return true if accelerator initializes successfully
     */
    static bool test_accelerator(IAccelerator* accelerator);
};

} // namespace hal
} // namespace neurax

#endif /* NEURAX_HAL_ACCELERATOR_FACTORY_HPP */
