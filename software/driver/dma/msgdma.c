#include "msgdma.h"

/* Utilitary functions */
static void setbit_reg32(volatile void __iomem *reg, u32 mask) {
	u32 val = ioread32(reg);

	iowrite32(val | mask, reg);
}

static void clearbit_reg32(volatile void __iomem *reg, u32 mask) {
	u32 val = ioread32(reg);

	iowrite32((val & (~mask)), reg);
}

static void
msgdma_reset(struct msgdma_reg *reg)
{
    /* Clear all status bits (write-1-to-clear), then reset, then clear again
     * This matches the official Altera mSGDMA driver reset sequence. */
    iowrite32(0x3FF, &reg->csr_status); /* clear all 10 status bits */
    setbit_reg32(&reg->csr_ctrl, RESET_DISPATCHER);
    while(ioread32(&reg->csr_status) & RESETTING);
    iowrite32(0x3FF, &reg->csr_status); /* clear any bits set during reset */
}

/* Push a descriptor using the standard 4-word format.
 * m2s (write to FPGA): rd_addr=HPS source, wr_addr=0
 * s2m (read from FPGA): rd_addr=0, wr_addr=HPS destination
 * Writing desc_ctrl with GO=1 commits the descriptor. */
static void
msgdma_push_descr(
    struct msgdma_reg *reg,
    dma_addr_t rd_addr,
    dma_addr_t wr_addr,
    u32 len,
    u32 ctrl)
{
    iowrite32(rd_addr, &reg->desc_read_addr);
    iowrite32(wr_addr, &reg->desc_write_addr);
    iowrite32(len,     &reg->desc_len);
    iowrite32(ctrl | GO, &reg->desc_ctrl);
}

static int
msgdma_open(struct inode *node, struct file *f) 
{
    // TODO : protect single openness
    struct msgdma_data *data;

    data = container_of(node->i_cdev, struct msgdma_data, cdev);
    f->private_data = data;

    return 0;
}

static int
msgdma_release(struct inode *node, struct file *f) 
{
    return 0;
}

static ssize_t
msgdma_write(struct file *f, const char __user *ubuf, size_t len, loff_t *off)
{
    struct msgdma_data *data;
    ssize_t write_ret;
    u32 status, fill, resp_fill;

    data = (struct msgdma_data*)f->private_data;

    write_ret = len > DMA_BUF_SIZE ? DMA_BUF_SIZE : len;

    /* Copy all user data to DMA buffer first */
    if(copy_from_user(data->dma_buf_wr, ubuf, write_ret) != 0)
        return -EFAULT;

    /* Push single descriptor for entire transfer */
    data->wr_in_progress = 1;
    /* m2s: read from HPS SDRAM, stream to FPGA sink. write_addr=0 (stream output). */
    msgdma_push_descr(
        data->msgdma0_reg,
        data->dma_buf_wr_handle,
        0,
        write_ret,
        TX_COMPLETE_IRQ_EN);

    /* Log post-push state.
     * fill_lvl bits 15:0 = read-side descriptor FIFO count (m2s).
     * fill_lvl bits 31:16 = write-side descriptor FIFO count (s2m).
     * For m2s: expect fill_rd=1 if descriptor accepted, or BUSY=1 if already executing. */
    status = ioread32(&data->msgdma0_reg->csr_status);
    fill   = ioread32(&data->msgdma0_reg->csr_fill_lvl);
    pr_info("msgdma write: pushed CSR=0x%08x fill_rd=%u fill_wr=%u dma=0x%08x len=%zd\n",
            status, fill & 0xffff, fill >> 16, (u32)data->dma_buf_wr_handle, write_ret);

    /* Poll for completion — decoupled from IRQ to isolate routing issues */
    {
        unsigned long deadline = jiffies + TX_TIMEOUT;
        while (time_before(jiffies, deadline)) {
            status = ioread32(&data->msgdma0_reg->csr_status);
            if (!(status & BUSY))
                break;
            msleep(1);
        }
    }
    data->wr_in_progress = 0;

    if (status & BUSY) {
        /* Timeout — DMA still BUSY after TX_TIMEOUT (stream stall) */
        msgdma_reset(data->msgdma0_reg);
        fill      = ioread32(&data->msgdma0_reg->csr_fill_lvl);
        resp_fill = ioread32(&data->msgdma0_reg->csr_resp_fill_lvl);
        pr_err("msgdma write timeout (stream stall)! CSR=0x%08x fill=%u resp_fill=%u dma=0x%08x len=%zu\n",
               status, fill, resp_fill, (u32)data->dma_buf_wr_handle, len);
        return -EIO;
    }
    if (status & (STOPPED_ON_ERR | STOPPED_EARLY_TERM)) {
        /* DMA stopped due to bus error (e.g. f2sdram bridge disabled) or early termination */
        fill      = ioread32(&data->msgdma0_reg->csr_fill_lvl);
        resp_fill = ioread32(&data->msgdma0_reg->csr_resp_fill_lvl);
        pr_err("msgdma write stopped (bus error?)! CSR=0x%08x fill=%u resp_fill=%u dma=0x%08x len=%zu\n",
               status, fill, resp_fill, (u32)data->dma_buf_wr_handle, len);
        msgdma_reset(data->msgdma0_reg);
        return -EIO;
    }

    return write_ret;
}

