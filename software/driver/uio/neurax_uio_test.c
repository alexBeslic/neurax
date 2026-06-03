/*
 * neurax_uio_test.c — Userspace DMA test for Neurax mSGDMA via UIO
 *
 * Replaces the /dev/msgdma kernel char-device path with direct mSGDMA
 * descriptor programming from userspace.
 *
 * Setup:
 *   insmod neurax_uio.ko          # loads the UIO kernel module
 *   ./neurax_uio_test             # runs write + read DMA test
 *
 * Access model:
 *   /dev/mem  → mmap LW bridge (0xFF200000) for mSGDMA and Neurax registers
 *   /dev/uio0 → mmap TX/RX DMA-coherent buffers
 *   sysfs     → read DMA buffer physical addresses for descriptor programming
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/mman.h>

#include "msgdma_uio.h"

/* ---------------------------------------------------------------------------
 * Neurax accelerator register map (LW bridge offset 0x000, same page as mSGDMAs)
 * Mirrors neurax_bsp.h — copied here to keep the test self-contained.
 * ------------------------------------------------------------------------- */
#define REG_CMD                 0
#define REG_STATUS              1
#define REG_CONV_CONFIG_0       3
#define REG_CONV_CONFIG_1       4
#define REG_BATCH_SIZE          8
#define REG_DEBUG_CYCLES       13

#define CMD_ENABLE              (1u << 0)
#define CMD_OP_CONV             (0u << 1)   /* OP_SELECT = 0 → convolution */
#define CMD_START               (1u << 3)

#define STATUS_DONE             (1u << 4)
#define STATUS_BUSY             (1u << 5)

/* RAM word layout (one 32-bit word per Q8.8 sample).
 * Must match FPGA generics g_FB_HEIGHT/g_FB_WIDTH and FPGA_accelerator offsets. */
#define RAM_INPUT_BASE   0u
#define RAM_WEIGHT_BASE  10000u
#define RAM_BIAS_BASE    13000u
#define RAM_OUTPUT_BASE  13016u
#define RAM_TOTAL_WORDS  23000u

/* Q8.8: 1.0 → 0x0100 */
#define Q8_8_ONE        0x00000100u
#define Q8_8_ZERO       0x00000000u

/* ---------------------------------------------------------------------------
 * Internal state
 * ------------------------------------------------------------------------- */
struct neurax_uio_ctx {
    /* /dev/mem mapping — covers full LW bridge page */
    int      fd_mem;
    void    *lw_base;               /* mmap base for LW bridge              */
    volatile struct msgdma_reg *m2s; /* mSGDMA0: HPS→FPGA (write path)      */
    volatile struct msgdma_reg *s2m; /* mSGDMA1: FPGA→HPS (read  path)      */
    volatile uint32_t          *neurax; /* Neurax control register block     */

    /* UIO mapping — DMA-coherent buffers */
    int      fd_uio;
    void    *tx_buf;                /* userspace VA of TX buffer             */
    void    *rx_buf;                /* userspace VA of RX buffer             */
    uint32_t tx_phys;               /* physical addr → m2s desc read_addr   */
    uint32_t rx_phys;               /* physical addr → s2m desc write_addr  */

    /* EOF tracking for neurax_read() */
    size_t   rx_offset;
};

/* ---------------------------------------------------------------------------
 * Timing helpers
 * ------------------------------------------------------------------------- */
static uint64_t now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000);
}

