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
#define RAM_TOTAL_WORDS  1u

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
 * test_pattern — unique 32-bit value for word index i
 * ------------------------------------------------------------------------- */
static inline uint32_t test_pattern(uint32_t i)
{
    return 0xFE580000u | (i & 0xFFFFu);
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
 * test_full_ram — write a unique pattern to every RAM word, read it back
 * through the circular loopback buffer, and verify all locations.
 *
 * Protocol note: s2m is pre-armed BEFORE the write so that data flows
 * through the 1-word pipeline stage as m2s fills it (backpressure would
 * otherwise stall m2s after the first word).  neurax_dma_write() sends a
 * channel=1 SOF first; the Qsys adapter suppresses valid for channel > 0,
 * so the FPGA never asserts asi_ready=0 during SOF and s2m capacity is
 * not consumed by it.
 *
 * Returns 0 on PASS, -1 on any error.
 * ------------------------------------------------------------------------- */
static int test_full_ram(struct neurax_uio_ctx *ctx)
{
    const uint32_t n_words = (uint32_t)(FPGA_RAM_WORDS);
    const uint32_t n_bytes = (uint32_t)(FPGA_RAM_BYTES);

    printf("\n=== Full RAM integrity test (%"PRIu32" words, %"PRIu32" bytes) ===\n",
           n_words, n_bytes);

    /* 1. Build test pattern in a temporary TX buffer */
    uint32_t *tx_data = malloc(n_bytes);
    if (!tx_data) {
        perror("malloc tx_data");
        return -1;
    }
    for (uint32_t i = 0; i < n_words; i++)
        tx_data[i] = test_pattern(i);

    /* 2. Clear RX buffer; pre-arm s2m before the write so data can stream
     *    through the circular buffer pipeline as m2s fills it. */
    memset(ctx->rx_buf, 0, n_bytes);
    __sync_synchronize();
    msgdma_reset(ctx->s2m);
    msgdma_push_descr(ctx->s2m, 0, (uint32_t)ctx->rx_phys, n_bytes, 0);

    /* 3. Write full buffer to FPGA (neurax_dma_write sends SOF first) */
    printf("[full-RAM] Writing %"PRIu32" words...\n", n_words);
    ssize_t wr = neurax_dma_write(ctx, tx_data, n_bytes);
    tx_data = NULL;
    if (wr < 0) {
        fprintf(stderr, "[full-RAM] Write failed\n");
        msgdma_reset(ctx->s2m);
        return -1;
    }
    printf("[full-RAM] Write OK: %zd bytes\n", wr);

    ctx->neurax->reg_data_sc = 0;  /* clear data length in words */
    ctx->neurax->reg_data_sc = FPGA_RAM_WORDS << 16u;  /* set data length in words */
    ctx->neurax->reg_data_sc = 1 << 31; /* set data start bit */
    __sync_synchronize();

    /* 4. Poll s2m: data streams through the FPGA pipeline to the RX buffer */
    printf("[full-RAM] Waiting for readback to complete...\n");
    if (poll_dma_complete(ctx->s2m, DMA_TIMEOUT_US) < 0) {
        fprintf(stderr, "[full-RAM] Readback timed out\n");
        return -1;
    }
    printf("[full-RAM] Readback OK\n");

    /* 5. Verify every word */
    volatile uint32_t *rx = (volatile uint32_t *)ctx->rx_buf;
    uint32_t errors = 0;
    for (uint32_t i = 0; i < n_words; i++) {
        uint32_t expected = test_pattern(i);
        uint32_t received = rx[i];
        if (received != expected) {
            if (errors < MAX_PRINT_ERRORS)
                printf("  Address 0x%04"PRIx32":\n"
                       "    expected: 0x%08"PRIx32"\n"
                       "    received: 0x%08"PRIx32"\n",
                       i, expected, received);
            else if (errors == MAX_PRINT_ERRORS)
                printf("  ... (further mismatches suppressed)\n");
            errors++;
        }
    }
    free(tx_data);


    /* 6. Summary */
    printf("\nTotal words tested : %"PRIu32"\n", n_words);
    printf("Errors             : %"PRIu32"\n", errors);
    printf("Result             : %s\n", errors == 0 ? "PASS" : "FAIL");

    return errors == 0 ? 0 : -1;
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

    /* --- Full RAM integrity test ----------------------------------------- */
    if (test_full_ram(&ctx) != 0)
        final_result = 1;

done:
    neurax_uio_deinit(&ctx);
    printf("\nDone.\n");
    return final_result;
}
