/*
 * neurax_uio_test_pooling.c — RAM-backed pooling board test.
 *
 * Reuses the UIO, DMA, and RAM readback helpers from the convolution test,
 * then checks MAX, AVERAGE, MIN, and SUM pooling against CPU references.
 */
#define NEURAX_UIO_HELPERS_ONLY
#include "neurax_uio_test_conv.c"
#undef NEURAX_UIO_HELPERS_ONLY

#define CMD_OP_POOLING       (1u << 1) /* OP_SELECT = 1 */
#define POOL_INPUT_HEIGHT    100u
#define POOL_INPUT_WIDTH     100u
#define POOL_SIZE            2u
#define POOL_STRIDE          2u
#define POOL_CHANNELS        1u
#define POOL_OUTPUT_HEIGHT   (((POOL_INPUT_HEIGHT - POOL_SIZE) / POOL_STRIDE) + 1u)
#define POOL_OUTPUT_WIDTH    (((POOL_INPUT_WIDTH - POOL_SIZE) / POOL_STRIDE) + 1u)
#define POOL_OUTPUT_WORDS    (POOL_OUTPUT_HEIGHT * POOL_OUTPUT_WIDTH * POOL_CHANNELS)
#define POOL_TYPE_MAX        0u
#define POOL_TYPE_AVERAGE    1u
#define POOL_TYPE_MIN        2u
#define POOL_TYPE_SUM        3u

static void usleep_us(uint32_t microseconds)
{
    struct timespec ts = {
        .tv_sec = microseconds / 1000000u,
        .tv_nsec = (long)(microseconds % 1000000u) * 1000L
    };
    nanosleep(&ts, NULL);
}

static int32_t pooling_input_value(uint32_t index)
{
    /* Signed Q24.8 values, all multiples of 1.0 for exact 2x2 averages. */
    return ((int32_t)(index % 17u) - 8) * 256;
}

static int32_t pooling_reference(uint32_t out_y, uint32_t out_x, uint32_t type)
{
    const uint32_t in_index = (out_y * POOL_STRIDE) * POOL_INPUT_WIDTH
                            + out_x * POOL_STRIDE;
    const int32_t values[4] = {
        pooling_input_value(in_index),
        pooling_input_value(in_index + 1u),
        pooling_input_value(in_index + POOL_INPUT_WIDTH),
        pooling_input_value(in_index + POOL_INPUT_WIDTH + 1u)
    };
    int32_t sum = values[0];
    int32_t minimum = values[0];
    int32_t maximum = values[0];

    for (uint32_t i = 1; i < 4; i++) {
        sum += values[i];
        if (values[i] < minimum)
            minimum = values[i];
        if (values[i] > maximum)
            maximum = values[i];
    }

    switch (type) {
    case POOL_TYPE_MAX:
        return maximum;
    case POOL_TYPE_AVERAGE:
        return sum / 4;
    case POOL_TYPE_MIN:
        return minimum;
    case POOL_TYPE_SUM:
        return sum;
    default:
        return 0;
    }
}