static void usleep_ms(int ms)
{
    struct timespec ts = { .tv_sec = ms / 1000,
                           .tv_nsec = (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

/* ---------------------------------------------------------------------------
 * UIO device discovery
 * Scans /sys/class/uio/ for an entry whose 'name' file matches 'target'.
 * Returns the UIO index (0, 1, …) or -1 on failure.
 * ------------------------------------------------------------------------- */
static int uio_find_by_name(const char *target)
{
    DIR *d = opendir("/sys/class/uio");
    if (!d) {
        perror("opendir /sys/class/uio");
        return -1;
    }

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strncmp(de->d_name, "uio", 3) != 0)
            continue;

        char path[512];
        snprintf(path, sizeof(path), "/sys/class/uio/%s/name", de->d_name);
        FILE *f = fopen(path, "r");
        if (!f) continue;

        char name[64] = {0};
        if (fgets(name, sizeof(name), f)) {
            /* strip trailing newline */
            size_t n = strlen(name);
            if (n > 0 && name[n-1] == '\n') name[n-1] = '\0';
            if (strcmp(name, target) == 0) {
                fclose(f);
                closedir(d);
                int idx = atoi(de->d_name + 3);
                return idx;
            }
        }
        fclose(f);
    }

    closedir(d);
    fprintf(stderr, "UIO device '%s' not found in /sys/class/uio\n", target);
    return -1;
}

/* Read a hex value from a UIO sysfs attribute (e.g. maps/map0/addr). */
static uint32_t uio_read_map_phys(int uio_idx, int map_idx)
{
    char path[256];
    snprintf(path, sizeof(path),
             "/sys/class/uio/uio%d/maps/map%d/addr", uio_idx, map_idx);
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
        return 0;
    }
    uint32_t addr = 0;
    fscanf(f, "0x%x", &addr);
    fclose(f);
    return addr;
}

/* ---------------------------------------------------------------------------
 * Init / deinit
 * ------------------------------------------------------------------------- */
int neurax_uio_init(struct neurax_uio_ctx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->fd_mem = -1;
    ctx->fd_uio = -1;

    /* --- /dev/mem: map the full LW-H2F bridge ----------------------------- */
    ctx->fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if (ctx->fd_mem < 0) {
        perror("open /dev/mem");
        return -1;
    }

    ctx->lw_base = mmap(NULL, LW_BRIDGE_SPAN,
                        PROT_READ | PROT_WRITE, MAP_SHARED,
                        ctx->fd_mem, LW_BRIDGE_BASE);
    if (ctx->lw_base == MAP_FAILED) {
        perror("mmap LW bridge");
        goto err;
    }

    ctx->neurax = (volatile uint32_t *)((char *)ctx->lw_base + NEURAX_REG_OFFSET);
    ctx->m2s    = (volatile struct msgdma_reg *)((char *)ctx->lw_base + MSGDMA0_OFFSET);
    ctx->s2m    = (volatile struct msgdma_reg *)((char *)ctx->lw_base + MSGDMA1_OFFSET);

    /* --- UIO: locate device, map TX/RX DMA buffers ----------------------- */
    int uio_idx = uio_find_by_name("neurax-msgdma");
    if (uio_idx < 0)
        goto err;

    char uio_path[32];
    snprintf(uio_path, sizeof(uio_path), "/dev/uio%d", uio_idx);
    ctx->fd_uio = open(uio_path, O_RDWR | O_SYNC);
    if (ctx->fd_uio < 0) {
        fprintf(stderr, "open %s: %s\n", uio_path, strerror(errno));
        goto err;
    }

    /* Linux 5.4+: mmap offset = region_index × PAGE_SIZE */
    long page = sysconf(_SC_PAGE_SIZE);

    ctx->tx_buf = mmap(NULL, DMA_BUF_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED, ctx->fd_uio, 0 * page);
    if (ctx->tx_buf == MAP_FAILED) {
        perror("mmap UIO TX buffer (mem[0])");
        goto err;
    }

    ctx->rx_buf = mmap(NULL, DMA_BUF_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED, ctx->fd_uio, 1 * page);
    if (ctx->rx_buf == MAP_FAILED) {
        perror("mmap UIO RX buffer (mem[1])");
        goto err;
    }

    ctx->tx_phys = uio_read_map_phys(uio_idx, 0);
    ctx->rx_phys = uio_read_map_phys(uio_idx, 1);
    if (!ctx->tx_phys || !ctx->rx_phys) {
        fprintf(stderr, "failed to read DMA buffer physical addresses from sysfs\n");
        goto err;
    }

    printf("[neurax_uio] TX buf: VA=%p  phys=0x%08x\n", ctx->tx_buf, ctx->tx_phys);
    printf("[neurax_uio] RX buf: VA=%p  phys=0x%08x\n", ctx->rx_buf, ctx->rx_phys);
    printf("[neurax_uio] mSGDMA0 (m2s) @ LW+0x%03x  mSGDMA1 (s2m) @ LW+0x%03x\n",
           MSGDMA0_OFFSET, MSGDMA1_OFFSET);

    /* Initial hardware reset */
    msgdma_reset(ctx->m2s);
    msgdma_reset(ctx->s2m);

    return 0;

err:
    if (ctx->rx_buf && ctx->rx_buf != MAP_FAILED) munmap(ctx->rx_buf, DMA_BUF_SIZE);
    if (ctx->tx_buf && ctx->tx_buf != MAP_FAILED) munmap(ctx->tx_buf, DMA_BUF_SIZE);
    if (ctx->lw_base && ctx->lw_base != MAP_FAILED) munmap(ctx->lw_base, LW_BRIDGE_SPAN);
    if (ctx->fd_uio  >= 0) close(ctx->fd_uio);
    if (ctx->fd_mem  >= 0) close(ctx->fd_mem);
    return -1;
}

