/*
 * neurax_uio_test.c — Userspace DMA test for Neurax mSGDMA via UIO
 *
 * Replaces the /dev/msgdma kernel char-device path with direct mSGDMA
 * descriptor programming from userspace.
 *
 * Setup:
 *   insmod uio.ko                 # loads the generic UIO kernel module
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
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>

#include "msgdma_uio.h"
#include "neurax_regs.h"

/* ---------------------------------------------------------------------------
 * Neurax accelerator register map (LW bridge offset 0x000, same page as mSGDMAs)
 * Mirrors neurax_bsp.h — copied here to keep the test self-contained.
 * ------------------------------------------------------------------------- */
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

/* Q8.8: 1.0 → 0x0100 */
#define Q8_8_ONE        0x00000100u
#define Q8_8_ZERO       0x00000000u

/* Full altsyncram integrity test constants (derived from msgdma_uio.h) */
#define FPGA_RAM_WORDS    (FPGA_OUTPUT_SIZE / sizeof(uint32_t)) /* 23000 words */
#define FPGA_RAM_BYTES    FPGA_OUTPUT_SIZE                     /* 92000 bytes */
#define MAX_PRINT_ERRORS  32u  /* cap per-mismatch output for readability    */

/* ---------------------------------------------------------------------------
 * Internal state
 * ------------------------------------------------------------------------- */
struct neurax_uio_ctx {
    /* /dev/mem mapping — covers full LW bridge page */
    int      fd_mem;
    void    *lw_base;               /* mmap base for LW bridge              */
    volatile struct msgdma_reg *m2s; /* mSGDMA0: HPS→FPGA (write path)      */
    volatile struct msgdma_reg *s2m; /* mSGDMA1: FPGA→HPS (read  path)      */
    volatile neurax_reg_t      *neurax; /* Neurax control register block     */

    /* UIO mapping — DMA-coherent buffers */
    int      fd_uio;
    void    *tx_buf;                /* userspace VA of TX buffer             */
    void    *rx_buf;                /* userspace VA of RX buffer             */
    uintptr_t  tx_phys;               /* physical addr → m2s desc read_addr   */
    uintptr_t  rx_phys;               /* physical addr → s2m desc write_addr  */

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
            size_t n = strlen(name);
            while (n > 0 && isspace((unsigned char)name[n - 1])) {
                name[n - 1] = '\0';
                n--;
            }
            if (strcmp(name, target) == 0) {
                fclose(f);
                closedir(d);
                return atoi(de->d_name + 3);
            }
        }
        fclose(f);
    }

    closedir(d);
    fprintf(stderr, "UIO device '%s' not found in /sys/class/uio\n", target);
    return -1;
}

/* Read a hex value from a UIO sysfs attribute (e.g. maps/map0/addr). */
static uintptr_t  uio_read_map_phys(int uio_idx, int map_idx)
{
    char path[256];
    snprintf(path, sizeof(path),
             "/sys/class/uio/uio%d/maps/map%d/addr", uio_idx, map_idx);
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
        return 0;
    }
    uintptr_t  addr = 0;
    if (fscanf(f, "%" SCNxPTR, &addr) != 1) {
        fprintf(stderr, "failed to parse address from %s\n", path);
        fclose(f);
        return 0;
    }
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
    ctx->lw_base = MAP_FAILED;
    ctx->tx_buf  = MAP_FAILED;
    ctx->rx_buf  = MAP_FAILED;

    ctx->fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if (ctx->fd_mem < 0) {
        perror("open /dev/mem");
        return -1;
    }

    ctx->lw_base = mmap(NULL, LW_BRIDGE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->fd_mem, LW_BRIDGE_BASE);
    if (ctx->lw_base == MAP_FAILED) {
        perror("mmap LW bridge");
        goto err;
    }

    ctx->neurax = (volatile neurax_reg_t *)((char *)ctx->lw_base + NEURAX_REG_OFFSET);
    ctx->m2s    = (volatile struct msgdma_reg *)((char *)ctx->lw_base + MSGDMA0_OFFSET);
    ctx->s2m    = (volatile struct msgdma_reg *)((char *)ctx->lw_base + MSGDMA1_OFFSET);

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

    long page = sysconf(_SC_PAGE_SIZE);

    ctx->tx_buf = mmap(NULL, DMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->fd_uio, 0 * page);
    if (ctx->tx_buf == MAP_FAILED) {
        perror("mmap UIO TX buffer");
        goto err;
    }

    ctx->rx_buf = mmap(NULL, DMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->fd_uio, 1 * page);
    if (ctx->rx_buf == MAP_FAILED) {
        perror("mmap UIO RX buffer");
        goto err;
    }

    ctx->tx_phys = uio_read_map_phys(uio_idx, 0);
    ctx->rx_phys = uio_read_map_phys(uio_idx, 1);
    if (!ctx->tx_phys || !ctx->rx_phys) {
        fprintf(stderr, "failed to read DMA addresses from sysfs\n");
        goto err;
    }

    printf("[neurax_uio] TX buf: VA=%p  phys=0x%08"PRIxPTR"\n", ctx->tx_buf, ctx->tx_phys);
    printf("[neurax_uio] RX buf: VA=%p  phys=0x%08"PRIxPTR"\n", ctx->rx_buf, ctx->rx_phys);

    msgdma_reset(ctx->m2s);
    msgdma_reset(ctx->s2m);

    return 0;

