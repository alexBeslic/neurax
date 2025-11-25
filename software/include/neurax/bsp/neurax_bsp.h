/**
 * @file neurax_bsp.h
 * @brief Board Support Package (BSP) for NEURAX Neural Network Accelerator
 *
 * This header defines the low-level hardware interface for the NEURAX
 * accelerator running on DE1-SoC FPGA platform. BSP layer provides
 * direct hardware communication functions.
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#ifndef NEURAX_BSP_H
#define NEURAX_BSP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "neurax/bsp/hps.h"

#include "neurax/bsp/hps_soc_system.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hardware register addresses and offsets */
#define H2F_AXI_MASTER_SPAN          ALT_H2F_UB_ADDR - ALT_H2F_LB_ADDR + 1
#define H2F_AXI_MASTER_OFFSET        ALT_H2F_OFST
#define NEURAX_BASE_ADDR             NEURAX_AVALON_ACCELERATOR_0_BASE
#define NEURAX_REG_SPAN              NEURAX_AVALON_ACCELERATOR_0_SPAN
#define NEURAX_CMD_OFFSET               (0x00)
#define NEURAX_STATUS_OFFSET            (0x04)
#define NEURAX_CONFIG_OFFSET            (0x08)
#define NEURAX_CONV_CONFIG_0_OFFSET     (0x0C)
#define NEURAX_CONV_CONFIG_1_OFFSET     (0x10)
#define NEURAX_POOL_CONFIG_OFFSET       (0x14)
#define NEURAX_ACTIVATION_CONFIG_OFFSET (0x18)
#define NEURAX_ACTIVATION_ALPHA_OFFSET  (0x1C)
#define NEURAX_BATCH_SIZE_OFFSET        (0x20)
#define NEURAX_TEMP_0_OFFSET            (0x24)
#define NEURAX_TEMP_1_OFFSET            (0x28)
#define NEURAX_TEMP_2_OFFSET            (0x2C)
#define NEURAX_TEMP_3_OFFSET            (0x30)
#define NEURAX_DEBUG_CYCLES_OFFSET      (0x34)
#define NEURAX_DEBUG_STATUS_OFFSET      (0x38)
#define NEURAX_READ_ONLY_OFFSET         (0x3C)

/* Control register bit definitions */
#define NEURAX_CTRL_ENABLE      (1 << 0)
#define NEURAX_CTRL_RESET       (1 << 1)
#define NEURAX_CTRL_START       (1 << 2)
#define NEURAX_CTRL_CONV_EN     (1 << 4)
#define NEURAX_CTRL_ACTIV_EN    (1 << 5)
#define NEURAX_CTRL_POOL_EN     (1 << 6)

/* Status register bit definitions */
#define NEURAX_STATUS_READY     (1 << 0)
#define NEURAX_STATUS_BUSY      (1 << 1)
#define NEURAX_STATUS_DONE      (1 << 2)
#define NEURAX_STATUS_ERROR     (1 << 3)

/* Read only registers */
#define NEURAX_READ_ONLY_ID     0xCAB00D1E 

/* Data types */
typedef enum {
    NEURAX_DATA_8BIT = 0,
    NEURAX_DATA_16BIT = 1
} neurax_data_width_t;

typedef enum {
    NEURAX_ACTIV_RELU = 0,
    NEURAX_ACTIV_TANH = 1,
    NEURAX_ACTIV_SIGMOID = 2
} neurax_activation_t;

typedef enum {
    NEURAX_POOL_MAX = 0,
    NEURAX_POOL_AVG = 1
} neurax_pooling_t;

typedef struct {
    uint32_t kernel_size;
    uint32_t stride;
    uint32_t padding;
    uint32_t input_channels;
    uint32_t output_channels;
} neurax_conv_config_t;

typedef struct {
    uint32_t pool_size;
    uint32_t stride;
    neurax_pooling_t type;
} neurax_pool_config_t;

/* BSP Function declarations */

/**
 * @brief Initialize the NEURAX accelerator hardware
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_init(void);

/**
 * @brief Deinitialize the NEURAX accelerator hardware
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_deinit(void);

/**
 * @brief Reset the NEURAX accelerator
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_reset(void);

/**
 * @brief Write to a hardware register
 * @param offset Register offset from base address
 * @param value Value to write
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_write_reg(uint32_t offset, uint32_t value);

/**
 * @brief Read from a hardware register
 * @param offset Register offset from base address
 * @param value Pointer to store read value
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_read_reg(uint32_t offset, uint32_t *value);

/**
 * @brief Configure convolution block
 * @param config Convolution configuration
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_config_conv(const neurax_conv_config_t *config);

/**
 * @brief Configure activation function
 * @param activation Activation function type
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_config_activation(neurax_activation_t activation);

/**
 * @brief Configure pooling block
 * @param config Pooling configuration
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_config_pooling(const neurax_pool_config_t *config);

/**
 * @brief Set data width for processing
 * @param width Data width (8-bit or 16-bit)
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_set_data_width(neurax_data_width_t width);

/**
 * @brief Start accelerator operation
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_start(void);

/**
 * @brief Check if accelerator is ready
 * @return true if ready, false otherwise
 */
bool neurax_bsp_is_ready(void);

/**
 * @brief Check if accelerator is busy
 * @return true if busy, false otherwise
 */
bool neurax_bsp_is_busy(void);

/**
 * @brief Wait for operation completion
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on success, -1 on timeout, negative error code on other failures
 */
int neurax_bsp_wait_done(uint32_t timeout_ms);

/**
 * @brief Setup DMA transfer
 * @param src_addr Source address
 * @param dst_addr Destination address
 * @param size Transfer size in bytes
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_dma_setup(uint32_t src_addr, uint32_t dst_addr, size_t size);

/**
 * @brief Start DMA transfer
 * @return 0 on success, negative error code on failure
 */
int neurax_bsp_dma_start(void);

/**
 * @brief Wait for DMA completion
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on success, -1 on timeout, negative error code on other failures
 */
int neurax_bsp_dma_wait(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* NEURAX_BSP_H */