void neurax_uio_deinit(struct neurax_uio_ctx *ctx)
{
    if (ctx->rx_buf  && ctx->rx_buf  != MAP_FAILED) munmap(ctx->rx_buf,  DMA_BUF_SIZE);
    if (ctx->tx_buf  && ctx->tx_buf  != MAP_FAILED) munmap(ctx->tx_buf,  DMA_BUF_SIZE);
    if (ctx->lw_base && ctx->lw_base != MAP_FAILED) munmap(ctx->lw_base, LW_BRIDGE_SPAN);
    if (ctx->fd_uio  >= 0) close(ctx->fd_uio);
    if (ctx->fd_mem  >= 0) close(ctx->fd_mem);
    memset(ctx, 0, sizeof(*ctx));
    ctx->fd_mem = ctx->fd_uio = -1;
}

/* ---------------------------------------------------------------------------
 * Send channel=1 SOF descriptor to mSGDMA0 (m2s).
 *
 * The FPGA neurax_data_interface resets write_index and clears buffer_full
 * on asi_channel_i=1, breaking any "buffer full / asi_ready=0" deadlock.
 * Call this before every write to guarantee the FPGA side is ready.
 * Returns 0 on success, -1 on timeout.
 * ------------------------------------------------------------------------- */
static int msgdma_send_sof(struct neurax_uio_ctx *ctx)
{
    msgdma_push_descr(ctx->m2s, ctx->tx_phys, 0,
                      sizeof(uint32_t), DESC_CHAN(1));

    uint64_t deadline = now_us() + SOF_TIMEOUT_US;
    uint32_t status;
    do {
        status = ctx->m2s->csr_status;
        if (!(status & CSR_ST_BUSY)) return 0;
        usleep_ms(1);
    } while (now_us() < deadline);

    msgdma_reset(ctx->m2s);
    fprintf(stderr, "[SOF] timeout (CSR=0x%08x) — channel support disabled in Qsys?\n",
            status);
    return -1;
}

/* ---------------------------------------------------------------------------
 * Write: copy up to DMA_BUF_SIZE bytes from 'src' to FPGA via mSGDMA0 (m2s).
 * Returns bytes transferred on success, negative on error.
 * ------------------------------------------------------------------------- */
