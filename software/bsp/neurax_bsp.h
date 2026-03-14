/**
 * @file neurax_bsp.h
 * @brief Neurax FPGA CNN Accelerator - Board Support Package
 *
 * Userspace driver for the Neurax accelerator on DE1-SoC.
 * Provides register access (via /dev/mem mmap) and DMA transfers
 * (via Altera mSGDMA) for streaming data to/from the FPGA.
 *
 * Memory Map (Lightweight H2F Bridge @ 0xFF200000):
 *   0x00 - 0x3F : Neurax register block (16 word-addressed regs)
 *   0x40 - 0x5F : DMA_neurax_read CSR     (ST→MM, output from FPGA)
 *   0x60 - 0x7F : DMA_neurax_write CSR    (MM→ST, input to FPGA)
 *   0x80 - 0x8F : DMA_neurax_read descriptor slave
 *   0x90 - 0x9F : DMA_neurax_write descriptor slave
 *
 * Data Flow:
 *   1. HPS allocates contiguous buffer (or uses known physical address)
 *   2. HPS configures neurax registers (conv/pool/activation params)
 *   3. HPS programs DMA_write to stream input data → FPGA RAM
 *   4. HPS starts accelerator (start bit in CMD register)
 *   5. HPS polls for done or waits for IRQ
 *   6. HPS programs DMA_read to stream results FPGA RAM → HPS memory
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

#define DMA_READ_CSR_OFFSET  0x0040     /* ST→MM (output from FPGA) */
#define DMA_WRITE_CSR_OFFSET 0x0060     /* MM→ST (input to FPGA) */
#define DMA_READ_DESC_OFFSET 0x0080
#define DMA_WRITE_DESC_OFFSET 0x0090

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
 * mSGDMA Register Definitions (CSR block)
 * ========================================================================= */
#define MSGDMA_CSR_STATUS       0x00    /* Status register */
#define MSGDMA_CSR_CONTROL      0x04    /* Control register */
#define MSGDMA_CSR_RW_FILL      0x08    /* R/W fill level */
#define MSGDMA_CSR_RESP_FILL    0x0C    /* Response fill level */
#define MSGDMA_CSR_RW_SEQ       0x10    /* R/W sequence number */

/* CSR Status bits */
#define MSGDMA_CSR_BUSY            (1 << 0)
#define MSGDMA_CSR_DESC_EMPTY      (1 << 1)
#define MSGDMA_CSR_RESETTING       (1 << 6)
#define MSGDMA_CSR_IRQ             (1 << 9)

/* CSR Control bits */
#define MSGDMA_CSR_STOP            (1 << 0)
#define MSGDMA_CSR_RESET           (1 << 1)
#define MSGDMA_CSR_STOP_DESC       (1 << 2)
#define MSGDMA_CSR_STOP_ERR        (1 << 3)
#define MSGDMA_CSR_IRQ_EN          (1 << 4)
#define MSGDMA_CSR_STOP_EARLY      (1 << 5)
#define MSGDMA_CSR_GLOBAL_IRQ_EN   (1 << 4)

/* mSGDMA Standard Descriptor (no prefetcher) */
/* MM→ST (write to FPGA): read_addr + length */
#define MSGDMA_DESC_READ_ADDR   0x00
#define MSGDMA_DESC_WRITE_ADDR  0x04
#define MSGDMA_DESC_LENGTH      0x08
#define MSGDMA_DESC_CONTROL     0x0C

/* Descriptor control bits */
#define MSGDMA_DESC_CTL_GO              (1 << 31)
#define MSGDMA_DESC_CTL_EARLY_DONE_EN   (1 << 24)
#define MSGDMA_DESC_CTL_GENERATE_SOP    (1 << 8)
#define MSGDMA_DESC_CTL_GENERATE_EOP    (1 << 9)
#define MSGDMA_DESC_CTL_PARK_READS      (1 << 10)
#define MSGDMA_DESC_CTL_PARK_WRITES     (1 << 11)
#define MSGDMA_DESC_CTL_END_ON_EOP      (1 << 12)
#define MSGDMA_DESC_CTL_TX_CHANNEL(ch)  ((ch) << 16)
#define MSGDMA_DESC_CTL_OWNED_BY_HW     (1 << 30)

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
    volatile uint32_t *dma_write_csr;   /* DMA write CSR (MM→ST) */
    volatile uint32_t *dma_write_desc;  /* DMA write descriptor */
    volatile uint32_t *dma_read_csr;    /* DMA read CSR (ST→MM) */
    volatile uint32_t *dma_read_desc;   /* DMA read descriptor */
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

/* ---- DMA Transfers ---- */

/** Reset a DMA engine (CSR pointer) */
void neurax_dma_reset(volatile uint32_t *dma_csr);

/**
 * DMA: Transfer data from HPS memory to FPGA (input path).
 * Uses DMA_neurax_write (MM→ST).
 *
 * @param bsp       BSP context
 * @param phys_addr Physical address of source buffer in HPS DDR
 * @param length    Transfer length in bytes
 * @return 0 on success
 */
int neurax_dma_send(neurax_bsp_t *bsp, uint32_t phys_addr, uint32_t length);

/**
 * DMA: Transfer data from FPGA to HPS memory (output path).
 * Uses DMA_neurax_read (ST→MM).
 *
 * @param bsp       BSP context
 * @param phys_addr Physical address of destination buffer in HPS DDR
 * @param length    Transfer length in bytes
 * @return 0 on success
 */
int neurax_dma_recv(neurax_bsp_t *bsp, uint32_t phys_addr, uint32_t length);

/** Wait for DMA write (MM→ST) to complete */
int neurax_dma_send_wait(neurax_bsp_t *bsp);

/** Wait for DMA read (ST→MM) to complete */
int neurax_dma_recv_wait(neurax_bsp_t *bsp);

/**
 * Full convolution cycle:
 *   1. DMA send input+weights+bias to FPGA
 *   2. Start convolution
 *   3. Wait for done
 *   4. DMA recv results from FPGA
 *
 * @param bsp           BSP context
 * @param cfg           Convolution config
 * @param input_phys    Physical address of input data
 * @param input_bytes   Input data size in bytes
 * @param output_phys   Physical address of output buffer
 * @param output_bytes  Expected output size in bytes
 * @return 0 on success
 */
int neurax_run_conv(neurax_bsp_t *bsp,
                    const neurax_conv_config_t *cfg,
                    uint32_t input_phys, uint32_t input_bytes,
                    uint32_t output_phys, uint32_t output_bytes);

#ifdef __cplusplus
}
#endif

#endif /* NEURAX_BSP_H */
