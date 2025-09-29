/**
 * @file neurax_bsp.c
 * @brief Implementation of Board Support Package for NEURAX accelerator
 *
 * This file implements the low-level hardware interface functions
 * for the NEURAX neural network accelerator on DE1-SoC platform.
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

/* Feature test macros must be defined before any headers */
#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE

#include "neurax/bsp/neurax_bsp.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

/* Static variables for memory mapping */
static int mem_fd = -1;
static void *mapped_base = NULL;
static volatile uint32_t *neurax_reg_base = NULL;

/* Helper function to map physical memory */
static int map_physical_memory(void) {
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        return -errno;
    }

    mapped_base = mmap(0, H2F_AXI_MASTER_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED,
                      mem_fd, H2F_AXI_MASTER_OFFSET);
    if (mapped_base == MAP_FAILED) {
        close(mem_fd);
        mem_fd = -1;
        return -errno;
    }

    neurax_reg_base = (volatile uint32_t *)(mapped_base + NEURAX_BASE_ADDR);
    return 0;
}

/* Helper function to unmap physical memory */
static void unmap_physical_memory(void) {
    if (mapped_base != NULL) {
        munmap(mapped_base, H2F_AXI_MASTER_OFFSET);
        mapped_base = NULL;
        neurax_reg_base = NULL;
    }
    if (mem_fd != -1) {
        close(mem_fd);
        mem_fd = -1;
    }
}

int neurax_bsp_init(void) {
    int ret = map_physical_memory();
    if (ret != 0) {
        return ret;
    }

    /* Reset the accelerator on initialization */
    /* TODO: Uncoment */
    // return neurax_bsp_reset();
}

int neurax_bsp_deinit(void) {
    /* Disable accelerator before cleanup */
    if (neurax_reg_base != NULL) {
        neurax_reg_base[0] = 0; /* Clear control register */
    }

    unmap_physical_memory();
    return 0;
}

int neurax_bsp_reset(void) {
    if (neurax_reg_base == NULL) {
        return -1;
    }

    /* Assert reset */
    neurax_reg_base[0] = NEURAX_CTRL_RESET;

    /* Small delay */
    usleep(1000);

    /* Deassert reset and enable */
    neurax_reg_base[0] = NEURAX_CTRL_ENABLE;

    return 0;
}

int neurax_bsp_write_reg(uint32_t offset, uint32_t value) {
    if (neurax_reg_base == NULL) {
        return -1;
    }

    if (offset >= NEURAX_REG_SPAN) {
        return -2; /* Offset out of range */
    }

    neurax_reg_base[offset] = value;
    return 0;
}

int neurax_bsp_read_reg(uint32_t offset, uint32_t *value) {
    if (neurax_reg_base == NULL || value == NULL) {
        return -1;
    }

    if (offset >= NEURAX_REG_SPAN) {
        return -2; /* Offset out of range */
    }

    *value = neurax_reg_base[offset];
    return 0;
}

int neurax_bsp_config_conv(const neurax_conv_config_t *config) {
    if (config == NULL || neurax_reg_base == NULL) {
        return -1;
    }

    /* Configure convolution parameters in hardware registers */
    /* This is a simplified implementation - actual register layout
       would be defined by the FPGA design */
    uint32_t conv_config = (config->kernel_size << 24) |
                          (config->stride << 16) |
                          (config->padding << 8) |
                          (config->input_channels & 0xFF);

    neurax_reg_base[2] = conv_config; /* Assuming offset 0x08 for config */

    return 0;
}

int neurax_bsp_config_activation(neurax_activation_t activation) {
    if (neurax_reg_base == NULL) {
        return -1;
    }

    uint32_t current_config;
    current_config = neurax_reg_base[2];

    /* Clear activation bits and set new value */
    current_config &= ~(0x3 << 4);
    current_config |= (activation & 0x3) << 4;

    neurax_reg_base[2] = current_config;

    return 0;
}

