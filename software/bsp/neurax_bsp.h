/**
 * @file neurax_bsp.h
 * @brief Neurax FPGA CNN Accelerator - Board Support Package
 *
 * Userspace driver for the Neurax accelerator on DE1-SoC.
 * Provides register access (via /dev/mem mmap) for configuring and
 * controlling the accelerator. DMA transfers are handled by the
 * Linux msgdma character device driver (/dev/msgdma).
 *
 * Memory Map (Lightweight H2F Bridge @ 0xFF200000):
 *   0x00 - 0x3F : Neurax register block (16 word-addressed regs)
 *
 * Data Flow:
 *   1. HPS configures neurax registers (conv/pool/activation params)
 *   2. HPS writes input data to FPGA via /dev/msgdma (write)
 *   3. HPS starts accelerator (start bit in CMD register)
 *   4. HPS polls for done or waits for IRQ
 *   5. HPS reads results from FPGA via /dev/msgdma (read)
 */

#ifndef NEURAX_BSP_H
#define NEURAX_BSP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Hardware Constants
 * ========================================================================= */

/* Lightweight H2F bridge physical base */
#define LW_BRIDGE_BASE      0xFF200000
#define LW_BRIDGE_SPAN      0x00200000  /* 2 MB */

/* Component offsets from LW bridge base */
#define NEURAX_REG_OFFSET   0x0000
#define NEURAX_REG_SPAN     0x0040      /* 64 bytes = 16 regs × 4 */

/* =========================================================================
 * Neurax Register Addresses (word offsets, multiply by 4 for byte offset)
 * ========================================================================= */
#define REG_CMD                 0   /* 0x00 */
#define REG_STATUS              1   /* 0x04 */
#define REG_CONFIG              2   /* 0x08 */
#define REG_CONV_CONFIG_0       3   /* 0x0C */
#define REG_CONV_CONFIG_1       4   /* 0x10 */
#define REG_POOL_CONFIG         5   /* 0x14 */
#define REG_ACTIVATION_CONFIG   6   /* 0x18 */
#define REG_ACTIVATION_ALPHA    7   /* 0x1C */
#define REG_BATCH_SIZE          8   /* 0x20 */
#define REG_TEMP_0              9   /* 0x24 */
#define REG_TEMP_1             10   /* 0x28 */
#define REG_TEMP_2             11   /* 0x2C */
#define REG_TEMP_3             12   /* 0x30 */
#define REG_DEBUG_CYCLES       13   /* 0x34 */
#define REG_DEBUG_STATUS       14   /* 0x38 */
#define REG_READ_ONLY          15   /* 0x3C */

/* =========================================================================
 * Command Register (REG_CMD = 0x00) Bit Fields
 * ========================================================================= */
#define CMD_ENABLE              (1 << 0)
#define CMD_OP_SELECT_SHIFT     1
#define CMD_OP_SELECT_MASK      (0x3 << CMD_OP_SELECT_SHIFT)
#define CMD_START               (1 << 3)
#define CMD_WEIGHT_VALID        (1 << 16)
#define CMD_BIAS_VALID          (1 << 17)

/* Operation select values */
#define OP_CONVOLUTION          0
#define OP_POOLING              1
#define OP_ACTIVATION           2

/* =========================================================================
 * Status Register (REG_STATUS = 0x04) Bit Fields
 * ========================================================================= */
#define STATUS_INPUT_READY      (1 << 0)
#define STATUS_INPUT_VALID      (1 << 1)
#define STATUS_OUTPUT_READY     (1 << 2)
#define STATUS_OUTPUT_VALID     (1 << 3)
#define STATUS_DONE             (1 << 4)
#define STATUS_BUSY             (1 << 5)
#define STATUS_CURRENT_OP_SHIFT 6
#define STATUS_CURRENT_OP_MASK  (0x3 << STATUS_CURRENT_OP_SHIFT)
#define STATUS_WEIGHT_READY     (1 << 17)
#define STATUS_BIAS_READY       (1 << 18)

/* =========================================================================
 * Pooling Type Constants
 * ========================================================================= */
#define POOL_MAX                0
#define POOL_AVERAGE            1
#define POOL_MIN                2
#define POOL_SUM                3

/* =========================================================================
 * Activation Type Constants
 * ========================================================================= */
#define ACT_RELU                0
#define ACT_SIGMOID             1
#define ACT_TANH                2
#define ACT_LINEAR              3
#define ACT_LEAKY_RELU          4
#define ACT_ELU                 5

