#ifndef NEURAX_REGS_H
#define NEURAX_REGS_H
#include <stdint.h>

/**
 * @brief Neurax BSP register map structure
 */
typedef struct {
    volatile uint32_t reg_cmd; /**< Command register */
    const volatile uint32_t reg_status; /**< Status register */
    volatile uint32_t reg_config; /**< Config register */
    volatile uint32_t reg_conv_config_0; /**< Convolution config register 0 */
    volatile uint32_t reg_conv_config_1; /**< Convolution config register 1 */
    volatile uint32_t reg_pool_config; /**< Pooling config register */
    volatile uint32_t reg_activation_config; /**< Activation config register */
    volatile uint32_t reg_activation_alpha; /**< Activation alpha register */
    volatile uint32_t reg_batch_size; /**< Batch size register */
    volatile uint32_t reg_data_sc; /**< Data status/config register */
    volatile uint32_t reg_data_read; /**< Data start/length register */
    volatile uint32_t reg_temp_2; /**< Temporary register 2 */
    volatile uint32_t reg_temp_3; /**< Temporary register 3 */
    const volatile uint32_t reg_debug_cycles; /**< Debug cycles register */
    const volatile uint32_t reg_debug_status; /**< Debug status register */
    const volatile uint32_t reg_read_only; /**< Magic number 0xCAB00D1E*/
} neurax_reg_t;

#endif /* NEURAX_REGS_H */