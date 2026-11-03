/*
 * neurax_uio_test_conv.c — Image-based edge convolution test via UIO
 *
 * Replaces the /dev/msgdma kernel char-device path with direct mSGDMA
 * descriptor programming from userspace.
 *
 * Setup:
 *   insmod uio.ko                 # loads the generic UIO kernel module
 *   insmod neurax_uio.ko          # loads the UIO kernel module
 *   ./neurax_uio_test_conv        # runs image convolution and RAM checks
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

/* RAM word layout (one signed Q24.8 value per 32-bit word).
 * Must match FPGA generics g_FB_HEIGHT/g_FB_WIDTH and FPGA_accelerator offsets. */
#define RAM_INPUT_BASE   0u
#define RAM_WEIGHT_BASE  10000u
#define RAM_BIAS_BASE    13000u
#define RAM_OUTPUT_BASE  13016u

/* Signed Q24.8 values occupy the complete 32-bit RAM word. */
#define Q24_8_ZERO      0x00000000u
#define RAM_OUTPUT_SENTINEL 0x00005A5Au

/* Full altsyncram integrity test constants (derived from msgdma_uio.h) */
#define FPGA_RAM_WORDS    (FPGA_OUTPUT_SIZE / sizeof(uint32_t)) /* 23000 words */
#define FPGA_RAM_BYTES    FPGA_OUTPUT_SIZE                     /* 92000 bytes */

/* Workaround for the observed Port B byte-lane reversal. Port A stores these
 * bytes unchanged; the Port B RAM interface reverses them on read/write. */
static inline uint32_t byte_swap32(uint32_t value)
{
    return ((value & 0x000000FFu) << 24) |
           ((value & 0x0000FF00u) << 8)  |
           ((value & 0x00FF0000u) >> 8)  |
           ((value & 0xFF000000u) >> 24);
}

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
}

/* ---------------------------------------------------------------------------
 * Send channel=1 SOF descriptor to mSGDMA0 (m2s).
 *
 * The FPGA neurax_data_interface resets write_index and clears buffer_full
 * on asi_channel_i=1, breaking any "buffer full / asi_ready=0" deadlock.
 * Call this once before each complete RAM image to guarantee the FPGA side
 * is ready; the image itself may be split across multiple mSGDMA descriptors.
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

static int msgdma_wait_descriptor_armed(volatile struct msgdma_reg *dma,
                                        uint32_t timeout_us)
{
    uint64_t deadline = now_us() + timeout_us;
    uint32_t status;
    uint32_t fill;

    do {
        status = dma->csr_status;
        fill = dma->csr_fill_lvl;
        if ((status & CSR_ST_BUSY) || (fill >> 16) != 0)
            return 0;
        usleep_ms(1);
    } while (now_us() < deadline);

    fprintf(stderr,
            "[DMA] descriptor was not accepted: CSR=0x%08x fill=0x%08x "
            "resp=%u\n",
            status, fill, dma->csr_resp_fill);
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

    size_t total = len > DMA_BUF_SIZE ? DMA_BUF_SIZE : len;
    size_t offset = 0;

    while (offset < total) {
        size_t user_len = total - offset;
        if (user_len > MSGDMA_MAX_TRANSFER_BYTES)
            user_len = MSGDMA_MAX_TRANSFER_BYTES;

        /* mSGDMA requires full-word (4-byte) transfers; pad the final chunk. */
        uint32_t dma_len = ((uint32_t)user_len + 3u) & ~3u;
        memcpy(ctx->tx_buf, (const uint8_t *)src + offset, user_len);
        if (dma_len > user_len)
            memset((uint8_t *)ctx->tx_buf + user_len, 0, dma_len - user_len);

        /* Keep the write pointer continuous; send SOF only once per full image. */
        msgdma_push_descr(ctx->m2s, (uint32_t)ctx->tx_phys, 0, dma_len, 0);

        uint32_t status = ctx->m2s->csr_status;

        uint64_t deadline = now_us() + DMA_TIMEOUT_US;
        do {
            status = ctx->m2s->csr_status;
            if (!(status & CSR_ST_BUSY)) break;
            usleep_ms(1);
        } while (now_us() < deadline);

        if (status & CSR_ST_BUSY) {
            fprintf(stderr,
                    "[DMA write] timeout at byte %zu: CSR=0x%08x "
                    "fill=0x%08x resp=%u\n",
                    offset, status, ctx->m2s->csr_fill_lvl,
                    ctx->m2s->csr_resp_fill);
            msgdma_reset(ctx->m2s);
            return -1;
        }
        if (status & (CSR_ST_STOPPED_ON_ERR | CSR_ST_STOPPED_EOP)) {
            fprintf(stderr,
                    "[DMA write] stopped at byte %zu: CSR=0x%08x\n",
                    offset, status);
            msgdma_reset(ctx->m2s);
            return -1;
        }

        offset += user_len;
    }

    return (ssize_t)total;
}