err:
    if (ctx->rx_buf  != MAP_FAILED) munmap(ctx->rx_buf, DMA_BUF_SIZE);
    if (ctx->tx_buf  != MAP_FAILED) munmap(ctx->tx_buf, DMA_BUF_SIZE);
    if (ctx->lw_base != MAP_FAILED) munmap(ctx->lw_base, LW_BRIDGE_SPAN);
    if (ctx->fd_uio  >= 0) close(ctx->fd_uio);
    if (ctx->fd_mem  >= 0) close(ctx->fd_mem);
    
    ctx->rx_buf = MAP_FAILED;
    ctx->tx_buf = MAP_FAILED;
    ctx->lw_base = MAP_FAILED;
    return -1;
}

void neurax_uio_deinit(struct neurax_uio_ctx *ctx)
{
    if (ctx->rx_buf  != MAP_FAILED) munmap(ctx->rx_buf, DMA_BUF_SIZE);
    if (ctx->tx_buf  != MAP_FAILED) munmap(ctx->tx_buf, DMA_BUF_SIZE);
    if (ctx->lw_base != MAP_FAILED) munmap(ctx->lw_base, LW_BRIDGE_SPAN);
    if (ctx->fd_uio  >= 0) close(ctx->fd_uio);
    if (ctx->fd_mem  >= 0) close(ctx->fd_mem);
    printf("[neurax_uio] Resources cleanly deinitialized.\n");
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
           ctx->m2s->csr_status, ctx->neurax->reg_status);
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
                   ctx->m2s->csr_status, ctx->neurax->reg_status,
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
 * poll_dma_complete — wait for an mSGDMA to leave BUSY state or timeout.
 * Resets the DMA and returns -1 on timeout.
 * ------------------------------------------------------------------------- */
static int poll_dma_complete(volatile struct msgdma_reg *dma, uint32_t timeout_us)
{
    uint64_t deadline = now_us() + timeout_us;
    uint32_t status;
    do {
        status = dma->csr_status;
        if (!(status & CSR_ST_BUSY)) return 0;
        usleep_ms(1);
    } while (now_us() < deadline);

    fprintf(stderr, "[DMA poll] timeout: CSR=0x%08x fill=0x%08x resp=%u\n",
            status, dma->csr_fill_lvl, dma->csr_resp_fill);
    msgdma_reset(dma);
    return -1;
}

/* ---------------------------------------------------------------------------
 * Q8.8 fixed-point helpers (mirrors software/bsp/neurax_bsp.h)
 * ------------------------------------------------------------------------- */
static inline int16_t float_to_q8_8(float val)
{
    int32_t fixed = (int32_t)(val * 256.0f);
    if (fixed > 32767) fixed = 32767;
    if (fixed < -32768) fixed = -32768;
    return (int16_t)fixed;
}

static inline float q8_8_to_float(int16_t val)
{
    return (float)val / 256.0f;
}

/* Convolution test parameters (must match FPGA generics g_FB_HEIGHT/WIDTH) */
#define CONV_INPUT_H     100
#define CONV_INPUT_W     100
#define CONV_KERNEL      3
#define CONV_IN_CH       1
#define CONV_OUT_CH      1
#define CONV_STRIDE      1
#define CONV_PADDING     0

/* Output size: floor((100 - 3)/1) + 1 = 98 → 98x98 */
#define CONV_OUTPUT_H    98
#define CONV_OUTPUT_W    98

