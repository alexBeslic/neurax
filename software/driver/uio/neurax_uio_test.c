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

    /* Poll for completion */
    uint64_t deadline = now_us() + DMA_TIMEOUT_US;
    do {
        status = ctx->s2m->csr_status;
        if (!(status & CSR_ST_BUSY)) break;
        usleep_ms(1);
    } while (now_us() < deadline);

    if (status & CSR_ST_BUSY) {
        fprintf(stderr, "[DMA read] timeout — stream stall? CSR=0x%08x fill=0x%08x resp=%u\n",
                status, ctx->s2m->csr_fill_lvl, ctx->s2m->csr_resp_fill);
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
 * main() — smoke test: send a block of incrementing words, read back results
 * ------------------------------------------------------------------------- */
int main(void)
{
    struct neurax_uio_ctx ctx;

    printf("=== Neurax UIO DMA test ===\n");

    if (neurax_uio_init(&ctx) < 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    /* --- Print mSGDMA CSR state ------------------------------------------ */
    printf("\nmSGDMA0 (m2s) CSR status=0x%08x ctrl=0x%08x\n",
           ctx.m2s->csr_status, ctx.m2s->csr_ctrl);
    printf("mSGDMA1 (s2m) CSR status=0x%08x ctrl=0x%08x\n\n",
           ctx.s2m->csr_status, ctx.s2m->csr_ctrl);

    /* --- Build test payload: 100×100 Q8.8 words of incrementing values ---- */
    const size_t n_words = 100u * 100u;
    const size_t tx_size = n_words * sizeof(uint32_t);

    uint32_t *payload = malloc(tx_size);
    if (!payload) { perror("malloc"); goto done; }
    for (size_t i = 0; i < n_words; i++)
        payload[i] = (uint32_t)(i & 0xFFFFu);  /* Q8.8: small fixed-point values */

    /* --- Write to FPGA ---------------------------------------------------- */
    printf("Writing %zu bytes to FPGA...\n", tx_size);
    ssize_t wr = neurax_dma_write(&ctx, payload, tx_size);
    if (wr < 0) {
        fprintf(stderr, "DMA write failed\n");
        free(payload);
        goto done;
    }
    printf("Write OK: %zd bytes transferred\n\n", wr);

    /* --- (Here you would start the Neurax accelerator via ctx.neurax regs) */
    /* Example (uncomment and adapt to actual register layout):
     *   ctx.neurax[REG_CMD] = CMD_ENABLE | CMD_START;
     *   while (!(ctx.neurax[REG_STATUS] & STATUS_DONE)) usleep_ms(1);
     */

    /* --- Read results from FPGA ------------------------------------------- */
    const size_t out_words = FPGA_OUTPUT_SIZE / sizeof(uint32_t);
    uint32_t *result = malloc(FPGA_OUTPUT_SIZE);
    if (!result) { perror("malloc"); free(payload); goto done; }

    printf("Reading %zu bytes from FPGA (FPGA_OUTPUT_SIZE=%u words)...\n",
           FPGA_OUTPUT_SIZE, (unsigned)out_words);

    size_t total_read = 0;
    ctx.rx_offset = 0;
    while (total_read < FPGA_OUTPUT_SIZE) {
        ssize_t rd = neurax_dma_read(&ctx,
                                     (uint8_t *)result + total_read,
                                     FPGA_OUTPUT_SIZE - total_read);
        if (rd == 0) break;   /* EOF */
        if (rd < 0) {
            fprintf(stderr, "DMA read failed at offset %zu\n", total_read);
            break;
        }
        total_read += (size_t)rd;
    }
    printf("Read OK: %zu bytes received\n\n", total_read);

    /* --- Dump first 8 output words ---------------------------------------- */
    printf("First 8 output words (Q8.8 format):\n");
    for (int i = 0; i < 8 && (size_t)i < total_read / sizeof(uint32_t); i++)
        printf("  [%d] 0x%08x  (%d.%02d)\n", i, result[i],
               result[i] >> 8, (int)(((result[i] & 0xFF) * 100) / 256));

    free(result);
    free(payload);

done:
    neurax_uio_deinit(&ctx);
    printf("\nDone.\n");
    return 0;
}