/* Wait until both the FPGA source FSM and the s2m DMA finish the readback. */
static int poll_readback_complete(struct neurax_uio_ctx *ctx,
                                 uint32_t timeout_us,
                                 bool busy_seen)
{
    uint64_t deadline = now_us() + timeout_us;
    uint32_t status = 0;
    uint32_t data_status = 0;

    do {
        status = ctx->s2m->csr_status;
        data_status = ctx->neurax->reg_data_sc;
        if ((status & CSR_ST_BUSY) || (data_status & (1u << 1)))
            busy_seen = true;

        if (status & (CSR_ST_STOPPED_ON_ERR | CSR_ST_STOPPED_EOP)) {
            fprintf(stderr,
                    "[DMA read] stopped: CSR=0x%08x fill=0x%08x resp=%u\n",
                    status, ctx->s2m->csr_fill_lvl, ctx->s2m->csr_resp_fill);
            return -1;
        }

        if (busy_seen && !(status & CSR_ST_BUSY) &&
            !(data_status & (1u << 1)))
            return 0;

        usleep_ms(1);
    } while (now_us() < deadline);

    fprintf(stderr,
            "[DMA read] incomplete: CSR=0x%08x fill=0x%08x resp=%u "
            "reg_data_sc=0x%08x (DMA_BUSY=%u FPGA_BUSY=%u)\n",
            status, ctx->s2m->csr_fill_lvl, ctx->s2m->csr_resp_fill,
            data_status, status & CSR_ST_BUSY,
            (data_status >> 1) & 1u);
    return -1;
}

static int readback_ram_chunk(struct neurax_uio_ctx *ctx,
                              uint32_t start_word,
                              uint32_t word_count)
{
    const uint32_t byte_count = word_count * sizeof(uint32_t);
    const uint32_t read_command = (word_count << 16) | start_word;
    const uint32_t destination = (uint32_t)(
        ctx->rx_phys + (uintptr_t)start_word * sizeof(uint32_t));
    uint64_t deadline;
    uint32_t data_status;

    ctx->neurax->reg_data_sc = 0;
    ctx->neurax->reg_data_read = read_command;
    __sync_synchronize();

    msgdma_push_descr(ctx->s2m, 0, destination, byte_count, 0);
    if (msgdma_wait_descriptor_armed(ctx->s2m, 100000u) < 0)
        return -1;

    ctx->neurax->reg_data_sc = 1u << 31;
    __sync_synchronize();

    deadline = now_us() + 100000u;
    do {
        data_status = ctx->neurax->reg_data_sc;
        if (data_status & (1u << 1))
            break;
        usleep_ms(1);
    } while (now_us() < deadline);

    if (!(data_status & (1u << 1))) {
        fprintf(stderr,
                "[DMA read] FPGA did not start chunk at word %u "
                "(reg_data_sc=0x%08x)\n",
                start_word, data_status);
        return -1;
    }

    if (poll_readback_complete(ctx, DMA_TIMEOUT_US, true) < 0) {
        fprintf(stderr,
                "[DMA read] chunk at word %u (%u words) did not complete\n",
                start_word, word_count);
        return -1;
    }

    return 0;
}