static ssize_t
msgdma_read(struct file *f, char __user *ubuf, size_t len, loff_t *off)
{
    struct msgdma_data *data;
    ssize_t read_ret;
    u32 status, fill, resp_fill;

    data = (struct msgdma_data*)f->private_data;

    read_ret = len > DMA_BUF_SIZE ? DMA_BUF_SIZE : len;

    /* s2m: stream from FPGA source, write to HPS SDRAM. read_addr=0 (stream input). */
    data->rd_in_progress = 1;
    msgdma_push_descr(
        data->msgdma1_reg,
        0,
        data->dma_buf_rd_handle,
        read_ret,
        TX_COMPLETE_IRQ_EN
    );

    /* Log post-push state.
     * fill_lvl bits 15:0 = read-side descriptor FIFO count (m2s).
     * fill_lvl bits 31:16 = write-side descriptor FIFO count (s2m).
     * For s2m: expect fill_wr=1 if descriptor accepted, or BUSY=1 if already executing. */
    status = ioread32(&data->msgdma1_reg->csr_status);
    fill   = ioread32(&data->msgdma1_reg->csr_fill_lvl);
    pr_info("msgdma read: pushed CSR=0x%08x fill_rd=%u fill_wr=%u dma=0x%08x len=%zd\n",
            status, fill & 0xffff, fill >> 16, (u32)data->dma_buf_rd_handle, read_ret);

    /* Poll for completion — decoupled from IRQ to isolate routing issues */
    {
        unsigned long deadline = jiffies + TX_TIMEOUT;
        while (time_before(jiffies, deadline)) {
            status = ioread32(&data->msgdma1_reg->csr_status);
            if (!(status & BUSY))
                break;
            msleep(1);
        }
    }
    data->rd_in_progress = 0;

    if (status & BUSY) {
        /* Timeout — DMA still BUSY after TX_TIMEOUT (stream stall) */
        msgdma_reset(data->msgdma1_reg);
        fill      = ioread32(&data->msgdma1_reg->csr_fill_lvl);
        resp_fill = ioread32(&data->msgdma1_reg->csr_resp_fill_lvl);
        pr_err("msgdma read timeout (stream stall)! CSR=0x%08x fill=%u resp_fill=%u dma=0x%08x len=%zu\n",
               status, fill, resp_fill, (u32)data->dma_buf_rd_handle, len);
        return -EIO;
    }
    if (status & (STOPPED_ON_ERR | STOPPED_EARLY_TERM)) {
        /* DMA stopped due to bus error (e.g. f2sdram bridge disabled) or early termination */
        fill      = ioread32(&data->msgdma1_reg->csr_fill_lvl);
        resp_fill = ioread32(&data->msgdma1_reg->csr_resp_fill_lvl);
        pr_err("msgdma read stopped (bus error?)! CSR=0x%08x fill=%u resp_fill=%u dma=0x%08x len=%zu\n",
               status, fill, resp_fill, (u32)data->dma_buf_rd_handle, len);
        msgdma_reset(data->msgdma1_reg);
        return -EIO;
    }

    if(copy_to_user(ubuf, data->dma_buf_rd, read_ret) != 0)
        return -EFAULT;

    return read_ret;
}

static irqreturn_t 
msgdma_irq_handler(int irq, void *dev_id)
{
    struct msgdma_reg *msgdma0_reg;
    struct msgdma_reg *msgdma1_reg;

    struct msgdma_data *data = (struct msgdma_data*)dev_id;
    msgdma0_reg = data->msgdma0_reg;
    msgdma1_reg = data->msgdma1_reg;

    /* Acknowledge corresponding DMA, and wake up whoever is waiting */
    if(ioread32(&msgdma0_reg->csr_status) & IRQ) {
        setbit_reg32(&msgdma0_reg->csr_status, IRQ);
        data->wr_in_progress = 0;
        wake_up_interruptible(&data->wr_complete_wq);
    }

    if(ioread32(&msgdma1_reg->csr_status) & IRQ) {
        setbit_reg32(&msgdma1_reg->csr_status, IRQ);
        data->rd_in_progress = 0;
        wake_up_interruptible(&data->rd_complete_wq);
    }

    return IRQ_HANDLED;
}

