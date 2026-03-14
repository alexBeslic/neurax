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

static inline void dma_csr_write(volatile uint32_t *csr, int reg_byte_offset, uint32_t val) {
    csr[reg_byte_offset / 4] = val;
}

static inline uint32_t dma_csr_read(volatile uint32_t *csr, int reg_byte_offset) {
    return csr[reg_byte_offset / 4];
}

static inline void dma_desc_write(volatile uint32_t *desc, int reg_byte_offset, uint32_t val) {
    desc[reg_byte_offset / 4] = val;
}

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
    bsp->dma_read_csr   = (volatile uint32_t *)(base + DMA_READ_CSR_OFFSET);
    bsp->dma_write_csr  = (volatile uint32_t *)(base + DMA_WRITE_CSR_OFFSET);
    bsp->dma_read_desc  = (volatile uint32_t *)(base + DMA_READ_DESC_OFFSET);
    bsp->dma_write_desc = (volatile uint32_t *)(base + DMA_WRITE_DESC_OFFSET);

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

/* =========================================================================
 * DMA Operations (Altera mSGDMA)
 * ========================================================================= */

void neurax_dma_reset(volatile uint32_t *dma_csr) {
    /* Assert dispatcher reset */
    dma_csr_write(dma_csr, MSGDMA_CSR_CONTROL, MSGDMA_CSR_RESET);
    msleep(1);

    /* Wait for reset to complete */
    int timeout = 1000;
    while ((dma_csr_read(dma_csr, MSGDMA_CSR_STATUS) & MSGDMA_CSR_RESETTING) && timeout > 0) {
        usleep(100);
        timeout--;
    }

    /* Clear any pending IRQ */
    dma_csr_write(dma_csr, MSGDMA_CSR_STATUS, MSGDMA_CSR_IRQ);
}

int neurax_dma_send(neurax_bsp_t *bsp, uint32_t phys_addr, uint32_t length) {
    volatile uint32_t *csr  = bsp->dma_write_csr;
    volatile uint32_t *desc = bsp->dma_write_desc;

    /* Check DMA is not busy */
    if (dma_csr_read(csr, MSGDMA_CSR_STATUS) & MSGDMA_CSR_BUSY) {
        fprintf(stderr, "neurax_dma_send: DMA write engine busy\n");
        return -1;
    }

    /*
     * MM→ST descriptor:
     *   read_address  = phys_addr (source in HPS DDR)
     *   write_address = 0 (unused for MM→ST — data goes to ST interface)
     *   length        = transfer size in bytes
     *   control       = GO | GENERATE_SOP | GENERATE_EOP
     *
     * The mSGDMA will read 'length' bytes from 'phys_addr' via the F2H bridge
     * and stream them out the ST source to neurax's avalon_sink.
     */
    dma_desc_write(desc, MSGDMA_DESC_READ_ADDR, phys_addr);
    dma_desc_write(desc, MSGDMA_DESC_WRITE_ADDR, 0);
    dma_desc_write(desc, MSGDMA_DESC_LENGTH, length);
    dma_desc_write(desc, MSGDMA_DESC_CONTROL,
                   MSGDMA_DESC_CTL_GO
                   | MSGDMA_DESC_CTL_GENERATE_SOP
                   | MSGDMA_DESC_CTL_GENERATE_EOP
                   | MSGDMA_DESC_CTL_TX_CHANNEL(0));

    return 0;
}

int neurax_dma_recv(neurax_bsp_t *bsp, uint32_t phys_addr, uint32_t length) {
    volatile uint32_t *csr  = bsp->dma_read_csr;
    volatile uint32_t *desc = bsp->dma_read_desc;

    /* Check DMA is not busy */
    if (dma_csr_read(csr, MSGDMA_CSR_STATUS) & MSGDMA_CSR_BUSY) {
        fprintf(stderr, "neurax_dma_recv: DMA read engine busy\n");
        return -1;
    }

    /*
     * ST→MM descriptor:
     *   read_address  = 0 (unused for ST→MM — data comes from ST interface)
     *   write_address = phys_addr (destination in HPS DDR)
     *   length        = transfer size in bytes
     *   control       = GO | END_ON_EOP
     *
     * The mSGDMA will accept data from neurax's avalon_source via the ST sink
     * and write it to 'phys_addr' via the F2H bridge.
     */
    dma_desc_write(desc, MSGDMA_DESC_READ_ADDR, 0);
    dma_desc_write(desc, MSGDMA_DESC_WRITE_ADDR, phys_addr);
    dma_desc_write(desc, MSGDMA_DESC_LENGTH, length);
    dma_desc_write(desc, MSGDMA_DESC_CONTROL,
                   MSGDMA_DESC_CTL_GO
                   | MSGDMA_DESC_CTL_END_ON_EOP);

    return 0;
}

int neurax_dma_send_wait(neurax_bsp_t *bsp) {
    volatile uint32_t *csr = bsp->dma_write_csr;
    int timeout = 1000000;
    while ((dma_csr_read(csr, MSGDMA_CSR_STATUS) & MSGDMA_CSR_BUSY) && timeout > 0) {
        usleep(1);
        timeout--;
    }
    if (timeout <= 0) {
        fprintf(stderr, "neurax_dma_send_wait: TIMEOUT\n");
        return -1;
    }
    return 0;
}

int neurax_dma_recv_wait(neurax_bsp_t *bsp) {
    volatile uint32_t *csr = bsp->dma_read_csr;
    int timeout = 1000000;
    while ((dma_csr_read(csr, MSGDMA_CSR_STATUS) & MSGDMA_CSR_BUSY) && timeout > 0) {
        usleep(1);
        timeout--;
    }
    if (timeout <= 0) {
        fprintf(stderr, "neurax_dma_recv_wait: TIMEOUT\n");
        return -1;
    }
    return 0;
}

/* =========================================================================
 * High-level: Full Convolution Cycle
 * ========================================================================= */

int neurax_run_conv(neurax_bsp_t *bsp,
                    const neurax_conv_config_t *cfg,
                    uint32_t input_phys, uint32_t input_bytes,
                    uint32_t output_phys, uint32_t output_bytes) {
    int rc;

    /* 1. Reset DMAs */
    neurax_dma_reset(bsp->dma_write_csr);
    neurax_dma_reset(bsp->dma_read_csr);

    /* 2. Configure the accelerator */
    neurax_reset(bsp);
    neurax_config_conv(bsp, cfg);
    neurax_set_batch_size(bsp, 1);

    /* 3. DMA send: input data → FPGA shared RAM */
    rc = neurax_dma_send(bsp, input_phys, input_bytes);
    if (rc) return rc;
    rc = neurax_dma_send_wait(bsp);
    if (rc) return rc;

    /* 4. Start convolution */
    neurax_start(bsp, OP_CONVOLUTION);

    /* 5. Wait for accelerator to finish */
    uint32_t cycles = neurax_wait_done(bsp);
    printf("Convolution done in %u cycles\n", cycles);

    /* 6. DMA recv: results from FPGA shared RAM → HPS memory */
    rc = neurax_dma_recv(bsp, output_phys, output_bytes);
    if (rc) return rc;
    rc = neurax_dma_recv_wait(bsp);
    if (rc) return rc;

    return 0;
}
