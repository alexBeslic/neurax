/**
 * @file neurax_bsp.c
 * @brief Neurax FPGA CNN Accelerator - BSP Implementation
 */

#include "neurax_bsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

/* =========================================================================
 * Internal helpers
 * ========================================================================= */

static void msleep(int ms) {
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

/* =========================================================================
 * BSP Init / Deinit
 * ========================================================================= */

int neurax_bsp_init(neurax_bsp_t *bsp) {
    memset(bsp, 0, sizeof(*bsp));

    bsp->fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if (bsp->fd_mem < 0) {
        perror("neurax_bsp_init: open /dev/mem");
        return -1;
    }

    bsp->lw_bridge = mmap(NULL, LW_BRIDGE_SPAN,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          bsp->fd_mem, LW_BRIDGE_BASE);
    if (bsp->lw_bridge == MAP_FAILED) {
        perror("neurax_bsp_init: mmap LW bridge");
        close(bsp->fd_mem);
        return -1;
    }

    uint8_t *base = (uint8_t *)bsp->lw_bridge;

    bsp->regs           = (volatile uint32_t *)(base + NEURAX_REG_OFFSET);

    return 0;
}

void neurax_bsp_deinit(neurax_bsp_t *bsp) {
    if (bsp->regs) {
        /* Disable accelerator */
        bsp->regs[REG_CMD] = 0;
    }

    if (bsp->lw_bridge && bsp->lw_bridge != MAP_FAILED) {
        munmap(bsp->lw_bridge, LW_BRIDGE_SPAN);
    }

    if (bsp->fd_mem >= 0) {
        close(bsp->fd_mem);
    }

    memset(bsp, 0, sizeof(*bsp));
    bsp->fd_mem = -1;
}

/* =========================================================================
 * Register Access
 * ========================================================================= */

uint32_t neurax_reg_read(neurax_bsp_t *bsp, int reg) {
    if (reg < 0 || reg > 15) return 0xFFFFFFFF;
    return bsp->regs[reg];
}

void neurax_reg_write(neurax_bsp_t *bsp, int reg, uint32_t value) {
    if (reg < 0 || reg > 15) return;
    bsp->regs[reg] = value;
}

int neurax_check_id(neurax_bsp_t *bsp) {
    uint32_t id = neurax_reg_read(bsp, REG_READ_ONLY);
    if (id != NEURAX_MAGIC_ID) {
        fprintf(stderr, "neurax_check_id: expected 0x%08X, got 0x%08X\n",
                NEURAX_MAGIC_ID, id);
        return -1;
    }
    return 0;
}

/* =========================================================================
 * Accelerator Control
 * ========================================================================= */

void neurax_reset(neurax_bsp_t *bsp) {
    /* Disable */
    neurax_reg_write(bsp, REG_CMD, 0);
    msleep(1);
    /* Re-enable */
    neurax_reg_write(bsp, REG_CMD, CMD_ENABLE);
    msleep(1);
}

void neurax_enable(neurax_bsp_t *bsp) {
    uint32_t cmd = neurax_reg_read(bsp, REG_CMD);
    cmd |= CMD_ENABLE;
    neurax_reg_write(bsp, REG_CMD, cmd);
}

void neurax_config_conv(neurax_bsp_t *bsp, const neurax_conv_config_t *cfg) {
    /* REG_CONV_CONFIG_0: stride[7:0] | padding[15:8] | groups[23:16] | kernel_size[31:24] */
    uint32_t conv0 = ((cfg->kernel_size & 0xFF) << 24)
                   | ((cfg->input_channels & 0xFF) << 16)  /* groups = input channels for now */
                   | ((cfg->padding & 0xFF) << 8)
                   | ((cfg->stride & 0xFF) << 0);
    neurax_reg_write(bsp, REG_CONV_CONFIG_0, conv0);

    /* REG_CONV_CONFIG_1: input_channels[7:0] | output_channels[15:8] */
    uint32_t conv1 = ((cfg->output_channels & 0xFF) << 8)
                   | ((cfg->input_channels & 0xFF) << 0);
    neurax_reg_write(bsp, REG_CONV_CONFIG_1, conv1);
}

void neurax_config_pool(neurax_bsp_t *bsp, const neurax_pool_config_t *cfg) {
    /* REG_POOL_CONFIG: size[7:0] | stride[15:8] | type[23:16] | channels[31:24] */
    uint32_t pool = ((cfg->channels & 0xFF) << 24)
                  | ((cfg->pool_type & 0xFF) << 16)
                  | ((cfg->stride & 0xFF) << 8)
                  | ((cfg->pool_size & 0xFF) << 0);
    neurax_reg_write(bsp, REG_POOL_CONFIG, pool);
}

void neurax_config_activation(neurax_bsp_t *bsp, const neurax_activation_config_t *cfg) {
    /* REG_ACTIVATION_CONFIG: type[7:0] | tensor_size[23:8] */
    uint32_t act = ((cfg->tensor_size & 0xFFFF) << 8)
                 | ((cfg->activation_type & 0xFF) << 0);
    neurax_reg_write(bsp, REG_ACTIVATION_CONFIG, act);

    /* REG_ACTIVATION_ALPHA */
    neurax_reg_write(bsp, REG_ACTIVATION_ALPHA, (uint32_t)(uint16_t)cfg->alpha);
}

void neurax_set_batch_size(neurax_bsp_t *bsp, int batch_size) {
    neurax_reg_write(bsp, REG_BATCH_SIZE, batch_size & 0xFF);
}

void neurax_start(neurax_bsp_t *bsp, int operation) {
    uint32_t cmd = CMD_ENABLE
                 | ((operation & 0x3) << CMD_OP_SELECT_SHIFT)
                 | CMD_START;
    neurax_reg_write(bsp, REG_CMD, cmd);

    /*
     * The start bit is edge-detected in HW (fires on IDLE→OP transition).
     * Clear the start bit after a short delay to avoid retriggering.
     */
    usleep(10);
    cmd &= ~CMD_START;
    neurax_reg_write(bsp, REG_CMD, cmd);
}

int neurax_is_busy(neurax_bsp_t *bsp) {
    return (neurax_reg_read(bsp, REG_STATUS) & STATUS_BUSY) ? 1 : 0;
}

int neurax_is_done(neurax_bsp_t *bsp) {
    return (neurax_reg_read(bsp, REG_STATUS) & STATUS_DONE) ? 1 : 0;
}

uint32_t neurax_wait_done(neurax_bsp_t *bsp) {
    int timeout = 1000000; /* ~1 second at 1µs poll rate */
    while (!neurax_is_done(bsp) && timeout > 0) {
        usleep(1);
        timeout--;
    }
    if (timeout <= 0) {
        fprintf(stderr, "neurax_wait_done: TIMEOUT\n");
    }
    return neurax_reg_read(bsp, REG_DEBUG_CYCLES);
}
