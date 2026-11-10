/*
 * neurax_uio_test_pipeline.c — sequential accelerator block integration tests.
 *
 * Each block reads from the fixed RAM input base and writes to the fixed RAM
 * output base. This test therefore reads each stage's output, converts the
 * board's observed Port-A/Port-B byte order, places the result at RAM word 0,
 * and uploads it before starting the next operation.
 *
 * Default run exercises:
 *   1. convolution -> ReLU -> MAX pooling
 *   2. ReLU -> MIN pooling
 *   3. convolution -> SUM pooling
 *
 * On the current 100x100 build, activation is limited to 4096 words. In the
 * three-stage case it transforms the first 4000 words (40 full rows); the
 * remaining rows retain the convolution result and are also checked by pool.
 */
#define NEURAX_UIO_HELPERS_ONLY
#include "neurax_uio_test_conv.c"
#undef NEURAX_UIO_HELPERS_ONLY

#define CMD_OP_POOLING          (1u << 1)
#define CMD_OP_ACTIVATION       (2u << 1)

#define PIPE_INPUT_HEIGHT       100u
#define PIPE_INPUT_WIDTH        100u
#define PIPE_INPUT_WORDS        (PIPE_INPUT_HEIGHT * PIPE_INPUT_WIDTH)
#define PIPE_ACTIVATION_WORDS   4000u
#define PIPE_POOL_SIZE          2u
#define PIPE_POOL_STRIDE        2u
#define PIPE_POOL_HEIGHT        (((PIPE_INPUT_HEIGHT - PIPE_POOL_SIZE) / PIPE_POOL_STRIDE) + 1u)
#define PIPE_POOL_WIDTH         (((PIPE_INPUT_WIDTH - PIPE_POOL_SIZE) / PIPE_POOL_STRIDE) + 1u)
#define PIPE_POOL_WORDS         (PIPE_POOL_HEIGHT * PIPE_POOL_WIDTH)
#define PIPE_CONV_WEIGHT        512 /* Q24.8 2.0 */
#define PIPE_POOL_MAX           0u
#define PIPE_POOL_MIN           2u
#define PIPE_POOL_SUM           3u
#define PIPE_ACTIVATION_RELU    0u

#define CONV_OUTPUT_WORDS PIPE_INPUT_WORDS

static void usleep_us(uint32_t microseconds)
{
    struct timespec ts = {
        .tv_sec = microseconds / 1000000u,
        .tv_nsec = (long)(microseconds % 1000000u) * 1000L
    };
    nanosleep(&ts, NULL);
}

static int32_t pipeline_input_value(uint32_t index)
{
    return ((int32_t)(index % 31u) - 15) * 256;
}

static void initialize_image(uint32_t *image)
{
    memset(image, 0, FPGA_RAM_BYTES);
    for (uint32_t i = 0; i < PIPE_INPUT_WORDS; i++)
        image[RAM_INPUT_BASE + i] = (uint32_t)pipeline_input_value(i);

    image[RAM_WEIGHT_BASE] = PIPE_CONV_WEIGHT;
    image[RAM_BIAS_BASE] = Q24_8_ZERO;
    for (uint32_t i = 0; i < FPGA_RAM_WORDS - RAM_OUTPUT_BASE; i++)
        image[RAM_OUTPUT_BASE + i] = RAM_OUTPUT_SENTINEL;
}

static int upload_logical_image(struct neurax_uio_ctx *ctx,
                                const uint32_t *logical_image)
{
    uint32_t *encoded = malloc(FPGA_RAM_BYTES);
    if (!encoded) {
        perror("pipeline upload allocation");
        return -1;
    }

    for (uint32_t i = 0; i < FPGA_RAM_WORDS; i++)
        encoded[i] = byte_swap32(logical_image[i]);

    ssize_t written = neurax_dma_write(ctx, encoded, FPGA_RAM_BYTES);
    free(encoded);
    if (written != (ssize_t)FPGA_RAM_BYTES) {
        fprintf(stderr, "[pipeline] upload failed: %zd of %u bytes\n",
                written, FPGA_RAM_BYTES);
        return -1;
    }
    usleep_ms(1);
    return 0;
}