ssize_t neurax_dma_write(struct neurax_uio_ctx *ctx,
                         const void *src, size_t len)
{
    if (len == 0) return 0;

    /* Send SOF first to reset FPGA write_index / clear buffer_full */
    if (msgdma_send_sof(ctx) < 0)
        return -1;

    ssize_t xfer = (ssize_t)(len > DMA_BUF_SIZE ? DMA_BUF_SIZE : len);

    /* Fill TX buffer */
    memcpy(ctx->tx_buf, src, (size_t)xfer);

    /* mSGDMA requires full-word (4-byte) transfers; pad to word boundary */
    uint32_t dma_len = ((uint32_t)xfer + 3u) & ~3u;
    if (dma_len > (uint32_t)xfer)
        memset((uint8_t *)ctx->tx_buf + xfer, 0, dma_len - (uint32_t)xfer);

    /* Push m2s descriptor: read from TX buffer phys, stream to FPGA sink */
    msgdma_push_descr(ctx->m2s, ctx->tx_phys, 0, dma_len, 0);

    uint32_t status = ctx->m2s->csr_status;
    uint32_t fill   = ctx->m2s->csr_fill_lvl;
    printf("[DMA write] CSR=0x%08x fill_rd=%u fill_wr=%u dma_len=%u user_len=%zd\n",
           status, fill & 0xffffu, fill >> 16, dma_len, xfer);

    /* Poll for completion */
    uint64_t deadline = now_us() + DMA_TIMEOUT_US;
    do {
        status = ctx->m2s->csr_status;
        if (!(status & CSR_ST_BUSY)) break;
        usleep_ms(1);
    } while (now_us() < deadline);

    if (status & CSR_ST_BUSY) {
        fprintf(stderr, "[DMA write] timeout — stream stall? CSR=0x%08x fill=0x%08x resp=%u\n",
                status, ctx->m2s->csr_fill_lvl, ctx->m2s->csr_resp_fill);
        msgdma_reset(ctx->m2s);
        return -1;
    }
    if (status & (CSR_ST_STOPPED_ON_ERR | CSR_ST_STOPPED_EOP)) {
        fprintf(stderr, "[DMA write] stopped (bus error?) CSR=0x%08x\n", status);
        msgdma_reset(ctx->m2s);
        return -1;
    }

    return xfer;
}

/* ---------------------------------------------------------------------------
 * Read: receive up to min(len, DMA_BUF_SIZE) bytes from FPGA via mSGDMA1 (s2m).
 * Returns 0 (EOF) once FPGA_OUTPUT_SIZE bytes have been consumed across calls.
 * Returns bytes received on success, negative on error.
 * ------------------------------------------------------------------------- */