static int 
msgdma_register_chrdev(struct msgdma_data *data)
{
    int ret = 0;

    ret = alloc_chrdev_region(&data->dev_id, 0, 1, DEV_NAME);
    if(ret < 0) {
        pr_err("Character device region allocation failed\n");
        goto _ret;
    }

    /* Actual registering of the device. At this point it must be 
     * fully initialized */
    cdev_init(&(data->cdev), &msgdma_fops);
    ret = cdev_add(&(data->cdev), data->dev_id, 1);
    if(ret < 0) {
        pr_err("Character device initialisation failed\n");
        goto _cdev_add_err;
    }

    data->dev_class = class_create(DEV_NAME);
    if(IS_ERR(data->dev_class)) {
        ret = PTR_ERR(data->dev_class);
        pr_err("Device class creation failed\n");
        goto _class_err;
    }

    data->dev_device = device_create(data->dev_class, NULL, data->dev_id, NULL, DEV_NAME);
    if(IS_ERR(data->dev_device)) {
        ret = PTR_ERR(data->dev_device);
        pr_err("Device creation failed\n");
        goto _device_err;
    }

    return 0;

_device_err:
    class_destroy(data->dev_class);

_class_err:
    cdev_del(&data->cdev);

_cdev_add_err:
    unregister_chrdev_region(data->dev_id, 1);

_ret:
    return ret;
}

static void
msgdma_unregister_chrdev(struct msgdma_data *data)
{
    device_destroy(data->dev_class, data->dev_id);
    class_destroy(data->dev_class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->dev_id, 1);
}