static int read_ram_region(struct neurax_uio_ctx *ctx,
                           uint32_t start_word,
                           uint32_t word_count,
                           uint32_t *logical_words)
{
    const uint32_t chunk_words = MSGDMA_MAX_TRANSFER_BYTES / sizeof(uint32_t);
    uint32_t *rx_words = (uint32_t *)ctx->rx_buf;

    for (uint32_t offset = 0; offset < word_count; offset += chunk_words) {
        uint32_t count = word_count - offset;
        if (count > chunk_words)
            count = chunk_words;

        if (readback_ram_chunk(ctx, start_word + offset, count) < 0) {
            fprintf(stderr, "[pipeline] readback failed at RAM word %u\n",
                    start_word + offset);
            return -1;
        }

        for (uint32_t i = 0; i < count; i++)
            logical_words[offset + i] =
                byte_swap32(rx_words[start_word + offset + i]);
    }
    return 0;
}

static int prepare_accelerator(struct neurax_uio_ctx *ctx)
{
    ctx->neurax->reg_cmd = 0;
    usleep_ms(1);
    ctx->neurax->reg_cmd = CMD_ENABLE;
    usleep_ms(1);
    ctx->neurax->reg_batch_size = 1;
    return 0;
}

static int start_and_wait(struct neurax_uio_ctx *ctx,
                          uint32_t operation_select,
                          const char *operation_name)
{
    ctx->neurax->reg_cmd = CMD_ENABLE;
    usleep_ms(1);
    ctx->neurax->reg_cmd = CMD_ENABLE | operation_select | CMD_START;
    __sync_synchronize();
    usleep_us(10);
    ctx->neurax->reg_cmd = CMD_ENABLE | operation_select;

    volatile uint32_t status = ctx->neurax->reg_status;
    if (!(status & STATUS_BUSY) || (status & STATUS_DONE)) {
        fprintf(stderr,
                "[pipeline] %s START was not accepted (STATUS=0x%08x)\n",
                operation_name, status);
        return -1;
    }

    uint64_t deadline = now_us() + DMA_TIMEOUT_US;
    do {
        status = ctx->neurax->reg_status;
        if (status & STATUS_DONE)
            return 0;
        usleep_ms(1);
    } while (now_us() < deadline);

    fprintf(stderr, "[pipeline] %s timed out (STATUS=0x%08x)\n",
            operation_name, status);
    return -1;
}

static int report_mismatches(const char *label,
                             const int32_t *actual,
                             const int32_t *expected,
                             uint32_t count)
{
    uint32_t errors = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (actual[i] != expected[i]) {
            if (errors < 8)
                fprintf(stderr,
                        "[pipeline] %s[%u]: got %" PRId32 ", expected %" PRId32 "\n",
                        label, i, actual[i], expected[i]);
            errors++;
        }
    }

    printf("%s: %s (%u mismatches / %u words)\n",
           label, errors == 0 ? "PASS" : "FAIL", errors, count);
    return errors == 0 ? 0 : -1;
}

static int run_convolution_stage(struct neurax_uio_ctx *ctx,
                                 const uint32_t *image,
                                 int32_t *output)
{
    int32_t expected[PIPE_INPUT_WORDS];

    if (prepare_accelerator(ctx) < 0)
        return -1;
    ctx->neurax->reg_conv_config_0 = (1u << 24) | 1u; /* 1x1, stride 1 */
    ctx->neurax->reg_conv_config_1 = (1u << 8) | 1u; /* one input/output channel */

    if (upload_logical_image(ctx, image) < 0 ||
        start_and_wait(ctx, 0, "1x1 convolution") < 0 ||
        read_ram_region(ctx, RAM_OUTPUT_BASE, CONV_OUTPUT_WORDS,
                        (uint32_t *)output) < 0)
        return -1;

    for (uint32_t i = 0; i < PIPE_INPUT_WORDS; i++)
        expected[i] = (int32_t)image[RAM_INPUT_BASE + i] * 2;

    return report_mismatches("convolution output", output, expected,
                             PIPE_INPUT_WORDS);
}