ssize_t neurax_dma_read(struct neurax_uio_ctx *ctx, void *dst, size_t len)
{
    if (ctx->rx_offset >= FPGA_OUTPUT_SIZE)
        return 0;  /* EOF */

    size_t remaining = FPGA_OUTPUT_SIZE - ctx->rx_offset;
    ssize_t xfer = (ssize_t)(len < remaining ? len : remaining);
    if ((size_t)xfer > DMA_BUF_SIZE)
        xfer = (ssize_t)DMA_BUF_SIZE;

    /* Round up to word boundary (s2m is "Full Word Accesses Only") */
    uint32_t dma_len = ((uint32_t)xfer + 3u) & ~3u;

    /* Push s2m descriptor: FPGA source streams into RX buffer */
    msgdma_push_descr(ctx->s2m, 0, ctx->rx_phys, dma_len, 0);

    uint32_t status = ctx->s2m->csr_status;
    uint32_t fill   = ctx->s2m->csr_fill_lvl;
    printf("[DMA read]  CSR=0x%08x fill_rd=%u fill_wr=%u dma_len=%u user_len=%zd\n",
           status, fill & 0xffffu, fill >> 16, dma_len, xfer);

    /* Extra diagnostic: sample initial fill level 1 ms after descriptor push */
    usleep_ms(1);
    printf("[DMA read]  after 1ms: CSR=0x%08x fill=0x%08x resp=%u m2s_CSR=0x%08x neurax_ST=0x%08x\n",
           ctx->s2m->csr_status, ctx->s2m->csr_fill_lvl, ctx->s2m->csr_resp_fill,
           ctx->m2s->csr_status, ctx->neurax[REG_STATUS]);
    {
        volatile uint32_t *rxw = (volatile uint32_t *)ctx->rx_buf;
        printf("[DMA read]  rx_buf[0..3] after 1ms: 0x%08x 0x%08x 0x%08x 0x%08x\n",
               rxw[0], rxw[1], rxw[2], rxw[3]);
    }

    /* Poll for completion with verbose status every 500 ms */
    uint64_t deadline   = now_us() + DMA_TIMEOUT_US;
    uint64_t next_print = now_us() + 500000u;
    do {
        status = ctx->s2m->csr_status;
        if (!(status & CSR_ST_BUSY)) break;
        uint64_t t = now_us();
        if (t >= next_print) {
            volatile uint32_t *rxw = (volatile uint32_t *)ctx->rx_buf;
            printf("[DMA read poll] CSR=0x%08x fill=0x%08x resp=%u | m2s=0x%08x neurax=0x%08x | rx[0]=0x%08x rx[1]=0x%08x\n",
                   status, ctx->s2m->csr_fill_lvl, ctx->s2m->csr_resp_fill,
                   ctx->m2s->csr_status, ctx->neurax[REG_STATUS],
                   rxw[0], rxw[1]);
            next_print = t + 500000u;
        }
        usleep_ms(1);
    } while (now_us() < deadline);

    if (status & CSR_ST_BUSY) {
        volatile uint32_t *rxw = (volatile uint32_t *)ctx->rx_buf;
        fprintf(stderr, "[DMA read] timeout — stream stall? CSR=0x%08x fill=0x%08x resp=%u\n",
                status, ctx->s2m->csr_fill_lvl, ctx->s2m->csr_resp_fill);
        printf("[DMA read] rx_buf[0..7] at timeout:\n");
        for (int i = 0; i < 8; i++)
            printf("  [%d] 0x%08x\n", i, rxw[i]);
        msgdma_reset(ctx->s2m);
        return -1;
    }
    if (status & (CSR_ST_STOPPED_ON_ERR | CSR_ST_STOPPED_EOP)) {
        fprintf(stderr, "[DMA read] stopped (bus error?) CSR=0x%08x\n", status);
        msgdma_reset(ctx->s2m);
        return -1;
    }

    memcpy(dst, ctx->rx_buf, (size_t)xfer);
    ctx->rx_offset += (size_t)xfer;
    return xfer;
}

/* ---------------------------------------------------------------------------
 * Configure and start a convolution on the Neurax accelerator.
 * Blocks until STATUS_DONE or timeout. Returns 0 on success, -1 on timeout.
 *
 * REG_CONV_CONFIG_0: kernel_size[31:24] | in_ch[23:16] | padding[15:8] | stride[7:0]
 * REG_CONV_CONFIG_1: out_ch[15:8] | in_ch[7:0]
 * ------------------------------------------------------------------------- */
static int accelerator_start_conv(struct neurax_uio_ctx *ctx,
                                   int kernel, int stride, int padding,
                                   int in_ch,  int out_ch)
{
    uint32_t conv0 = ((uint32_t)(kernel  & 0xFF) << 24)
                   | ((uint32_t)(in_ch   & 0xFF) << 16)
                   | ((uint32_t)(padding & 0xFF) <<  8)
                   | ((uint32_t)(stride  & 0xFF) <<  0);
    uint32_t conv1 = ((uint32_t)(out_ch  & 0xFF) <<  8)
                   | ((uint32_t)(in_ch   & 0xFF) <<  0);

    /* Reset: disable → 1 ms → enable → 1 ms (matches neurax_reset in BSP) */
    ctx->neurax[REG_CMD] = 0;
    __sync_synchronize();
    usleep_ms(1);
    ctx->neurax[REG_CMD] = CMD_ENABLE;
    __sync_synchronize();
    usleep_ms(1);

    /* Write convolution config and batch size */
    ctx->neurax[REG_CONV_CONFIG_0] = conv0;
    ctx->neurax[REG_CONV_CONFIG_1] = conv1;
    ctx->neurax[REG_BATCH_SIZE]    = 1;
    __sync_synchronize();

    /* Start: write CMD_ENABLE | CMD_START, then clear START after 10 µs.
     * The start bit is edge-detected in HW (IDLE→OP transition), so it must
     * be cleared to avoid retriggering on any later register read/write.    */
    ctx->neurax[REG_CMD] = CMD_ENABLE | CMD_OP_CONV | CMD_START;
    __sync_synchronize();

    printf("[accel] started: kernel=%d stride=%d pad=%d in_ch=%d out_ch=%d "
           "CONV0=0x%08x CONV1=0x%08x\n",
           kernel, stride, padding, in_ch, out_ch, conv0, conv1);

    usleep(10);
    ctx->neurax[REG_CMD] = CMD_ENABLE | CMD_OP_CONV;  /* clear START bit */
    __sync_synchronize();

    uint64_t deadline = now_us() + 5000000u;  /* 5 s timeout */
    while (now_us() < deadline) {
        uint32_t st = ctx->neurax[REG_STATUS];
        if (st & STATUS_DONE) {
            printf("[accel] done  STATUS=0x%08x cycles=%u\n",
                   st, ctx->neurax[REG_DEBUG_CYCLES]);
            return 0;
        }
        usleep_ms(1);
    }
    fprintf(stderr, "[accel] timeout! STATUS=0x%08x\n", ctx->neurax[REG_STATUS]);
    return -1;
}

