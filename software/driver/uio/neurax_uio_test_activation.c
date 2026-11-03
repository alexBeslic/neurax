/*
 * neurax_uio_test_activation.c — RAM-backed ReLU/Linear activation board test.
 *
 * Reuse the proven UIO, DMA, and RAM readback helpers from the convolution
 * test, but run activation operations and compare their outputs with CPU
 * references.
 */
#define NEURAX_UIO_HELPERS_ONLY
#include "neurax_uio_test_conv.c"
#undef NEURAX_UIO_HELPERS_ONLY

#define CMD_OP_ACTIVATION       (2u << 1) /* OP_SELECT = 2 */
#define ACTIVATION_TENSOR_SIZE  4096u
#define ACTIVATION_RELU         0u
#define ACTIVATION_LINEAR       3u
#define ACTIVATION_ALPHA_Q24_8  26u

static void usleep_us(uint32_t microseconds)
{
    struct timespec ts = {
        .tv_sec = microseconds / 1000000u,
        .tv_nsec = (long)(microseconds % 1000000u) * 1000L
    };
    nanosleep(&ts, NULL);
}

static int32_t activation_input_value(uint32_t index)
{
    /* Include zero, positive, negative, and signed-range boundary values. */
    static const int32_t pattern[] = {
        0, 256, -256, 512, -512, INT32_MAX, INT32_MIN, -1
    };
    return pattern[index % (sizeof(pattern) / sizeof(pattern[0]))];
}

