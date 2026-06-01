#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <asm/io.h>

#define DEV_NAME            "msgdma"

#define MSGDMA_MAP_SIZE         0x30
#define MSGDMA1_OFFSET          0x40

#define MSGDMA_MAX_TX_LEN       (1 << 12) // 4KB
#define DMA_BUF_SIZE            (1 << 20) //1MB

#define TX_TIMEOUT              (5 * HZ)   /* 5 s: full DMA transfer timeout */
#define SOF_TIMEOUT             (HZ / 5)   /* 200 ms: channel=1 SOF should complete near-instantly */

/* FPGA neurax_data_interface output buffer size.
 * Must match g_RAM_SIZE in neurax_data_interface.vhd (23000 32-bit words).
 * The FPGA streams exactly this many bytes per read cycle; used for EOF tracking. */
#define FPGA_OUTPUT_SIZE        (23000u * sizeof(u32))

/* HPS SDRAM Controller FPGA port reset register (altr,sdr-ctl at 0xFFC25000).
 * Writing a 1 to a bit releases that F2SDRAM port from reset (enables it).
 * Writing 0 holds the port in reset (no access to HPS SDRAM from that FPGA master).
 *
 * Port assignments (from Qsys .sopcinfo and hps_isw_handoff/hps.xml):
 *   Bit 0  = F2SDRAM read  port 0  (hps_0_bridges.f2h_sdram0_data)
 *              → DMA_neurax_read m2s Avalon MM master (WRITE path to FPGA)
 *   Bit 4  = F2SDRAM write port 0  (hps_0_bridges.f2h_sdram1_data)
 *              → DMA_neurax_write s2m Avalon MM master (READ path from FPGA)
 *   Bits 8-9 = F2SDRAM command ports 0-1
 *
 * F2SDRAM_RESET_PORT_USED = 0x311 from hps_isw_handoff/soc_system_hps_0/hps.xml.
 * U-Boot often enables only the write port (bit 4), leaving read port 0 (bit 0)
 * in reset → the m2s DMA stalls on every SDRAM read → write never completes. */
#define SDR_CTL_BASE            0xFFC25000UL
#define SDR_CTL_FPGAPORTRST     0x80
#define FPGAPORTRST_F2SDRAM_ALL 0x311u

typedef u32 volatile reg_t;

#pragma pack(1)
struct msgdma_reg {
    /* CSR port Registers */
    reg_t csr_status;
    reg_t csr_ctrl;
    reg_t csr_fill_lvl;
    reg_t csr_resp_fill_lvl;
    reg_t csr_seq_num;
    reg_t csr_comp_config1;
    reg_t csr_comp_config2;
    reg_t csr_comp_info;

    /* Descriptor Slave port registers (standard 4-word format, 0x10 bytes).
     * m2s: read_addr=HPS source, write_addr=0 (stream output, ignored)
     * s2m: read_addr=0 (stream input, ignored), write_addr=HPS destination
     * Writing desc_ctrl with GO=1 commits the descriptor to the FIFO. */
    reg_t desc_read_addr;
    reg_t desc_write_addr;
    reg_t desc_len;
    reg_t desc_ctrl;
};
#pragma pack()

/* MSGDMA Register bit fields */
enum STATUS {
    IRQ                 = (1 << 9),
    STOPPED_EARLY_TERM  = (1 << 8),
    STOPPED_ON_ERR      = (1 << 7),
    RESETTING           = (1 << 6),
    STOPPED             = (1 << 5),
    RESP_BUF_FULL       = (1 << 4),
    RESP_BUF_EMPTY      = (1 << 3),
    DESCR_BUF_FULL      = (1 << 2),
    DESCR_BUF_EMTPY     = (1 << 1),
    BUSY                = (1 << 0),
};

enum CONTROL {
    STOP_DESCR          = (1 << 5),
    GLOBAL_INT_EN_MASK  = (1 << 4),
    STOP_ON_EARLY_TERM  = (1 << 3),
    STOP_ON_ERROR       = (1 << 2),
    RESET_DISPATCHER    = (1 << 1),
    STOP_DISPATCHER     = (1 << 0),
};

enum DESC_CTRL {
    GO                  = (1 << 31),
    EARLY_DONE_EN       = (1 << 23),   /* bit 23 per Altera mSGDMA spec */
    TX_ERR_IRQ_EN       = (1 << 15),   /* bit 15 */
    EARLY_TERM_IRQ_EN   = (1 << 14),   /* bit 14 */
    TX_COMPLETE_IRQ_EN  = (1 << 13),   /* bit 13 */
    END_ON_EOP          = (1 << 12),
    PARK_WR             = (1 << 11),
    PARK_RD             = (1 << 10),
    GEN_EOP             = (1 << 9),
    GEN_SOP             = (1 << 8),
};

/* TX channel field [22:16] of descriptor control word.
 * Channel=1 is used as a start-of-frame signal to the FPGA data interface:
 * the FPGA resets write_index and clears buffer_full when asi_channel_i=1. */
#define TX_CHAN(n) (((n) & 0x7fu) << 16)

/* Driver private data */
struct msgdma_data {
    dev_t dev_id;
    struct cdev cdev;
    struct class *dev_class;
    struct device *dev_device;

    struct msgdma_reg *msgdma0_reg;
    struct msgdma_reg *msgdma1_reg;

    int msgdma0_irq;
    int msgdma1_irq;

    void *dma_buf_wr;
    void *dma_buf_rd;
    dma_addr_t dma_buf_wr_handle;
    dma_addr_t dma_buf_rd_handle;

    wait_queue_head_t wr_complete_wq;
    wait_queue_head_t rd_complete_wq;
    int wr_in_progress;
    int rd_in_progress;
};

/* Function declarations */
static int
msgdma_open(struct inode *node, struct file *f);
static int
msgdma_release(struct inode *node, struct file *f);
static ssize_t
msgdma_read(struct file *f, char __user *ubuf, size_t len, loff_t *off);
static ssize_t
msgdma_write(struct file *f, const char __user *ubuf, size_t len, loff_t *off);

static int
msgdma_probe(struct platform_device *pdev);
static void
msgdma_remove(struct platform_device *pdev);

static const struct file_operations msgdma_fops = {
    .owner      = THIS_MODULE,
    .open       = msgdma_open,
    .release    = msgdma_release,
    .read       = msgdma_read,
    .write      = msgdma_write
};

static const struct of_device_id msgdma_of_match[] = {
    {.compatible = "neurax,msgdma,main" },
    {}
};

static struct platform_driver msgdma_driver = {
    .probe = msgdma_probe,
    .remove = msgdma_remove,
    .driver = {
        .name = DEV_NAME,
        .of_match_table = msgdma_of_match,
    },
};