/* ---------------------------------------------------------------------------
 * main() — full test: build RAM image → DMA write → start → wait → DMA read
 * ------------------------------------------------------------------------- */
int main(void)
{
    struct neurax_uio_ctx ctx;

    printf("=== Neurax UIO DMA test ===\n");

    if (neurax_uio_init(&ctx) < 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    /* --- Print initial register state ------------------------------------ */
    printf("\nmSGDMA0 (m2s) CSR status=0x%08x ctrl=0x%08x\n",
           ctx.m2s->csr_status, ctx.m2s->csr_ctrl);
    printf("mSGDMA1 (s2m) CSR status=0x%08x ctrl=0x%08x\n",
           ctx.s2m->csr_status, ctx.s2m->csr_ctrl);
    printf("Neurax   CMD=0x%08x STATUS=0x%08x\n\n",
           ctx.neurax[REG_CMD], ctx.neurax[REG_STATUS]);

    /* --- Build full 23000-word RAM image ---------------------------------- *
     * Layout matches FPGA generics (see neurax_test.c):
     *   [0      .. 9999 ] input:   100×100 pixels, Q8.8 value 1.0
     *   [10000  .. 10008] weights: 3×3 kernel, Q8.8 value 1.0 (all-ones)
     *   [10009  .. 12999] unused weight area (zero)
     *   [13000         ] bias:    Q8.8 value 0.0
     *   [13001  .. 13015] unused bias area (zero)
     *   [13016  .. 23000] output:  written by accelerator
     * ----------------------------------------------------------------------- */
    const size_t ram_bytes = RAM_TOTAL_WORDS * sizeof(uint32_t);

    uint32_t *ram_buf = calloc(RAM_TOTAL_WORDS, sizeof(uint32_t));
    if (!ram_buf) { perror("calloc ram_buf"); goto done; }

    /* Input: 100×100 = 10000 words, each = 1.0 in Q8.8 */
    for (uint32_t i = RAM_INPUT_BASE; i < RAM_INPUT_BASE + 10000u; i++)
        ram_buf[i] = Q8_8_ONE;

    /* Weights: 3×3×1×1 = 9 words, each = 1.0 in Q8.8 */
    for (uint32_t i = RAM_WEIGHT_BASE; i < RAM_WEIGHT_BASE + 9u; i++)
        ram_buf[i] = Q8_8_ONE;

    /* Bias: 1 word for 1 output channel = 0.0 */
    ram_buf[RAM_BIAS_BASE] = Q8_8_ZERO;

    /* =========================================================================
     * DIAGNOSTIC TEST A: pre-arm s2m BEFORE write.
     * If this works (DMA completes), streaming is fundamentally OK and the
     * issue in the normal flow is timing (aso_ready_i not asserted when
     * reading_active first becomes 1, or something clears reading_active later).
     * If this ALSO fails, there is a fundamental hardware path issue.
     * ======================================================================= */
    printf("=== DIAGNOSTIC TEST A: pre-arm s2m descriptor BEFORE DMA write ===\n");
    {
        memset(ctx.rx_buf, 0xAB, 8 * sizeof(uint32_t));
        __sync_synchronize();
        msgdma_reset(ctx.s2m);
        msgdma_push_descr(ctx.s2m, 0, ctx.rx_phys, (uint32_t)ram_bytes, 0);
        printf("[TEST A] s2m armed: CSR=0x%08x fill=0x%08x\n",
               ctx.s2m->csr_status, ctx.s2m->csr_fill_lvl);

        /* Now write — when buffer_full fires, aso_ready_i is already 1 */
        ssize_t wra = neurax_dma_write(&ctx, ram_buf, ram_bytes);
        if (wra < 0) {
            fprintf(stderr, "[TEST A] DMA write failed\n");
        } else {
            printf("[TEST A] Write done (%zd bytes). Polling s2m...\n", wra);
            uint64_t dl = now_us() + 3000000u;
            uint64_t np = now_us();
            uint32_t st;
            volatile uint32_t *rxw = (volatile uint32_t *)ctx.rx_buf;
            do {
                st = ctx.s2m->csr_status;
                if (!(st & CSR_ST_BUSY)) break;
                uint64_t t = now_us();
                if (t >= np) {
                    printf("[TEST A poll] CSR=0x%08x fill=0x%08x resp=%u | rx[0]=0x%08x\n",
                           st, ctx.s2m->csr_fill_lvl, ctx.s2m->csr_resp_fill, rxw[0]);
                    np = t + 500000u;
                }
                usleep_ms(1);
            } while (now_us() < dl);
            printf("[TEST A] RESULT: CSR=0x%08x %s\n", ctx.s2m->csr_status,
                   (st & CSR_ST_BUSY) ? "*** TIMEOUT (streaming broken) ***"
                                      : "DONE (streaming OK!)");
            printf("[TEST A] rx_buf[0..7] (expect 0x%08x = Q8_8_ONE for word[0]):\n", Q8_8_ONE);
            for (int i = 0; i < 8; i++)
                printf("  [%d] 0x%08x\n", i, rxw[i]);
        }
        msgdma_reset(ctx.s2m);
    }
    printf("=== END DIAGNOSTIC TEST A ===\n\n");

    /* =========================================================================
     * DIAGNOSTIC TEST B: push s2m AFTER write but BEFORE accelerator.
     * If reading_active=1 persists after write, streaming should start as
     * soon as aso_ready_i=1 (i.e., immediately when we push the descriptor).
     * If TEST A passes but TEST B fails, the accelerator clears reading_active.
     * If TEST B passes but normal test fails, something else is wrong.
     * ======================================================================= */
    printf("=== DIAGNOSTIC TEST B: s2m push immediately AFTER write (no accel) ===\n");
    {
        memset(ctx.rx_buf, 0xAB, 8 * sizeof(uint32_t));
        __sync_synchronize();
        msgdma_reset(ctx.s2m);
        ssize_t wrb = neurax_dma_write(&ctx, ram_buf, ram_bytes);
        if (wrb < 0) {
            fprintf(stderr, "[TEST B] DMA write failed\n");
        } else {
            printf("[TEST B] Write done. Pushing s2m descriptor now...\n");
            msgdma_push_descr(ctx.s2m, 0, ctx.rx_phys, (uint32_t)ram_bytes, 0);
            printf("[TEST B] s2m initial: CSR=0x%08x fill=0x%08x\n",
                   ctx.s2m->csr_status, ctx.s2m->csr_fill_lvl);
            usleep_ms(1);
            {
                volatile uint32_t *rxw = (volatile uint32_t *)ctx.rx_buf;
                printf("[TEST B] after 1ms: CSR=0x%08x fill=0x%08x resp=%u | rx[0]=0x%08x\n",
                       ctx.s2m->csr_status, ctx.s2m->csr_fill_lvl,
                       ctx.s2m->csr_resp_fill, rxw[0]);
            }
            uint64_t dl = now_us() + 3000000u;
            uint64_t np = now_us() + 500000u;
            uint32_t st;
            volatile uint32_t *rxw = (volatile uint32_t *)ctx.rx_buf;
            do {
                st = ctx.s2m->csr_status;
                if (!(st & CSR_ST_BUSY)) break;
                uint64_t t = now_us();
                if (t >= np) {
                    printf("[TEST B poll] CSR=0x%08x fill=0x%08x resp=%u | rx[0]=0x%08x\n",
                           st, ctx.s2m->csr_fill_lvl, ctx.s2m->csr_resp_fill, rxw[0]);
                    np = t + 500000u;
                }
                usleep_ms(1);
            } while (now_us() < dl);
            printf("[TEST B] RESULT: CSR=0x%08x %s\n", ctx.s2m->csr_status,
                   (st & CSR_ST_BUSY) ? "*** TIMEOUT (reading_active issue?) ***"
                                      : "DONE (reading_active OK!)");
            printf("[TEST B] rx_buf[0..7]:\n");
            for (int i = 0; i < 8; i++)
                printf("  [%d] 0x%08x\n", i, rxw[i]);
        }
        msgdma_reset(ctx.s2m);
    }
    printf("=== END DIAGNOSTIC TEST B ===\n\n");

    /* --- DMA write: stream full 23000-word RAM image to FPGA -------------- */
    printf("Writing %zu bytes (%u words) to FPGA...\n", ram_bytes, RAM_TOTAL_WORDS);
    ssize_t wr = neurax_dma_write(&ctx, ram_buf, ram_bytes);
    if (wr < 0) {
        fprintf(stderr, "DMA write failed\n");
        free(ram_buf);
        goto done;
    }
    printf("Write OK: %zd bytes\n\n", wr);

    /* --- Start Neurax convolution ----------------------------------------- *
     * 3×3 kernel, stride=1, padding=0, 1 input channel, 1 output channel.
     * Expected output (no bias): each interior pixel = sum of 3×3 ones = 9.0
     * in Q8.8 → 0x00000900.  Edge pixels depend on padding mode (none here).
     * ----------------------------------------------------------------------- */
    if (accelerator_start_conv(&ctx, 3, 1, 0, 1, 1) < 0) {
        fprintf(stderr, "Accelerator start failed\n");
        free(ram_buf);
        goto done;
    }

    /* --- DMA read: stream full RAM image back from FPGA ------------------- */
    uint32_t *result = calloc(RAM_TOTAL_WORDS, sizeof(uint32_t));
    if (!result) { perror("calloc result"); free(ram_buf); goto done; }

    printf("\nReading %zu bytes (%u words) from FPGA...\n", ram_bytes, RAM_TOTAL_WORDS);
    ctx.rx_offset = 0;
    size_t total_read = 0;
    while (total_read < ram_bytes) {
        ssize_t rd = neurax_dma_read(&ctx,
                                     (uint8_t *)result + total_read,
                                     ram_bytes - total_read);
        if (rd == 0) break;
        if (rd < 0) {
            fprintf(stderr, "DMA read failed at offset %zu\n", total_read);
            break;
        }
        total_read += (size_t)rd;
    }
    printf("Read OK: %zu bytes\n\n", total_read);

    /* --- Dump first 8 output words (expected ~0x00000900 = 9.0 in Q8.8) -- */
    printf("Output words [%u..%u] (expected 9.0 = 0x00000900 for interior pixels):\n",
           RAM_OUTPUT_BASE, RAM_OUTPUT_BASE + 7u);
    for (uint32_t i = 0; i < 8u; i++) {
        uint32_t w = result[RAM_OUTPUT_BASE + i];
        int16_t q = (int16_t)(w & 0xFFFF);
        printf("  [%u] 0x%08x  (Q8.8 = %d → %.4f)\n",
               RAM_OUTPUT_BASE + i, w, q, (float)q / 256.0f);
    }

    free(result);
    free(ram_buf);

done:
    neurax_uio_deinit(&ctx);
    printf("\nDone.\n");
    return 0;
}