/* ---------------------------------------------------------------------------
 * test_full_ram — build a small convolution workload (uniform 1.0 input,
 * all-ones 3x3 kernel, zero bias), stream it into FPGA RAM, run the
 * accelerator, then stream the RAM back out and verify a handful of
 * interior output values (see software/bsp/neurax_test.c for the reference
 * sequence this mirrors).
 *
 * DMA plumbing is unchanged from the earlier RAM-integrity test: s2m is
 * pre-armed before neurax_dma_write() (which itself sends the channel=1 SOF
 * descriptor), then results are pulled back with poll_dma_complete().
 *
 * Returns 0 on PASS, -1 on any error.
 * ------------------------------------------------------------------------- */
static int test_full_ram(struct neurax_uio_ctx *ctx)
{
    const uint32_t n_words = FPGA_RAM_WORDS;   /* 23000 */
    const uint32_t n_bytes = FPGA_RAM_BYTES;   /* 92000 */

    printf("\n=== Convolution test (%dx%d input, %dx%d kernel) ===\n",
           CONV_INPUT_H, CONV_INPUT_W, CONV_KERNEL, CONV_KERNEL);

    /* 1. Build the RAM image: input=1.0, weights=1.0, bias=0.0, output=0 */
    uint32_t *tx_data = calloc(n_words, sizeof(uint32_t));
    if (!tx_data) {
        perror("calloc tx_data");
        return -1;
    }

    int16_t val_one = float_to_q8_8(1.0f);
    for (uint32_t i = 0; i < (uint32_t)(CONV_INPUT_H * CONV_INPUT_W); i++)
        tx_data[RAM_INPUT_BASE + i] = (uint16_t)val_one;

    for (uint32_t i = 0; i < (uint32_t)(CONV_KERNEL * CONV_KERNEL * CONV_IN_CH * CONV_OUT_CH); i++)
        tx_data[RAM_WEIGHT_BASE + i] = (uint16_t)val_one;

    tx_data[RAM_BIAS_BASE] = (uint16_t)float_to_q8_8(0.0f);

    /* [diag] Overlay a unique, unambiguous ramp on the first 10 input words
     * (row 0, cols 0-9). These border pixels are NOT read by any of the
     * interior output positions checked below (output[1][1].. depend on
     * input rows/cols >= 1), so this cannot change the convolution result.
     * Using values that don't collide with the uniform 0x0100 (=1.0) used
     * everywhere else lets us conclusively detect a write-side address
     * shift/drop: if the DMA write drops or misroutes one word, the
     * readback pattern here will NOT be 0x2000..0x2009 at addresses 0..9. */
    // for (uint32_t i = 0; i < 10; i++)
    //     tx_data[RAM_INPUT_BASE + i] = 0x2000u + i;

    printf("[conv] Input: %dx%d uniform 1.0, Weights: %dx%d all-ones, Bias: 0.0\n",
           CONV_INPUT_H, CONV_INPUT_W, CONV_KERNEL, CONV_KERNEL);

    /* 2. Configure the accelerator BEFORE the DMA write so the ST sink is
     *    ready (mirrors neurax_reset()/neurax_config_conv() in neurax_bsp.c) */
    ctx->neurax->reg_cmd = 0;
    usleep_ms(1);
    ctx->neurax->reg_cmd = CMD_ENABLE;
    usleep_ms(1);

    uint32_t conv0 = ((CONV_KERNEL & 0xFF) << 24)
                   | ((CONV_PADDING & 0xFF) << 8)
                   | ((CONV_STRIDE & 0xFF) << 0);
    uint32_t conv1 = ((CONV_OUT_CH & 0xFF) << 8)
                   | ((CONV_IN_CH  & 0xFF) << 0);
    ctx->neurax->reg_conv_config_0 = conv0;
    ctx->neurax->reg_conv_config_1 = conv1;
    ctx->neurax->reg_batch_size    = 1;  /* MUST be 1, 0 skips computation */
    volatile uint32_t status;

    /* 3. Clear RX buffer; pre-arm s2m before the write so data can stream
     *    through the circular buffer pipeline as m2s fills it. */
    memset(ctx->rx_buf, 0, n_bytes);
    __sync_synchronize();
    msgdma_reset(ctx->s2m);
    msgdma_push_descr(ctx->s2m, 0, (uint32_t)ctx->rx_phys, n_bytes, 0);

    /* 4. Write full RAM image to FPGA (neurax_dma_write sends SOF first) */
    printf("[conv] Writing %"PRIu32" words...\n", n_words);
    ssize_t wr = neurax_dma_write(ctx, tx_data, n_bytes);
    if (wr < 0 || (size_t)wr != n_bytes) {
        fprintf(stderr, "[conv] Write failed or incomplete: %zd of %u bytes\n",
                wr, n_bytes);
        msgdma_reset(ctx->s2m);
        free(tx_data);
        return -1;
    }
    printf("[conv] Write OK: %zd bytes\n", wr);

    usleep_ms(1000);

    /* 5. Start the convolution. START is edge-detected — clear it again
     *    after a short delay so it doesn't retrigger (see neurax_start()). */
    printf("[conv] Starting convolution...\n");
    ctx->neurax->reg_cmd = CMD_ENABLE | CMD_OP_CONV | CMD_START;
    __sync_synchronize();
    usleep_ms(1);
    ctx->neurax->reg_cmd = CMD_ENABLE | CMD_OP_CONV;

    status = ctx->neurax->reg_status;
    if (status & STATUS_DONE) {
        fprintf(stderr,
                "[conv] START was not accepted: DONE stayed set (STATUS=0x%08x)\n",
                status);
        free(tx_data);
        return -1;
    }

    /* 6. Poll for completion (STATUS_DONE). This MUST happen before triggering
     *    the RAM->HPS readback below: the data_interface read FSM and the
     *    accelerator's writes to RAM Port B are otherwise unsynchronized, so
     *    without this wait the readback can start (and even finish) before
     *    the accelerator has written any/all of the output region, yielding
     *    stale or all-zero output words regardless of correct Q8.8 data. */
    uint64_t conv_deadline = now_us() + DMA_TIMEOUT_US;
    do {
        status = ctx->neurax->reg_status;
        if (status & STATUS_DONE) break;
        usleep_ms(1);
    } while (now_us() < conv_deadline);

    if (!(status & STATUS_DONE)) {
        fprintf(stderr, "[conv] Convolution TIMEOUT (STATUS=0x%08x)\n", status);
        free(tx_data);
        return -1;
    }
    printf("[conv] Done. STATUS=0x%08x cycles=%u\n",
           status, ctx->neurax->reg_debug_cycles);

    /* Data length must match the s2m DMA descriptor pushed in step 3
     * (n_bytes = n_words words) -- requesting more words here than the
     * mSGDMA descriptor is sized for desyncs the internal read FSM from
     * the external DMA (FSM keeps streaming, mSGDMA stops accepting). */
    ctx->neurax->reg_data_sc = 0;  /* clear data length in words */
    ctx->neurax->reg_data_read = (n_words << 16u);  /* set data length in words */
    printf("[conv][diag] reg_data_sc before START=0x%08x reg_data_read=0x%08x\n",
           ctx->neurax->reg_data_sc, ctx->neurax->reg_data_read);
    ctx->neurax->reg_data_sc = 1 << 31; /* set data start bit */
    __sync_synchronize();
    printf("[conv][diag] reg_data_sc right after START=0x%08x\n",
           ctx->neurax->reg_data_sc);

    /* 7. Poll s2m: full RAM image (including output region) streams back */
    printf("[conv] Waiting for readback to complete...\n");
    if (poll_dma_complete(ctx->s2m, DMA_TIMEOUT_US) < 0) {
        fprintf(stderr, "[conv] Readback timed out\n");
        free(tx_data);
        return -1;
    }
    printf("[conv] Readback OK\n");
    printf("[conv][diag] reg_data_sc right after s2m poll_dma_complete=0x%08x\n",
           ctx->neurax->reg_data_sc);

    usleep_ms(1000);

    /* Diagnostic: independently confirm the neurax core's OWN read-FSM
     * status agrees with the mSGDMA's completion (bit0=DONE, bit1=BUSY).
     * If BUSY never clears here, the mSGDMA believed the transfer was
     * complete (byte count satisfied) while the neurax read state machine
     * disagrees -- a strong signal that the two are desynchronized. */
    {
        uint64_t deadline = now_us() + DMA_TIMEOUT_US;
        uint32_t sc;
        do {
            sc = ctx->neurax->reg_data_sc;
            if (!(sc & (1u << 1))) break;  /* BUSY clear */
            usleep_ms(1);
        } while (now_us() < deadline);
        printf("[conv][diag] reg_data_sc settle-poll result=0x%08x (BUSY=%d DONE=%d)\n",
               sc, (sc >> 1) & 1, sc & 1);
        if (sc & (1u << 1)) {
            fprintf(stderr, "[conv] FPGA RAM read FSM did not finish\n");
            free(tx_data);
            return -1;
        }
    }

    /* 8. Verify a few interior output values (expected 3x3 sum = 9.0) */
    uint32_t *rx = malloc(n_bytes);
    if (!rx) {
        perror("malloc rx");
        free(tx_data);
        return -1;
    }
    memcpy(rx, ctx->rx_buf, n_bytes);

    int ram_errors = 0;
    const uint32_t input_words = CONV_INPUT_H * CONV_INPUT_W;
    const uint32_t weight_words =
        CONV_KERNEL * CONV_KERNEL * CONV_IN_CH * CONV_OUT_CH;
    for (uint32_t i = 0; i < input_words; i++) {
        if (rx[RAM_INPUT_BASE + i] != (uint32_t)(uint16_t)val_one) {
            if (ram_errors < 8)
                fprintf(stderr,
                        "[conv] RAM input mismatch at word %u: got 0x%08" PRIx32
                        ", expected 0x%08" PRIx32 "\n",
                        i, rx[RAM_INPUT_BASE + i],
                        (uint32_t)(uint16_t)val_one);
            ram_errors++;
        }
    }
    for (uint32_t i = 0; i < weight_words; i++) {
        if (rx[RAM_WEIGHT_BASE + i] != (uint32_t)(uint16_t)val_one) {
            if (ram_errors < 8)
                fprintf(stderr,
                        "[conv] RAM weight mismatch at word %u: got 0x%08" PRIx32
                        ", expected 0x%08" PRIx32 "\n",
                        i, rx[RAM_WEIGHT_BASE + i],
                        (uint32_t)(uint16_t)val_one);
            ram_errors++;
        }
    }
    if (ram_errors != 0) {
        fprintf(stderr,
                "[conv] %d input/weight RAM values are wrong; investigate m2s "
                "DMA or RAM readback before convolution arithmetic\n",
                ram_errors);
    } else {
        printf("[conv] RAM input and all %u weights verified after readback\n",
               weight_words);
    }

    float expected_val = 9.0f;
    int check_positions[][2] = { {10, 10}, {50, 50}, {90, 90}, {1, 1}, {97, 97} };
    int num_checks = sizeof(check_positions) / sizeof(check_positions[0]);
    int errors = 0;

    printf("[conv] Checking output values (expected %.1f for interior positions):\n",
           expected_val);
    for (int c = 0; c < num_checks; c++) {
        int oh = check_positions[c][0];
        int ow = check_positions[c][1];
        int out_idx = oh * CONV_OUTPUT_W + ow;

        uint32_t word = rx[RAM_OUTPUT_BASE + out_idx];
        int16_t val = (int16_t)(word & 0xFFFFu);
        float fval = q8_8_to_float(val);

        printf("    output[%d][%d] (idx=%d) = %.4f (raw=0x%04x, expected=%.1f)\n",
               oh, ow, out_idx, fval, (uint16_t)val, expected_val);

        if (fval < expected_val - 0.5f || fval > expected_val + 0.5f)
            errors++;
    }

    printf("Input values:\n");
    for (size_t i = 0; i < 10; i++)
    {
        printf("Read at %d: 0x%08x (expected 0x%08x)\n", (int)i, rx[i], (uint32_t)(uint16_t)val_one);
    }
    
    printf("Ouput values:\n");
    for (size_t i = RAM_OUTPUT_BASE; i < RAM_OUTPUT_BASE+10; i++)
    {
        printf("Read at %d: %d\n",i,rx[i]);
    }
    
    FILE *pf = fopen("res.txt", "w");

    if (!pf) {
        perror("fopen res.txt");
        free(rx);
        free(tx_data);
        return -1;
    }
    for (size_t i = 0; i < n_words; i++)
        fprintf(pf, "Read at %zu: 0x%08" PRIx32 "\n", i, rx[i]);
    fclose(pf);
    

    free(rx);
    free(tx_data);
    tx_data = NULL;

    printf("\nResult             : %s\n",
           errors == 0 && ram_errors == 0 ? "PASS" : "FAIL");
    return errors == 0 && ram_errors == 0 ? 0 : -1;
}