static int run_activation(struct neurax_uio_ctx *ctx)
{
    const uint32_t n_words = FPGA_RAM_WORDS;
    const uint32_t n_bytes = FPGA_RAM_BYTES;
    const uint32_t tensor_size = ACTIVATION_TENSOR_SIZE;
    uint32_t *tx_data = calloc(n_words, sizeof(*tx_data));
    if (!tx_data) {
        perror("calloc tx_data");
        return -1;
    }

        printf("\n=== ReLU/Linear activation test (%u Q24.8 values) ===\n",
            tensor_size);
    for (uint32_t i = 0; i < tensor_size; i++)
        tx_data[RAM_INPUT_BASE + i] = (uint32_t)activation_input_value(i);
    for (uint32_t i = 0; i < tensor_size; i++)
        tx_data[RAM_OUTPUT_BASE + i] = RAM_OUTPUT_SENTINEL;

    /* Preserve the tested board's Port B byte swap and one-word DMA offset. */
    for (uint32_t word = 1; word < n_words; word++)
        tx_data[word - 1] = byte_swap32(tx_data[word]);
    tx_data[n_words - 1] = 0;

    ctx->neurax->reg_cmd = 0;
    usleep_ms(1);
    ctx->neurax->reg_cmd = CMD_ENABLE;
    usleep_ms(1);
    ctx->neurax->reg_activation_alpha = ACTIVATION_ALPHA_Q24_8;
    ctx->neurax->reg_batch_size = 1;

    ssize_t written = neurax_dma_write(ctx, tx_data, n_bytes);
    if (written < 0 || (size_t)written != n_bytes) {
        fprintf(stderr, "[activation] RAM upload failed: %zd of %u bytes\n",
                written, n_bytes);
        free(tx_data);
        return -1;
    }
    usleep_ms(1);

    const uint32_t words_per_chunk =
        MSGDMA_MAX_TRANSFER_BYTES / sizeof(uint32_t);
    uint32_t *rx = malloc(n_bytes);
    if (!rx) {
        perror("malloc rx");
        free(tx_data);
        return -1;
    }
    uint32_t input_errors = 0;
    bool all_passed = true;
    const uint32_t activation_types[] = {
        ACTIVATION_RELU, ACTIVATION_LINEAR
    };
    const char *activation_names[] = { "ReLU", "Linear" };

    for (size_t mode = 0;
         mode < sizeof(activation_types) / sizeof(activation_types[0]);
         mode++) {
        ctx->neurax->reg_activation_config =
            (tensor_size << 8) | activation_types[mode];
        ctx->neurax->reg_cmd = CMD_ENABLE;
        usleep_ms(1);

        /* START is edge-detected; clear it before this short operation ends. */
        ctx->neurax->reg_cmd = CMD_ENABLE | CMD_OP_ACTIVATION | CMD_START;
        __sync_synchronize();
        usleep_us(10);
        ctx->neurax->reg_cmd = CMD_ENABLE | CMD_OP_ACTIVATION;

        volatile uint32_t status = ctx->neurax->reg_status;
        if (!(status & (STATUS_BUSY | STATUS_DONE))) {
            fprintf(stderr,
                    "[activation] %s START was not accepted (STATUS=0x%08x)\n",
                    activation_names[mode], status);
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
            fprintf(stderr, "[activation] %s TIMEOUT (STATUS=0x%08x)\n",
                    activation_names[mode], status);
            all_passed = false;
            break;
        }

        for (uint32_t start_word = 0; start_word < n_words;
             start_word += words_per_chunk) {
            uint32_t word_count = n_words - start_word;
            if (word_count > words_per_chunk)
                word_count = words_per_chunk;
            if (readback_ram_chunk(ctx, start_word, word_count) < 0) {
                fprintf(stderr,
                        "[activation] RAM readback failed at word %u\n",
                        start_word);
                all_passed = false;
                break;
            }
            usleep_ms(1);
        }
        if (!all_passed)
            break;

        memcpy(rx, ctx->rx_buf, n_bytes);
        uint32_t output_errors = 0;
        uint32_t untouched_outputs = 0;

        if (mode == 0) {
            /* RAM[0] is skipped due to the board's first-word DMA offset. */
            for (uint32_t i = 1; i < tensor_size; i++) {
                const uint32_t expected_input =
                    byte_swap32((uint32_t)activation_input_value(i));
                if (rx[RAM_INPUT_BASE + i] != expected_input) {
                    if (input_errors < 8)
                        fprintf(stderr,
                                "[activation] input RAM[%u]: got 0x%08" PRIx32
                                ", expected 0x%08" PRIx32 "\n",
                                i, rx[RAM_INPUT_BASE + i], expected_input);
                    input_errors++;
                }
            }
        }

        /* Output zero depends on the intentionally skipped source RAM[0]. */
        for (uint32_t i = 1; i < tensor_size; i++) {
            const uint32_t output = rx[RAM_OUTPUT_BASE + i];
            const int32_t input = activation_input_value(i);
            const int32_t expected_value = activation_types[mode] == ACTIVATION_RELU
                ? (input > 0 ? input : 0) : input;
            const uint32_t expected = byte_swap32((uint32_t)expected_value);
            if (output == byte_swap32(RAM_OUTPUT_SENTINEL)) {
                untouched_outputs++;
            } else if (output != expected) {
                if (output_errors < 8)
                    fprintf(stderr,
                            "[activation] %s output RAM[%u]: got 0x%08" PRIx32
                            ", expected 0x%08" PRIx32 "\n",
                            activation_names[mode], i, output, expected);
                output_errors++;
            }
        }

        const bool mode_passed = output_errors == 0 && untouched_outputs == 0 &&
                                 (mode != 0 || input_errors == 0);
        printf("%s activation: %s (%u output mismatches, %u untouched outputs)\n",
               activation_names[mode], mode_passed ? "PASS" : "FAIL",
               output_errors, untouched_outputs);
        if (!mode_passed)
            all_passed = false;
    }

    if (all_passed && save_ram_readback(rx, n_words) < 0)
        all_passed = false;
    if (all_passed)
        printf("Final RAM readback saved to res.txt.\n");
    else if (input_errors != 0)
        printf("Input RAM integrity: FAIL (%u mismatches)\n", input_errors);

    free(rx);
    free(tx_data);
    return all_passed ? 0 : -1;
}

int main(void)
{
    struct neurax_uio_ctx ctx;

    printf("=== Neurax ReLU/Linear activation test ===\n");
    if (neurax_uio_init(&ctx) < 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    int result = run_activation(&ctx);
    neurax_uio_deinit(&ctx);
    return result == 0 ? 0 : 1;
}