static int run_pooling(struct neurax_uio_ctx *ctx)
{
    const uint32_t n_words = FPGA_RAM_WORDS;
    const uint32_t n_bytes = FPGA_RAM_BYTES;
    uint32_t *tx_data = calloc(n_words, sizeof(*tx_data));
    uint32_t *rx = malloc(n_bytes);
    if (!tx_data || !rx) {
        perror("pooling buffer allocation");
        free(tx_data);
        free(rx);
        return -1;
    }

    printf("\n=== Pooling test (%ux%u input, %ux%u window, stride %u) ===\n",
           POOL_INPUT_HEIGHT, POOL_INPUT_WIDTH, POOL_SIZE, POOL_SIZE,
           POOL_STRIDE);

    for (uint32_t i = 0; i < POOL_INPUT_HEIGHT * POOL_INPUT_WIDTH; i++)
        tx_data[RAM_INPUT_BASE + i] = (uint32_t)pooling_input_value(i);
    for (uint32_t i = 0; i < POOL_OUTPUT_WORDS; i++)
        tx_data[RAM_OUTPUT_BASE + i] = RAM_OUTPUT_SENTINEL;

    /* Preserve the tested board's Port B byte-lane reversal. The explicit
     * FPGA pointer-reset handshake now places payload word 0 at RAM[0], so do
     * not shift the image to compensate for the old SOF first-beat loss. */
    for (uint32_t word = 0; word < n_words; word++)
        tx_data[word] = byte_swap32(tx_data[word]);

    ctx->neurax->reg_cmd = 0;
    usleep_ms(1);
    ctx->neurax->reg_cmd = CMD_ENABLE;
    usleep_ms(1);
    ctx->neurax->reg_batch_size = 1;

    ssize_t written = neurax_dma_write(ctx, tx_data, n_bytes);
    if (written < 0 || (size_t)written != n_bytes) {
        fprintf(stderr, "[pooling] RAM upload failed: %zd of %u bytes\n",
                written, n_bytes);
        free(rx);
        free(tx_data);
        return -1;
    }
    usleep_ms(1);

    static const uint32_t pool_types[] = {
        POOL_TYPE_MAX, POOL_TYPE_AVERAGE, POOL_TYPE_MIN, POOL_TYPE_SUM
    };
    static const char *pool_names[] = { "MAX", "AVERAGE", "MIN", "SUM" };
    bool all_passed = true;
    bool have_readback = false;
    bool saved_failure_readback = false;
    uint32_t input_errors = 0;

    for (size_t mode = 0; mode < sizeof(pool_types) / sizeof(pool_types[0]); mode++) {
        const uint32_t type = pool_types[mode];
        const uint32_t pool_config = (POOL_CHANNELS << 24)
                                   | (type << 16)
                                   | (POOL_STRIDE << 8)
                                   | POOL_SIZE;
        ctx->neurax->reg_pool_config = pool_config;
        ctx->neurax->reg_cmd = CMD_ENABLE;
        usleep_ms(1);

        /* Pooling is OP_SELECT=1; START is edge-detected and must be cleared. */
        ctx->neurax->reg_cmd = CMD_ENABLE | CMD_OP_POOLING | CMD_START;
        __sync_synchronize();
        usleep_us(10);
        ctx->neurax->reg_cmd = CMD_ENABLE | CMD_OP_POOLING;

        volatile uint32_t status = ctx->neurax->reg_status;
        if (!(status & STATUS_BUSY) || (status & STATUS_DONE)) {
            fprintf(stderr,
                    "[pooling] %s START was not accepted (STATUS=0x%08x)\n",
                    pool_names[mode], status);
            all_passed = false;
            break;
        }

        uint64_t deadline = now_us() + DMA_TIMEOUT_US;
        do {
            status = ctx->neurax->reg_status;
            if (status & STATUS_DONE)
                break;
            usleep_ms(1);
        } while (now_us() < deadline);
        if (!(status & STATUS_DONE)) {
            fprintf(stderr, "[pooling] %s TIMEOUT (STATUS=0x%08x)\n",
                    pool_names[mode], status);
            all_passed = false;
            break;
        }

        bool readback_ok = true;
        const uint32_t words_per_chunk =
            MSGDMA_MAX_TRANSFER_BYTES / sizeof(uint32_t);
        for (uint32_t start_word = 0; start_word < n_words;
             start_word += words_per_chunk) {
            uint32_t word_count = n_words - start_word;
            if (word_count > words_per_chunk)
                word_count = words_per_chunk;
            if (readback_ram_chunk(ctx, start_word, word_count) < 0) {
                fprintf(stderr,
                        "[pooling] RAM readback failed at word %u\n",
                        start_word);
                all_passed = false;
                readback_ok = false;
                break;
            }
            usleep_ms(1);
        }
        if (!readback_ok)
            break;

        memcpy(rx, ctx->rx_buf, n_bytes);
        have_readback = true;
        uint32_t output_errors = 0;
        uint32_t untouched_outputs = 0;

        if (mode == 0) {
            for (uint32_t i = 0; i < POOL_INPUT_HEIGHT * POOL_INPUT_WIDTH; i++) {
                const uint32_t expected_input = byte_swap32(
                    (uint32_t)pooling_input_value(i));
                if (rx[RAM_INPUT_BASE + i] != expected_input) {
                    if (input_errors < 8)
                        fprintf(stderr,
                                "[pooling] input RAM[%u]: got 0x%08" PRIx32
                                ", expected 0x%08" PRIx32 "\n",
                                i, rx[RAM_INPUT_BASE + i], expected_input);
                    input_errors++;
                }
            }
        }

        for (uint32_t y = 0; y < POOL_OUTPUT_HEIGHT; y++) {
            for (uint32_t x = 0; x < POOL_OUTPUT_WIDTH; x++) {
                const uint32_t index = y * POOL_OUTPUT_WIDTH + x;
                const uint32_t output = rx[RAM_OUTPUT_BASE + index];
                const uint32_t expected = byte_swap32(
                    (uint32_t)pooling_reference(y, x, type));

                if (output == byte_swap32(RAM_OUTPUT_SENTINEL)) {
                    untouched_outputs++;
                } else if (output != expected) {
                    if (output_errors < 8)
                        fprintf(stderr,
                                "[pooling] %s output[%u,%u]: got 0x%08" PRIx32
                                ", expected 0x%08" PRIx32 "\n",
                                pool_names[mode], y, x, output, expected);
                    output_errors++;
                }
            }
        }

        const bool passed = output_errors == 0 && untouched_outputs == 0;
        printf("%s pooling: %s (%u output mismatches, %u untouched outputs)\n",
               pool_names[mode], passed ? "PASS" : "FAIL",
               output_errors, untouched_outputs);
        if (!passed) {
            all_passed = false;
            if (!saved_failure_readback) {
                if (save_ram_readback(rx, n_words) < 0)
                    all_passed = false;
                else
                    printf("%s failure readback saved to res.txt.\n",
                           pool_names[mode]);
                saved_failure_readback = true;
            }
        }
    }

    if (input_errors != 0) {
        printf("Input RAM integrity: FAIL (%u mismatches)\n", input_errors);
        all_passed = false;
    }

    if (have_readback && !saved_failure_readback) {
        if (save_ram_readback(rx, n_words) < 0)
            all_passed = false;
        else
            printf("RAM readback saved to res.txt (including failed checks).\n");
    }

    free(rx);
    free(tx_data);
    return all_passed ? 0 : -1;
}

int main(void)
{
    struct neurax_uio_ctx ctx;

    printf("=== Neurax pooling board test ===\n");
    if (neurax_uio_init(&ctx) < 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    int result = run_pooling(&ctx);
    neurax_uio_deinit(&ctx);
    return result == 0 ? 0 : 1;
}