int neurax_bsp_config_pooling(const neurax_pool_config_t *config) {
    if (config == NULL || neurax_reg_base == NULL) {
        return -1;
    }

    /* Configure pooling parameters */
    uint32_t pool_config = (config->pool_size << 16) |
                          (config->stride << 8) |
                          (config->type & 0x1);

    neurax_reg_base[3] = pool_config; /* Assuming another config register */

    return 0;
}

int neurax_bsp_set_data_width(neurax_data_width_t width) {
    if (neurax_reg_base == NULL) {
        return -1;
    }

    uint32_t ctrl_reg = neurax_reg_base[0];

    if (width == NEURAX_DATA_16BIT) {
        ctrl_reg |= (1 << 7); /* Set 16-bit mode */
    } else {
        ctrl_reg &= ~(1 << 7); /* Clear for 8-bit mode */
    }

    neurax_reg_base[0] = ctrl_reg;

    return 0;
}

int neurax_bsp_start(void) {
    if (neurax_reg_base == NULL) {
        return -1;
    }

    uint32_t ctrl_reg = neurax_reg_base[0];
    ctrl_reg |= NEURAX_CTRL_START;
    neurax_reg_base[0] = ctrl_reg;

    return 0;
}

bool neurax_bsp_is_ready(void) {
    if (neurax_reg_base == NULL) {
        return false;
    }

    uint32_t status = neurax_reg_base[1]; /* Status register */
    return (status & NEURAX_STATUS_READY) != 0;
}

bool neurax_bsp_is_busy(void) {
    if (neurax_reg_base == NULL) {
        return false;
    }

    uint32_t status = neurax_reg_base[1]; /* Status register */
    return (status & NEURAX_STATUS_BUSY) != 0;
}

int neurax_bsp_wait_done(uint32_t timeout_ms) {
    if (neurax_reg_base == NULL) {
        return -1;
    }

    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        uint32_t status = neurax_reg_base[1];
        if (status & NEURAX_STATUS_DONE) {
            return 0; /* Operation completed */
        }

        if (status & NEURAX_STATUS_ERROR) {
            return -2; /* Hardware error */
        }

        clock_gettime(CLOCK_MONOTONIC, &current);
        uint32_t elapsed_ms = (current.tv_sec - start.tv_sec) * 1000 +
                             (current.tv_nsec - start.tv_nsec) / 1000000;

        if (elapsed_ms >= timeout_ms) {
            return -1; /* Timeout */
        }

        usleep(1000); /* Sleep 1ms */
    }
}

int neurax_bsp_dma_setup(uint32_t src_addr, uint32_t dst_addr, size_t size) {
    if (neurax_reg_base == NULL) {
        return -1;
    }

    /* Configure DMA registers - simplified implementation */
    neurax_reg_base[8] = src_addr;   /* DMA source address */
    neurax_reg_base[9] = dst_addr;   /* DMA destination address */
    neurax_reg_base[10] = size;      /* DMA transfer size */

    return 0;
}

int neurax_bsp_dma_start(void) {
    if (neurax_reg_base == NULL) {
        return -1;
    }

    uint32_t dma_ctrl = neurax_reg_base[8]; /* DMA control register */
    dma_ctrl |= 0x1; /* Start DMA */
    neurax_reg_base[8] = dma_ctrl;

    return 0;
}

int neurax_bsp_dma_wait(uint32_t timeout_ms) {
    if (neurax_reg_base == NULL) {
        return -1;
    }

    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        uint32_t dma_status = neurax_reg_base[8];
        if ((dma_status & 0x2) == 0) { /* DMA done bit cleared */
            return 0;
        }

        clock_gettime(CLOCK_MONOTONIC, &current);
        uint32_t elapsed_ms = (current.tv_sec - start.tv_sec) * 1000 +
                             (current.tv_nsec - start.tv_nsec) / 1000000;

        if (elapsed_ms >= timeout_ms) {
            return -1; /* Timeout */
        }

        usleep(1000); /* Sleep 1ms */
    }
}
