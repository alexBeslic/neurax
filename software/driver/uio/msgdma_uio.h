/*
 * msgdma_uio.h — Userspace mSGDMA register definitions for Neurax / Cyclone V
 *
 * Both mSGDMA instances live in the LW-H2F bridge page at 0xFF200000.
 * Map the full bridge (or at least one page) via /dev/mem, then use these
 * offsets to reach the CSR and descriptor registers of each instance:
 *
 *   volatile struct msgdma_reg *m2s = (void *)((char *)lw_base + MSGDMA0_OFFSET);
 *   volatile struct msgdma_reg *s2m = (void *)((char *)lw_base + MSGDMA1_OFFSET);
 *
 * Descriptor programming sequence (poll-based):
 *   1. Write read_addr / write_addr / len
 *   2. Memory barrier (__sync_synchronize)
 *   3. Write desc_ctrl | GO   ← commits descriptor, DMA starts
 *   4. Poll csr_status & BUSY until 0
 */

#ifndef MSGDMA_UIO_H
#define MSGDMA_UIO_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * LW-H2F bridge layout (byte offsets from base 0xFF200000)
 * ------------------------------------------------------------------------- */
#define LW_BRIDGE_BASE      0xFF200000u
#define LW_BRIDGE_SPAN      0x00200000u   /* 2 MB — full LW bridge range */

#define NEURAX_REG_OFFSET   0x000u        /* Neurax control register block  */
#define MSGDMA0_OFFSET      0x040u        /* mSGDMA0: m2s (HPS→FPGA write)  */
#define MSGDMA1_OFFSET      0x080u        /* mSGDMA1: s2m (FPGA→HPS read)   */

/* ---------------------------------------------------------------------------
 * mSGDMA register layout (standard 4-word descriptor slave, no extended)
 * Each instance is 0x30 bytes; fits within the 0x40-byte slot in LW bridge.
 * ------------------------------------------------------------------------- */
struct msgdma_reg {
    /* CSR port (0x00–0x1F) */
    volatile uint32_t csr_status;       /* 0x00  R/W1C  status flags       */
    volatile uint32_t csr_ctrl;         /* 0x04  R/W    control flags       */
    volatile uint32_t csr_fill_lvl;     /* 0x08  RO     descriptor fill     */
    volatile uint32_t csr_resp_fill;    /* 0x0C  RO     response fill       */
    volatile uint32_t csr_seq_num;      /* 0x10  RO     sequence numbers    */
    volatile uint32_t csr_cfg1;         /* 0x14  RO     component config 1  */
    volatile uint32_t csr_cfg2;         /* 0x18  RO     component config 2  */
    volatile uint32_t csr_info;         /* 0x1C  RO     component info      */

    /* Descriptor slave port (0x20–0x2F) */
    volatile uint32_t desc_read_addr;   /* 0x20  WO  source address (m2s)   */
    volatile uint32_t desc_write_addr;  /* 0x24  WO  dest   address (s2m)   */
    volatile uint32_t desc_len;         /* 0x28  WO  transfer length bytes  */
    volatile uint32_t desc_ctrl;        /* 0x2C  WO  control + GO bit       */
};

/* ---------------------------------------------------------------------------
 * CSR Status register bits (write-1-to-clear)
 * ------------------------------------------------------------------------- */
#define CSR_ST_BUSY             (1u << 0)
#define CSR_ST_DESC_BUF_EMPTY   (1u << 1)
#define CSR_ST_DESC_BUF_FULL    (1u << 2)
#define CSR_ST_RESP_BUF_EMPTY   (1u << 3)
#define CSR_ST_RESP_BUF_FULL    (1u << 4)
#define CSR_ST_STOPPED          (1u << 5)
#define CSR_ST_RESETTING        (1u << 6)
#define CSR_ST_STOPPED_ON_ERR   (1u << 7)
#define CSR_ST_STOPPED_EOP      (1u << 8)
#define CSR_ST_IRQ              (1u << 9)
#define CSR_ST_CLEAR_ALL        0x3FFu

