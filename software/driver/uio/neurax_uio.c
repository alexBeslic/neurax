// SPDX-License-Identifier: GPL-2.0
/*
 * neurax_uio.c — Minimal UIO driver for Neurax mSGDMA on Cyclone V SoC
 *
 * Allocates DMA-coherent TX/RX buffers and exposes them to userspace via
 * the UIO framework. Userspace programs the mSGDMA descriptors directly
 * using the physical buffer addresses published in sysfs.
 *
 * UIO memory regions (Linux 5.4+: offset = region_index × PAGE_SIZE):
 *   /dev/uio0, offset 0: TX DMA buffer  (HPS → FPGA, mSGDMA m2s)
 *   /dev/uio0, offset 1: RX DMA buffer  (FPGA → HPS, mSGDMA s2m)
 *
 * DMA buffer physical addresses (feed into mSGDMA descriptor fields):
 *   /sys/class/uio/uio0/maps/map0/addr  ← TX buffer phys addr (read_addr for m2s)
 *   /sys/class/uio/uio0/maps/map1/addr  ← RX buffer phys addr (write_addr for s2m)
 *
 * mSGDMA CSR registers and Neurax control registers are in the LW-H2F bridge
 * (0xFF200000). Userspace accesses them via /dev/mem exactly as neurax_bsp does.
 *   0xFF200000+0x000: Neurax control register block
 *   0xFF200000+0x040: mSGDMA0 (m2s, HPS→FPGA write path)  CSR + descriptor
 *   0xFF200000+0x080: mSGDMA1 (s2m, FPGA→HPS read  path)  CSR + descriptor
 *
 * Device tree binding: compatible = "neurax,msgdma,main"
 * The sibling node compatible = "neurax,msgdma" is mSGDMA1 at base+0x40.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/of.h>

/* DMA buffer size — must match kernel msgdma.c and FPGA RAM size */
#define DMA_BUF_SIZE        (1 << 20)   /* 1 MB per buffer */

/* HPS SDRAM controller — FPGA port reset register.
 * Bit 1 = port enabled; 0 = port held in reset.
 * 0x311 covers all used F2SDRAM ports per hps_isw_handoff/hps.xml:
 *   bit 0  : f2h_sdram0 read  port (m2s master, write path)
 *   bit 4  : f2h_sdram1 write port (s2m master, read  path)
 *   bits 8-9: command ports */
#define SDR_CTL_BASE            0xFFC25000UL
#define SDR_CTL_FPGAPORTRST     0x80
#define FPGAPORTRST_F2SDRAM_ALL 0x311u

/* mSGDMA CSR register offsets (from instance base) */
#define MSGDMA_CSR_STATUS   0x00
#define MSGDMA_CSR_CTRL     0x04
/* CSR status/control bits */
#define CSR_BUSY            BIT(0)
#define CSR_RESETTING       BIT(6)
#define CSR_RESET_DISP      BIT(1)

/* Offset from mSGDMA0 base to mSGDMA1 (adjacent instances in LW bridge) */
#define MSGDMA1_OFFSET      0x40

struct neurax_uio_priv {
    struct uio_info  uio;
    struct device   *dev;
    void            *dma_tx_virt;   /* kernel VA of TX buffer (HPS→FPGA) */
    dma_addr_t       dma_tx_phys;   /* physical/DMA addr of TX buffer     */
    void            *dma_rx_virt;   /* kernel VA of RX buffer (FPGA→HPS) */
    dma_addr_t       dma_rx_phys;   /* physical/DMA addr of RX buffer     */
};

/* Reset a single mSGDMA instance. Mirrors the kernel msgdma_reset() sequence. */
static void msgdma_hw_reset(void __iomem *base)
{
    iowrite32(0x3FF, base + MSGDMA_CSR_STATUS);   /* clear all w1c status bits */
    iowrite32(ioread32(base + MSGDMA_CSR_CTRL) | CSR_RESET_DISP,
              base + MSGDMA_CSR_CTRL);
    while (ioread32(base + MSGDMA_CSR_STATUS) & CSR_RESETTING)
        cpu_relax();
    iowrite32(0x3FF, base + MSGDMA_CSR_STATUS);   /* clear bits set during reset */
}

/*
 * Custom mmap: use dma_mmap_coherent() for architecturally-correct page
 * protection (non-cacheable/device on ARM) for DMA coherent buffers.
 *
 * Linux 5.4+: UIO sets vma->vm_pgoff = region_index (0 or 1) before calling
 * info->mmap(). We reset it to 0 so dma_mmap_coherent maps the full buffer.
 */
static int neurax_uio_mmap(struct uio_info *info, struct vm_area_struct *vma)
{
    struct neurax_uio_priv *priv =
        container_of(info, struct neurax_uio_priv, uio);
    int mi = (int)vma->vm_pgoff;

    vma->vm_pgoff = 0;

    switch (mi) {
    case 0:
        return dma_mmap_coherent(priv->dev, vma,
                                 priv->dma_tx_virt,
                                 priv->dma_tx_phys,
                                 DMA_BUF_SIZE);
    case 1:
        return dma_mmap_coherent(priv->dev, vma,
                                 priv->dma_rx_virt,
                                 priv->dma_rx_phys,
                                 DMA_BUF_SIZE);
    default:
        return -EINVAL;
    }
}