static int
msgdma_probe(struct platform_device *pdev)
{
    struct msgdma_data *data;
    struct resource *csr_res;
    struct device *dev;
    int ret = 0;

    dev = &pdev->dev;

    data = (struct msgdma_data*)devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);    
    if(data == NULL)
        return -ENOMEM;

    platform_set_drvdata(pdev, (void*)data);

    /* Prepare DMA buffers */
    dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));

    data->dma_buf_rd = dma_alloc_coherent(
        dev, 
        DMA_BUF_SIZE, 
        &data->dma_buf_rd_handle, 
        GFP_KERNEL);

    if(data->dma_buf_rd == NULL) {
        ret = -ENOMEM;
        goto fail;
    }

    data->dma_buf_wr = dma_alloc_coherent(
        dev, 
        DMA_BUF_SIZE, 
        &data->dma_buf_wr_handle, 
        GFP_KERNEL);

    if(data->dma_buf_wr == NULL) {
        ret = -ENOMEM;
        goto fail;
    }

    /* Map both CSR (0x20 bytes) and Descriptor slave (0x10 bytes) regions.
     * Device tree splits them as separate resources, but they're adjacent in HW.
     * CSR: resource 0
     * Descriptor: resource 1 (at CSR+0x20) */
    csr_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(!csr_res) {
        dev_err(dev, "CSR resource not found");
        return -ENODEV;
    }

    /* Probed node is neurax,msgdma,main = DMA_neurax_read at 0xff200040.
     * DMA_neurax_read  (0xff200040): memory-to-stream  → HPS SDRAM → FPGA ST Sink  (WRITE path)
     * DMA_neurax_write (0xff200080): stream-to-memory  → FPGA ST Source → HPS SDRAM (READ path)
     * Convention: msgdma0_reg = write (m2s), msgdma1_reg = read (s2m) */
    data->msgdma0_reg = devm_ioremap(dev, csr_res->start, MSGDMA_MAP_SIZE);
    if(!data->msgdma0_reg) {
        dev_err(dev, "failed to map msgdma0 (write, m2s) region");
        return -ENOMEM;
    }

    dev_info(dev, "msgdma0 (write m2s): mapped at %px (phys 0x%x)",
             data->msgdma0_reg, (u32)csr_res->start);

    /* Map read DMA (stream-to-memory) at write DMA base + 0x40 */
    {
        phys_addr_t msgdma1_phys = csr_res->start + MSGDMA1_OFFSET;
        data->msgdma1_reg = devm_ioremap(dev, msgdma1_phys, MSGDMA_MAP_SIZE);
        if(!data->msgdma1_reg) {
            dev_err(dev, "failed to map msgdma1 (read, s2m) region");
            return -ENOMEM;
        }
        dev_info(dev, "msgdma1 (read s2m): mapped at %px (phys 0x%x)",
                 data->msgdma1_reg, (u32)msgdma1_phys);
    }

    /* Initialize the device itself */
    msgdma_reset(data->msgdma0_reg);
    msgdma_reset(data->msgdma1_reg);

    setbit_reg32(&data->msgdma0_reg->csr_ctrl, 
        STOP_ON_EARLY_TERM | STOP_ON_ERROR | GLOBAL_INT_EN_MASK);
    setbit_reg32(&data->msgdma1_reg->csr_ctrl, 
        STOP_ON_EARLY_TERM | STOP_ON_ERROR | GLOBAL_INT_EN_MASK);

    /* Get WRITE DMA (m2s, 0xff200040) IRQ from probed node (neurax,msgdma,main) */
    data->msgdma0_irq = platform_get_irq(pdev, 0);
    if(data->msgdma0_irq < 0) {
        dev_err(dev, "could not get write DMA irq");
        return -ENXIO;
    }

    ret = devm_request_irq(dev, data->msgdma0_irq, msgdma_irq_handler, IRQF_SHARED, "msgdma_wr", data);
    if(ret < 0) {
        dev_err(dev, "Could not request irq %d", data->msgdma0_irq);
        return ret;
    }

    /* Get READ DMA (s2m, 0xff200080) IRQ from sibling node (neurax,msgdma) */
    {
        struct device_node *rd_node;
        rd_node = of_find_compatible_node(NULL, NULL, "neurax,msgdma");
        if(!rd_node) {
            dev_err(dev, "read DMA node not found");
            return -ENODEV;
        }
        data->msgdma1_irq = of_irq_get(rd_node, 0);
        of_node_put(rd_node);
    }
    if(data->msgdma1_irq < 0) {
        dev_err(dev, "could not get read DMA irq");
        return -ENXIO;
    }

    ret = devm_request_irq(dev, data->msgdma1_irq, msgdma_irq_handler, IRQF_SHARED, "msgdma_rd", data);
    if(ret < 0) {
        dev_err(dev, "Could not request irq %d", data->msgdma1_irq);
        return ret;
    }

    data->wr_in_progress = 0;
    data->rd_in_progress = 0;
    init_waitqueue_head(&data->rd_complete_wq);
    init_waitqueue_head(&data->wr_complete_wq);

    dev_info(dev, "write DMA buf: virt=%px phys=0x%08x", data->dma_buf_wr, (u32)data->dma_buf_wr_handle);
    dev_info(dev, "read  DMA buf: virt=%px phys=0x%08x", data->dma_buf_rd, (u32)data->dma_buf_rd_handle);
    dev_info(dev, "msgdma0 (wr) mapped at %px, msgdma1 (rd) mapped at %px", data->msgdma0_reg, data->msgdma1_reg);

    ret = msgdma_register_chrdev(data);
    if(ret < 0)
        return ret;

    return 0;

fail:
    msgdma_remove(pdev);

    return ret;
}

static void
msgdma_remove(struct platform_device *pdev)
{
    struct msgdma_data *data = (struct msgdma_data*)platform_get_drvdata(pdev);

    msgdma_unregister_chrdev(data);

    /* Stop any in-flight DMA before freeing buffers and unmapping IRQs.
     * Without this, a pending IRQ or active DMA could access freed memory. */
    if(data->msgdma0_reg)
        msgdma_reset(data->msgdma0_reg);
    if(data->msgdma1_reg)
        msgdma_reset(data->msgdma1_reg);

    dma_free_coherent(
        &pdev->dev, 
        DMA_BUF_SIZE, 
        data->dma_buf_rd, 
        data->dma_buf_rd_handle);
    dma_free_coherent(
        &pdev->dev, 
        DMA_BUF_SIZE, 
        data->dma_buf_wr, 
        data->dma_buf_wr_handle);

}

static int __init
msgdma_init(void)
{
    return platform_driver_register(&msgdma_driver);
}

static void __exit
msgdma_exit(void)
{
    platform_driver_unregister(&msgdma_driver);
}

subsys_initcall(msgdma_init);
module_exit(msgdma_exit);

MODULE_DESCRIPTION("MSGDMA (test) driver");
MODULE_AUTHOR("Sydney Hauke, ReDS");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");