static int run_activation_stage(struct neurax_uio_ctx *ctx,
                                const uint32_t *image,
                                int32_t *output)
{
    int32_t expected[PIPE_ACTIVATION_WORDS];

    if (prepare_accelerator(ctx) < 0)
        return -1;
    ctx->neurax->reg_activation_config =
        (PIPE_ACTIVATION_WORDS << 8) | PIPE_ACTIVATION_RELU;
    ctx->neurax->reg_activation_alpha = 26u;

    if (upload_logical_image(ctx, image) < 0 ||
        start_and_wait(ctx, CMD_OP_ACTIVATION, "ReLU") < 0 ||
        read_ram_region(ctx, RAM_OUTPUT_BASE, PIPE_ACTIVATION_WORDS,
                        (uint32_t *)output) < 0)
        return -1;

    for (uint32_t i = 0; i < PIPE_ACTIVATION_WORDS; i++) {
        int32_t value = (int32_t)image[RAM_INPUT_BASE + i];
        expected[i] = value > 0 ? value : 0;
    }

    return report_mismatches("ReLU output", output, expected,
                             PIPE_ACTIVATION_WORDS);
}

static int save_pipeline_ram_snapshot(struct neurax_uio_ctx *ctx)
{
    uint32_t *snapshot = malloc(FPGA_RAM_BYTES);
    if (!snapshot) {
        perror("pipeline snapshot allocation");
        return -1;
    }

    const uint32_t chunk_words = MSGDMA_MAX_TRANSFER_BYTES / sizeof(uint32_t);
    uint32_t *rx_words = (uint32_t *)ctx->rx_buf;
    for (uint32_t start = 0; start < FPGA_RAM_WORDS; start += chunk_words) {
        uint32_t count = FPGA_RAM_WORDS - start;
        if (count > chunk_words)
            count = chunk_words;
        if (readback_ram_chunk(ctx, start, count) < 0) {
            free(snapshot);
            return -1;
        }
        memcpy(snapshot + start, rx_words + start, count * sizeof(uint32_t));
    }

    int result = save_ram_readback(snapshot, FPGA_RAM_WORDS);
    free(snapshot);
    if (result == 0)
        printf("Full RAM snapshot saved to res.txt.\n");
    return result;
}

