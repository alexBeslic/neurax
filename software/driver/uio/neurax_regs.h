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
    volatile uint32_t reg_data_read; /**< [15:0] start word, [31:16] word count */
    volatile uint32_t reg_reserved_11; /**< Reserved register */
    volatile uint32_t reg_reserved_12; /**< Reserved register */
    volatile uint32_t reg_reserved_13; /**< Reserved register */
    volatile uint32_t reg_reserved_14; /**< Reserved register */
    const volatile uint32_t reg_read_only; /**< Magic number 0xCAB00D1E*/
} neurax_reg_t;

/* DATA_SC reset handshake: write REQ; FPGA clears REQ and sets ACK when idle. */
#define NEURAX_DATA_SC_WR_RESET_REQ (1u << 2)
#define NEURAX_DATA_SC_WR_RESET_ACK (1u << 3)
#define NEURAX_DATA_SC_READ_ERROR  (1u << 4)

/* STATUS register reports a rejected/invalid operation configuration. */
#define NEURAX_STATUS_ERROR        (1u << 8)

/* DATA_READ: [15:0] start word, [31:16] word count. Reject rather than mask. */
static inline int neurax_data_read_pack(uint32_t start, uint32_t count,
                                        uint32_t *packed)
{
    if (!packed || start > 0xFFFFu || count == 0 || count > 0xFFFFu ||
        count > 0x10000u - start)
        return -1;

    *packed = (count << 16) | start;
    return 0;
}

#endif /* NEURAX_REGS_H */