static int neurax_uio_probe(struct platform_device *pdev)
{
    struct neurax_uio_priv *priv;
    struct resource *res;
    struct device *dev = &pdev->dev;
    void __iomem *sdr, *dma0, *dma1;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = dev;
    platform_set_drvdata(pdev, priv);

    /* Enable all F2SDRAM bridge ports so mSGDMA masters can reach HPS SDRAM.
     * Without this, the m2s master stalls with BUSY=1 on every write. */
    sdr = ioremap(SDR_CTL_BASE, 0x100);
    if (sdr) {
        u32 v = readl(sdr + SDR_CTL_FPGAPORTRST);
        dev_info(dev, "FPGAPORTRST before: 0x%08x", v);
        writel(v | FPGAPORTRST_F2SDRAM_ALL, sdr + SDR_CTL_FPGAPORTRST);
        dev_info(dev, "FPGAPORTRST after:  0x%08x",
                 readl(sdr + SDR_CTL_FPGAPORTRST));
        iounmap(sdr);
    } else {
        dev_warn(dev, "cannot map HPS SDR controller — F2SDRAM bridges may be disabled");
    }

    /* Allocate DMA-coherent buffers (physical address = DMA address on Cyclone V) */
    ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
    if (ret) {
        dev_err(dev, "dma_set_coherent_mask failed: %d", ret);
        return ret;
    }

    priv->dma_tx_virt = dma_alloc_coherent(dev, DMA_BUF_SIZE,
                                            &priv->dma_tx_phys, GFP_KERNEL);
    if (!priv->dma_tx_virt) {
        dev_err(dev, "failed to alloc TX DMA buffer");
        return -ENOMEM;
    }

    priv->dma_rx_virt = dma_alloc_coherent(dev, DMA_BUF_SIZE,
                                            &priv->dma_rx_phys, GFP_KERNEL);
    if (!priv->dma_rx_virt) {
        dev_err(dev, "failed to alloc RX DMA buffer");
        ret = -ENOMEM;
        goto err_free_tx;
    }

    dev_info(dev, "TX DMA buf: virt=%px phys=0x%08x", priv->dma_tx_virt, (u32)priv->dma_tx_phys);
    dev_info(dev, "RX DMA buf: virt=%px phys=0x%08x", priv->dma_rx_virt, (u32)priv->dma_rx_phys);

    /* Reset both mSGDMA instances before handing off to userspace.
     * Resource 0 from DT is the mSGDMA0 (m2s) base; mSGDMA1 (s2m) is at +0x40. */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res) {
        dma0 = ioremap(res->start, 0x40);
        dma1 = ioremap(res->start + MSGDMA1_OFFSET, 0x40);
        if (dma0 && dma1) {
            msgdma_hw_reset(dma0);
            msgdma_hw_reset(dma1);
            dev_info(dev, "mSGDMA0 reset at 0x%08x, mSGDMA1 at 0x%08x",
                     (u32)res->start, (u32)(res->start + MSGDMA1_OFFSET));
        }
        if (dma0) iounmap(dma0);
        if (dma1) iounmap(dma1);
    } else {
        dev_warn(dev, "no MEM resource in DT — mSGDMAs not reset");
    }

    /* UIO device setup */
    priv->uio.name    = "neurax-msgdma";
    priv->uio.version = "1.0";
    priv->uio.mmap    = neurax_uio_mmap;
    priv->uio.irq     = UIO_IRQ_NONE;  /* poll-based prototype, no IRQ needed */

    /* mem[0]: TX buffer — userspace fills this, mSGDMA m2s streams to FPGA.
     * Physical address published at /sys/class/uio/uio0/maps/map0/addr. */
    priv->uio.mem[0].name    = "dma-tx";
    priv->uio.mem[0].addr    = priv->dma_tx_phys;
    priv->uio.mem[0].size    = DMA_BUF_SIZE;
    priv->uio.mem[0].memtype = UIO_MEM_PHYS;

    /* mem[1]: RX buffer — mSGDMA s2m streams from FPGA into this, userspace reads.
     * Physical address published at /sys/class/uio/uio0/maps/map1/addr. */
    priv->uio.mem[1].name    = "dma-rx";
    priv->uio.mem[1].addr    = priv->dma_rx_phys;
    priv->uio.mem[1].size    = DMA_BUF_SIZE;
    priv->uio.mem[1].memtype = UIO_MEM_PHYS;

    ret = uio_register_device(dev, &priv->uio);
    if (ret) {
        dev_err(dev, "uio_register_device failed: %d", ret);
        goto err_free_rx;
    }

    dev_info(dev, "neurax UIO ready — TX phys=0x%08x  RX phys=0x%08x",
             (u32)priv->dma_tx_phys, (u32)priv->dma_rx_phys);
    return 0;

err_free_rx:
    dma_free_coherent(dev, DMA_BUF_SIZE, priv->dma_rx_virt, priv->dma_rx_phys);
err_free_tx:
    dma_free_coherent(dev, DMA_BUF_SIZE, priv->dma_tx_virt, priv->dma_tx_phys);
    return ret;
}

static void neurax_uio_remove(struct platform_device *pdev)
{
    struct neurax_uio_priv *priv = platform_get_drvdata(pdev);

    uio_unregister_device(&priv->uio);
    dma_free_coherent(priv->dev, DMA_BUF_SIZE,
                      priv->dma_rx_virt, priv->dma_rx_phys);
    dma_free_coherent(priv->dev, DMA_BUF_SIZE,
                      priv->dma_tx_virt, priv->dma_tx_phys);
}

static const struct of_device_id neurax_uio_of_match[] = {
    { .compatible = "neurax,msgdma,main" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, neurax_uio_of_match);

static struct platform_driver neurax_uio_driver = {
    .probe  = neurax_uio_probe,
    .remove = neurax_uio_remove,
    .driver = {
        .name           = "neurax-uio",
        .of_match_table = neurax_uio_of_match,
    },
};
module_platform_driver(neurax_uio_driver);

MODULE_DESCRIPTION("Neurax mSGDMA UIO driver for Cyclone V SoC");
MODULE_AUTHOR("ReDS");
MODULE_LICENSE("GPL v2");