/* ---------------------------------------------------------------------------
 * main() — full test: build RAM image → DMA write → start → wait → DMA read
 * ------------------------------------------------------------------------- */
int main(void)
{
    struct neurax_uio_ctx ctx;
    int final_result = 0;

    printf("=== Neurax UIO DMA test ===\n");

    if (neurax_uio_init(&ctx) < 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    /* --- Print initial register state ------------------------------------ */
    printf("Sanity check: Read the magic number from the Neurax register block: 0x%08x\n",
           ctx.neurax->reg_read_only);
    printf("\nmSGDMA0 (m2s) CSR status=0x%08x ctrl=0x%08x\n",
           ctx.m2s->csr_status, ctx.m2s->csr_ctrl);
    printf("mSGDMA1 (s2m) CSR status=0x%08x ctrl=0x%08x\n",
           ctx.s2m->csr_status, ctx.s2m->csr_ctrl);
    printf("Neurax   CMD=0x%08x STATUS=0x%08x\n\n",
           ctx.neurax->reg_cmd, ctx.neurax->reg_status);
    printf("Neurax reg reg_data_sc=0x%08x reg_data_read=0x%08x\n",
           ctx.neurax->reg_data_sc, ctx.neurax->reg_data_read);

    
    // const size_t ram_bytes = RAM_TOTAL_WORDS * sizeof(uint32_t);

    // uint32_t *ram_buf = calloc(RAM_TOTAL_WORDS, sizeof(uint32_t));
    // if (!ram_buf) { perror("calloc ram_buf"); goto done; }

    // ram_buf[0] = 0x12345678u;

    // memset(ctx.rx_buf, 0xAB, 1 * sizeof(uint32_t));
    // __sync_synchronize();
    // msgdma_reset(ctx.s2m);
    // msgdma_push_descr(ctx.s2m, 0, ctx.rx_phys, (uint32_t)ram_bytes, 0);
    // printf("Writing %zu bytes (%u words) to FPGA...\n", ram_bytes, RAM_TOTAL_WORDS);
    // ssize_t wr = neurax_dma_write(&ctx, ram_buf, ram_bytes);
    // if (wr < 0) {
    //     fprintf(stderr, "DMA write failed\n");
    //     free(ram_buf);
    //     goto done;
    // }
    // printf("Write OK: %zd bytes\n\n", wr);

    // /* --- DMA read: stream full RAM image back from FPGA ------------------- */
    // uint32_t *result = calloc(RAM_TOTAL_WORDS, sizeof(uint32_t));
    // if (!result) { perror("calloc result"); free(ram_buf); goto done; }

    // printf("\nReading %zu bytes (%u words) from FPGA...\n", ram_bytes, RAM_TOTAL_WORDS);
    // ctx.rx_offset = 0;
    // ssize_t rd = neurax_dma_read(&ctx,
    //                                  (uint8_t *)result,
    //                                  ram_bytes);
    //     if (rd < 0) {
    //         fprintf(stderr, "DMA read failed at offset %zu\n", ram_bytes);
    //     }


    // printf("Result sample 0x%08x \n", result[0]);

    // free(result);
    // free(ram_buf);

    /* --- Convolution test -------------------------------------------------- */
    if (test_full_ram(&ctx) != 0)
        final_result = 1;

    printf("Sanity check: Read the magic number from the Neurax register block: 0x%08x\n",
           ctx.neurax->reg_read_only);
    printf("\nmSGDMA0 (m2s) CSR status=0x%08x ctrl=0x%08x\n",
           ctx.m2s->csr_status, ctx.m2s->csr_ctrl);
    printf("mSGDMA1 (s2m) CSR status=0x%08x ctrl=0x%08x\n",
           ctx.s2m->csr_status, ctx.s2m->csr_ctrl);
    printf("Neurax   CMD=0x%08x STATUS=0x%08x\n\n",
           ctx.neurax->reg_cmd, ctx.neurax->reg_status);
    printf("Neurax reg reg_data_sc=0x%08x reg_data_read=0x%08x\n",
           ctx.neurax->reg_data_sc, ctx.neurax->reg_data_read);

done:
    neurax_uio_deinit(&ctx);
    printf("\nDone.\n");
    return final_result;
}