/* ---------------------------------------------------------------------------
 * CSR Control register bits
 * ------------------------------------------------------------------------- */
#define CSR_CT_STOP_DISP        (1u << 0)
#define CSR_CT_RESET_DISP       (1u << 1)
#define CSR_CT_STOP_ON_ERR      (1u << 2)
#define CSR_CT_STOP_ON_EOP      (1u << 3)
#define CSR_CT_GLOBAL_IRQ_EN    (1u << 4)
#define CSR_CT_STOP_DESCR       (1u << 5)

/* ---------------------------------------------------------------------------
 * Descriptor Control word bits
 * ------------------------------------------------------------------------- */
#define DESC_GO                 (1u << 31)  /* commit descriptor, start DMA  */
#define DESC_GEN_SOP            (1u << 8)
#define DESC_GEN_EOP            (1u << 9)
#define DESC_PARK_RD            (1u << 10)
#define DESC_PARK_WR            (1u << 11)
#define DESC_END_ON_EOP         (1u << 12)
#define DESC_IRQ_ON_COMPLETE    (1u << 13)
#define DESC_IRQ_ON_EARLY_TERM  (1u << 14)
#define DESC_IRQ_ON_ERR         (1u << 15)
#define DESC_EARLY_DONE         (1u << 23)

/* Avalon-ST channel field [22:16].  Channel=1 signals start-of-frame to
 * the FPGA neurax_data_interface, resetting write_index and buffer_full. */
#define DESC_CHAN(n)            (((uint32_t)(n) & 0x7Fu) << 16)

/* ---------------------------------------------------------------------------
 * Buffer / transfer constants (must match kernel driver and FPGA generics)
 * ------------------------------------------------------------------------- */
#define DMA_BUF_SIZE        (1u << 20)          /* 1 MB per TX/RX buffer    */

/* neurax_data_interface output RAM size: g_RAM_SIZE = 23000 × 32-bit words */
#define FPGA_OUTPUT_SIZE    (23000u * sizeof(uint32_t))

/* Timeouts (microseconds) */
#define SOF_TIMEOUT_US      200000u  /* 200 ms: SOF should complete quickly  */
#define DMA_TIMEOUT_US      5000000u /* 5 s:    full transfer timeout        */

/* ---------------------------------------------------------------------------
 * Inline register helpers
 * (use volatile struct msgdma_reg * pointer obtained from mmap)
 * ------------------------------------------------------------------------- */

static inline uint32_t msgdma_rd(volatile struct msgdma_reg *r, uint32_t mask)
{
    return r->csr_status & mask;
}

static inline void msgdma_reset(volatile struct msgdma_reg *r)
{
    r->csr_status = CSR_ST_CLEAR_ALL;
    r->csr_ctrl = CSR_CT_RESET_DISP;

    while (r->csr_status & CSR_ST_RESETTING)
    {
        #if defined(__arm__) || defined(__aarch64__)
            __asm__ volatile("yield" ::: "memory");
        #endif
    }

    r->csr_ctrl = 0;

    r->csr_status = CSR_ST_CLEAR_ALL;
}

/*
 * Push one standard (4-word) descriptor to an mSGDMA instance.
 *
 * m2s (HPS→FPGA): rd_addr = TX buffer phys, wr_addr = 0
 * s2m (FPGA→HPS): rd_addr = 0,              wr_addr = RX buffer phys
 *
 * __sync_synchronize() is the ARM-userspace equivalent of wmb(): ensures
 * all address/length writes are globally visible before the GO write.
 */
static inline void msgdma_push_descr(volatile struct msgdma_reg *r,
                                     uint32_t rd_addr,
                                     uint32_t wr_addr,
                                     uint32_t len,
                                     uint32_t ctrl)
{
    r->desc_read_addr  = rd_addr;
    r->desc_write_addr = wr_addr;
    r->desc_len        = len;
    
    __sync_synchronize();
    
    r->desc_ctrl       = ctrl | DESC_GO;
}

#endif /* MSGDMA_UIO_H */