static int32_t pool_reference(const uint32_t *image,
                              uint32_t output_index,
                              uint32_t pool_type)
{
    const uint32_t out_y = output_index / PIPE_POOL_WIDTH;
    const uint32_t out_x = output_index % PIPE_POOL_WIDTH;
    const uint32_t input_index = (out_y * PIPE_POOL_STRIDE) * PIPE_INPUT_WIDTH
                               + out_x * PIPE_POOL_STRIDE;
    int32_t values[4] = {
        (int32_t)image[input_index],
        (int32_t)image[input_index + 1u],
        (int32_t)image[input_index + PIPE_INPUT_WIDTH],
        (int32_t)image[input_index + PIPE_INPUT_WIDTH + 1u]
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

    switch (pool_type) {
    case PIPE_POOL_MAX:
        return maximum;
    case PIPE_POOL_MIN:
        return minimum;
    case PIPE_POOL_SUM:
        return sum;
    default:
        return 0;
    }
}

static int run_pool_stage(struct neurax_uio_ctx *ctx,
                          const uint32_t *image,
                          uint32_t pool_type,
                          const char *pool_name)
{
    int32_t actual[PIPE_POOL_WORDS];
    int32_t expected[PIPE_POOL_WORDS];
    const uint32_t pool_config = (1u << 24)
                               | (pool_type << 16)
                               | (PIPE_POOL_STRIDE << 8)
                               | PIPE_POOL_SIZE;

    if (prepare_accelerator(ctx) < 0)
        return -1;
    ctx->neurax->reg_pool_config = pool_config;

    if (upload_logical_image(ctx, image) < 0 ||
        start_and_wait(ctx, CMD_OP_POOLING, pool_name) < 0 ||
        read_ram_region(ctx, RAM_OUTPUT_BASE, PIPE_POOL_WORDS,
                        (uint32_t *)actual) < 0)
        return -1;

    for (uint32_t i = 0; i < PIPE_POOL_WORDS; i++)
        expected[i] = pool_reference(image, i, pool_type);

    char label[64];
    snprintf(label, sizeof(label), "%s pooling output", pool_name);
    int result = report_mismatches(label, actual, expected, PIPE_POOL_WORDS);
    if (save_pipeline_ram_snapshot(ctx) < 0)
        return -1;
    return result;
}

enum pipeline_case {
    CASE_CONV_ACT_POOL,
    CASE_ACT_POOL,
    CASE_CONV_POOL
};

static int run_pipeline_case(struct neurax_uio_ctx *ctx,
                             enum pipeline_case test_case)
{
    uint32_t *image = calloc(FPGA_RAM_WORDS, sizeof(*image));
    int32_t *conv_output = malloc(PIPE_INPUT_WORDS * sizeof(*conv_output));
    int32_t *activation_output =
        malloc(PIPE_ACTIVATION_WORDS * sizeof(*activation_output));
    if (!image || !conv_output || !activation_output) {
        perror("pipeline test allocation");
        free(activation_output);
        free(conv_output);
        free(image);
        return -1;
    }

    initialize_image(image);
    int result = 0;

    if (test_case == CASE_CONV_ACT_POOL || test_case == CASE_CONV_POOL) {
        printf("\n[pipeline] Running 1x1 convolution (weight=2.0)\n");
        if (run_convolution_stage(ctx, image, conv_output) < 0) {
            result = -1;
            goto done;
        }
        for (uint32_t i = 0; i < PIPE_INPUT_WORDS; i++)
            image[RAM_INPUT_BASE + i] = (uint32_t)conv_output[i];
    }

    if (test_case == CASE_CONV_ACT_POOL || test_case == CASE_ACT_POOL) {
        printf("[pipeline] Running ReLU on the first %u words\n",
               PIPE_ACTIVATION_WORDS);
        if (run_activation_stage(ctx, image, activation_output) < 0) {
            result = -1;
            goto done;
        }
        for (uint32_t i = 0; i < PIPE_ACTIVATION_WORDS; i++)
            image[RAM_INPUT_BASE + i] = (uint32_t)activation_output[i];
    }

    uint32_t pool_type;
    const char *pool_name;
    if (test_case == CASE_CONV_ACT_POOL) {
        pool_type = PIPE_POOL_MAX;
        pool_name = "MAX";
        printf("[pipeline] Running MAX pooling (top 40 rows passed all 3 stages)\n");
    } else if (test_case == CASE_ACT_POOL) {
        pool_type = PIPE_POOL_MIN;
        pool_name = "MIN";
        printf("[pipeline] Running MIN pooling (first 40 rows were activated)\n");
    } else {
        pool_type = PIPE_POOL_SUM;
        pool_name = "SUM";
        printf("[pipeline] Running SUM pooling after convolution\n");
    }

    if (run_pool_stage(ctx, image, pool_type, pool_name) < 0)
        result = -1;

done:
    free(activation_output);
    free(conv_output);
    free(image);
    return result;
}

int main(int argc, char **argv)
{
    struct neurax_uio_ctx ctx;
    enum pipeline_case cases[] = {
        CASE_CONV_ACT_POOL,
        CASE_ACT_POOL,
        CASE_CONV_POOL
    };
    const char *names[] = {
        "conv-act-pool",
        "act-pool",
        "conv-pool"
    };
    int first_case = 0;
    int last_case = (int)(sizeof(cases) / sizeof(cases[0]));
    bool selected_case = false;

    if (argc > 2) {
        fprintf(stderr,
                "Usage: %s [conv-act-pool|act-pool|conv-pool]\n", argv[0]);
        return 2;
    }
    if (argc == 2) {
        for (int i = 0; i < last_case; i++) {
            if (strcmp(argv[1], names[i]) == 0) {
                first_case = i;
                last_case = i + 1;
                selected_case = true;
                break;
            }
        }
        if (!selected_case) {
            fprintf(stderr,
                    "Usage: %s [conv-act-pool|act-pool|conv-pool]\n", argv[0]);
            return 2;
        }
    }

    printf("=== Neurax sequential block integration tests ===\n");
    if (neurax_uio_init(&ctx) < 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    int result = 0;
    for (int i = first_case; i < last_case; i++) {
        printf("\n=== Pipeline: %s ===\n", names[i]);
        if (run_pipeline_case(&ctx, cases[i]) < 0) {
            printf("Pipeline %s: FAIL\n", names[i]);
            result = 1;
            break;
        }
        printf("Pipeline %s: PASS\n", names[i]);
    }

    neurax_uio_deinit(&ctx);
    return result;
}