/* Magic ID for read-only register */
#define NEURAX_MAGIC_ID         0xCAB00D1E

/* =========================================================================
 * Data Format: Q8.8 Fixed Point
 * ========================================================================= */
#define FRAC_BITS       8
#define DATA_WIDTH      16

/** Convert float to Q8.8 fixed point (16-bit) */
static inline int16_t float_to_q8_8(float val) {
    int32_t fixed = (int32_t)(val * (1 << FRAC_BITS));
    if (fixed > 32767) fixed = 32767;
    if (fixed < -32768) fixed = -32768;
    return (int16_t)fixed;
}

/** Convert Q8.8 fixed point to float */
static inline float q8_8_to_float(int16_t val) {
    return (float)val / (float)(1 << FRAC_BITS);
}

/** Pack two Q8.8 values into one 32-bit word (low=val0, high=val1) */
static inline uint32_t pack_q8_8_pair(int16_t val0, int16_t val1) {
    return ((uint32_t)(uint16_t)val1 << 16) | (uint32_t)(uint16_t)val0;
}

/* =========================================================================
 * Configuration Structures
 * ========================================================================= */

typedef struct {
    int kernel_size;        /* 1..5 */
    int stride;             /* 1..4 */
    int padding;            /* 0..2 */
    int input_channels;     /* 1..16 */
    int output_channels;    /* 1..16 */
} neurax_conv_config_t;

typedef struct {
    int pool_size;          /* 1..8 */
    int stride;             /* 1..8 */
    int pool_type;          /* POOL_MAX, POOL_AVERAGE, etc. */
    int channels;           /* 1..16 */
} neurax_pool_config_t;

typedef struct {
    int activation_type;    /* ACT_RELU, ACT_SIGMOID, etc. */
    int tensor_size;        /* 1..4096 */
    int16_t alpha;          /* Q8.8, for Leaky ReLU / ELU */
} neurax_activation_config_t;

/* =========================================================================
 * BSP Context
 * ========================================================================= */

typedef struct {
    int         fd_mem;         /* /dev/mem file descriptor */
    void       *lw_bridge;     /* mmap'd pointer to LW H2F bridge */
    volatile uint32_t *regs;   /* Neurax register base */
} neurax_bsp_t;

/* =========================================================================
 * BSP API
 * ========================================================================= */

/**
 * Initialize the BSP: open /dev/mem, mmap the LW bridge, set up pointers.
 * @return 0 on success, -1 on failure (check errno)
 */
int neurax_bsp_init(neurax_bsp_t *bsp);

/**
 * Deinitialize: disable accelerator, munmap, close /dev/mem.
 */
void neurax_bsp_deinit(neurax_bsp_t *bsp);

/** Read a neurax register (word index 0..15) */
uint32_t neurax_reg_read(neurax_bsp_t *bsp, int reg);

/** Write a neurax register (word index 0..15) */
void neurax_reg_write(neurax_bsp_t *bsp, int reg, uint32_t value);

/** Verify the magic ID register. Returns 0 on success. */
int neurax_check_id(neurax_bsp_t *bsp);

/** Reset the accelerator (toggle reset via enable bit) */
void neurax_reset(neurax_bsp_t *bsp);

/** Enable the accelerator */
void neurax_enable(neurax_bsp_t *bsp);

/** Configure convolution parameters */
void neurax_config_conv(neurax_bsp_t *bsp, const neurax_conv_config_t *cfg);

/** Configure pooling parameters */
void neurax_config_pool(neurax_bsp_t *bsp, const neurax_pool_config_t *cfg);

/** Configure activation parameters */
void neurax_config_activation(neurax_bsp_t *bsp, const neurax_activation_config_t *cfg);

/** Set batch size */
void neurax_set_batch_size(neurax_bsp_t *bsp, int batch_size);

/** Start an operation (convolution, pooling, or activation) */
void neurax_start(neurax_bsp_t *bsp, int operation);

/** Check if accelerator is busy */
int neurax_is_busy(neurax_bsp_t *bsp);

/** Check if operation is done */
int neurax_is_done(neurax_bsp_t *bsp);

/** Poll-wait until operation completes. Returns cycle count. */
uint32_t neurax_wait_done(neurax_bsp_t *bsp);

#ifdef __cplusplus
}
#endif

#endif /* NEURAX_BSP_H */