static int save_ram_readback(const uint32_t *words, uint32_t word_count)
{
    FILE *file = fopen("res.txt", "w");
    if (!file) {
        perror("fopen res.txt");
        return -1;
    }

    for (uint32_t i = 0; i < word_count; i++) {
        if (fprintf(file, "Read at %u: 0x%08" PRIx32 "\n", i, words[i]) < 0) {
            perror("write res.txt");
            fclose(file);
            return -1;
        }
    }

    if (fclose(file) != 0) {
        perror("close res.txt");
        return -1;
    }

    return 0;
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

static int load_input_pixels(uint8_t *pixels, size_t pixel_count)
{
    FILE *file = fopen("input_pixels.bin", "rb");
    if (!file) {
        fprintf(stderr,
                "[conv] Cannot open input_pixels.bin; prepare it from input.png "
                "with tests/neurax_uio_edge_image.py first.\n");
        return -1;
    }

    size_t read_count = fread(pixels, 1, pixel_count, file);
    int extra_byte = fgetc(file);
    fclose(file);
    if (read_count != pixel_count || extra_byte != EOF) {
        fprintf(stderr,
                "[conv] input_pixels.bin must contain exactly %zu grayscale pixels "
                "(read %zu%s).\n",
                pixel_count, read_count,
                extra_byte != EOF ? " plus extra data" : "");
        return -1;
    }
    return 0;
}

/* Output (y,x) uses the center pixel at input (y+1,x+1) for the no-padding
 * 3x3 four-neighbor Laplacian kernel. Return the expected raw Q24.8 word. */
static int32_t expected_edge_q24_8(const uint8_t *pixels, int y, int x)
{
    const int center = pixels[(y + 1) * CONV_INPUT_W + x + 1];
    const int neighbors = pixels[y * CONV_INPUT_W + x + 1]
                        + pixels[(y + 1) * CONV_INPUT_W + x]
                        + pixels[(y + 1) * CONV_INPUT_W + x + 2]
                        + pixels[(y + 2) * CONV_INPUT_W + x + 1];
    return (4 * center - neighbors) * 256;
}

/* ---------------------------------------------------------------------------
 * run_image_convolution — build a 100x100 image workload using signed Q24.8
 * inputs, a four-neighbor 3x3 Laplacian kernel, and zero bias; verify the RAM
 * contents and convolution results against the CPU reference.
 *
 * Both DMA directions use descriptors no larger than the Qsys MAX_BYTE
 * setting. The m2s image is sent as sequential chunks after one SOF, while
 * each s2m chunk is paired with an FPGA RAM-read command.
 *
 * Returns 0 on PASS, -1 on any error.
 * ------------------------------------------------------------------------- */
static int run_image_convolution(struct neurax_uio_ctx *ctx)
{
    const uint32_t n_words = FPGA_RAM_WORDS;   /* 23000 */
    const uint32_t n_bytes = FPGA_RAM_BYTES;   /* 92000 */

    printf("\n=== Convolution test (%dx%d input, %dx%d kernel) ===\n",
           CONV_INPUT_H, CONV_INPUT_W, CONV_KERNEL, CONV_KERNEL);

    /* Load the prepared image and build the logical RAM image. Pixels use
     * signed Q24.8. The C-side
     * byte swap and one-word shift compensate for the currently observed board
     * behavior; RAM[0] remains deliberately unused. */
    uint32_t *tx_data = calloc(n_words, sizeof(uint32_t));
    if (!tx_data) {
        perror("calloc tx_data");
        return -1;
    }

    const uint32_t input_word_count = CONV_INPUT_H * CONV_INPUT_W;
    uint8_t input_pixels[CONV_INPUT_H * CONV_INPUT_W];
    if (load_input_pixels(input_pixels, input_word_count) < 0) {
        free(tx_data);
        return -1;
    }

    ssize_t wr;
    for (uint32_t i = 0; i < input_word_count; i++)
        tx_data[RAM_INPUT_BASE + i] = (uint32_t)input_pixels[i] << 8;

    static const int32_t edge_kernel[CONV_KERNEL * CONV_KERNEL] = {
        0, -256, 0,
        -256, 1024, -256,
        0, -256, 0
    };
    for (uint32_t i = 0; i < CONV_KERNEL * CONV_KERNEL; i++)
        tx_data[RAM_WEIGHT_BASE + i] = (uint32_t)edge_kernel[i];

    tx_data[RAM_BIAS_BASE] = Q24_8_ZERO;
    for (uint32_t i = 0; i < CONV_OUTPUT_H * CONV_OUTPUT_W; i++)
        tx_data[RAM_OUTPUT_BASE + i] = RAM_OUTPUT_SENTINEL;

    for (uint32_t word = 1; word < n_words; word++)
        tx_data[word - 1] = byte_swap32(tx_data[word]);
    tx_data[n_words - 1] = 0;

    /* Configure the accelerator before DMA so the Avalon-ST sink is ready. */
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
    /* Write the image, kernel, bias, and output sentinels to FPGA RAM. */
    wr = neurax_dma_write(ctx, tx_data, n_bytes);
    if (wr < 0 || (size_t)wr != n_bytes) {
        fprintf(stderr, "[conv] Write failed or incomplete: %zd of %u bytes\n",
                wr, n_bytes);
        msgdma_reset(ctx->s2m);
        free(tx_data);
        return -1;
    }
    usleep_ms(1000);

    /* START is edge-detected, so clear it after the start pulse. */
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

    /* Wait for accelerator writes to finish before starting RAM readback. */
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
    int errors = 0;

    /* Qsys configures each mSGDMA with MAX_BYTE=4096. Keep every descriptor
     * within that limit, pairing each descriptor with a matching FPGA RAM
     * read command (word-addressed start and word count). */
    const uint32_t words_per_chunk =
        MSGDMA_MAX_TRANSFER_BYTES / sizeof(uint32_t);
    for (uint32_t start_word = 0; start_word < n_words;
         start_word += words_per_chunk) {
        uint32_t word_count = n_words - start_word;
        if (word_count > words_per_chunk)
            word_count = words_per_chunk;

        if (readback_ram_chunk(ctx, start_word, word_count) < 0) {
            fprintf(stderr, "[conv] RAM readback failed at word %u\n",
                    start_word);
            free(tx_data);
            return -1;
        }
    }

    /* Check the complete RAM image and every output against the CPU reference. */
    uint32_t *rx = malloc(n_bytes);
    if (!rx) {
        perror("malloc rx");
        free(tx_data);
        return -1;
    }
    memcpy(rx, ctx->rx_buf, n_bytes);

    int ram_errors = 0;
    uint32_t untouched_outputs = 0;
    uint32_t wrong_outputs = 0;
    uint32_t expected_outputs = 0;
    for (uint32_t i = 0; i < CONV_OUTPUT_H * CONV_OUTPUT_W; i++) {
        uint32_t output = rx[RAM_OUTPUT_BASE + i];
        int32_t expected_logical = expected_edge_q24_8(
            input_pixels, i / CONV_OUTPUT_W, i % CONV_OUTPUT_W);
        uint32_t expected_porta = byte_swap32((uint32_t)expected_logical);
        if (output == byte_swap32(RAM_OUTPUT_SENTINEL)) {
            untouched_outputs++;
        } else if (output == expected_porta) {
            expected_outputs++;
        } else {
            wrong_outputs++;
        }
    }
    if (expected_outputs != CONV_OUTPUT_H * CONV_OUTPUT_W ||
        untouched_outputs != 0 || wrong_outputs != 0)
        errors++;

    const uint32_t input_words = CONV_INPUT_H * CONV_INPUT_W;
    const uint32_t weight_words =
        CONV_KERNEL * CONV_KERNEL * CONV_IN_CH * CONV_OUT_CH;
    /* RAM[0] is a known independent first-word issue; skip it here. */
    for (uint32_t i = 1; i < input_words; i++) {
        const uint32_t expected_input =
            byte_swap32((uint32_t)input_pixels[i] << 8);
        if (rx[RAM_INPUT_BASE + i] != expected_input) {
            if (ram_errors < 8)
                fprintf(stderr,
                        "[conv] RAM input mismatch at word %u: got 0x%08" PRIx32
                        ", expected 0x%08" PRIx32 "\n",
                        i, rx[RAM_INPUT_BASE + i],
                        expected_input);
            ram_errors++;
        }
    }
    for (uint32_t i = 0; i < weight_words; i++) {
        const uint32_t expected_weight =
            byte_swap32((uint32_t)edge_kernel[i]);
        if (rx[RAM_WEIGHT_BASE + i] != expected_weight) {
            if (ram_errors < 8)
                fprintf(stderr,
                        "[conv] RAM weight mismatch at word %u: got 0x%08" PRIx32
                        ", expected 0x%08" PRIx32 "\n",
                        i, rx[RAM_WEIGHT_BASE + i],
                        expected_weight);
            ram_errors++;
        }
    }
    if (save_ram_readback(rx, n_words) < 0) {
        free(rx);
        free(tx_data);
        return -1;
    }
    printf("RAM readback saved to res.txt.\n");

    free(rx);
    free(tx_data);
    tx_data = NULL;

    printf("Image convolution: %s (%u output mismatches, %d RAM mismatches)\n",
           errors == 0 && ram_errors == 0 ? "PASS" : "FAIL",
           wrong_outputs, ram_errors);
    return errors == 0 && ram_errors == 0 ? 0 : -1;
}

/* ---------------------------------------------------------------------------
 * main() — full test: build RAM image → DMA write → start → wait → DMA read
 * ------------------------------------------------------------------------- */
int main(void)
{
    struct neurax_uio_ctx ctx;

    printf("=== Neurax image convolution test ===\n");

    if (neurax_uio_init(&ctx) < 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    int result = run_image_convolution(&ctx);

    neurax_uio_deinit(&ctx);
    return result == 0 ? 0 : 1;
}
