#define _POSIX_C_SOURCE 200809L
#define NEURAX_UIO_HELPERS_ONLY
#include "../driver/uio/neurax_uio_test_conv.c"
#undef NEURAX_UIO_HELPERS_ONLY

#include <math.h>

#define CMD_OP_POOLING       (1u << 1)
#define CMD_OP_ACTIVATION    (2u << 1)
#define ACTIVATION_RELU      0u
#define POOL_TYPE_MAX        0u
#define Q24_8_SCALE          256.0f
#define FPGA_IMAGE_WIDTH     100u
#define FPGA_IMAGE_HEIGHT    100u
#define FPGA_WEIGHT_WORDS    3000u
#define FPGA_BIAS_WORDS      16u
#define FPGA_MAX_CHANNELS    4u
#define FPGA_OUTPUT_WORDS    (FPGA_RAM_WORDS - RAM_OUTPUT_BASE)
#define FPGA_ACTIVATION_SIZE 4096u

struct neurax_fpga {
    struct neurax_uio_ctx uio;
};

static char last_error[256];

static int fail(int status, const char *message)
{
    snprintf(last_error, sizeof(last_error), "%s", message);
    return status;
}

static int float_to_q24_8(float value, int32_t *result)
{
    if (!isfinite(value))
        return fail(-1, "Q24.8 input contains NaN or infinity");

    const double scaled = trunc((double)value * Q24_8_SCALE);
    if (scaled < (double)INT32_MIN || scaled > (double)INT32_MAX)
        return fail(-1, "Q24.8 input is outside the signed 32-bit range");

    *result = (int32_t)scaled;
    return 0;
}

static void reset_ram(uint32_t *ram)
{
    memset(ram, 0, FPGA_RAM_BYTES);
}

static int upload_ram(struct neurax_uio_ctx *ctx, const uint32_t *ram)
{
    uint32_t *encoded = malloc(FPGA_RAM_BYTES);
    if (!encoded)
        return fail(-1, "cannot allocate FPGA RAM upload buffer");

    for (uint32_t i = 0; i < FPGA_RAM_WORDS; i++)
        encoded[i] = byte_swap32(ram[i]);

    const ssize_t written = neurax_dma_write(ctx, encoded, FPGA_RAM_BYTES);
    free(encoded);
    if (written != (ssize_t)FPGA_RAM_BYTES)
        return fail(-1, "FPGA RAM upload failed; see UIO diagnostics");
    return 0;
}

static int read_ram_words(struct neurax_uio_ctx *ctx,
                          uint32_t start_word,
                          uint32_t word_count,
                          uint32_t *words)
{
    const uint32_t chunk_words = MSGDMA_MAX_TRANSFER_BYTES / sizeof(uint32_t);
    uint32_t *rx = (uint32_t *)ctx->rx_buf;

    if (start_word > FPGA_RAM_WORDS ||
        word_count > FPGA_RAM_WORDS - start_word)
        return fail(-1, "FPGA RAM readback range is out of bounds");

    for (uint32_t offset = 0; offset < word_count; offset += chunk_words) {
        uint32_t count = word_count - offset;
        if (count > chunk_words)
            count = chunk_words;
        if (readback_ram_chunk(ctx, start_word + offset, count) < 0)
            return fail(-1, "FPGA RAM readback failed; see UIO diagnostics");

        for (uint32_t i = 0; i < count; i++)
            words[offset + i] = byte_swap32(rx[start_word + offset + i]);
    }
    return 0;
}

static int start_operation(struct neurax_uio_ctx *ctx,
                           uint32_t operation,
                           const char *name)
{
    ctx->neurax->reg_cmd = CMD_ENABLE;
    usleep_ms(1);
    ctx->neurax->reg_cmd = CMD_ENABLE | operation | CMD_START;
    __sync_synchronize();
    usleep_ms(1);
    ctx->neurax->reg_cmd = CMD_ENABLE | operation;

    uint32_t status = ctx->neurax->reg_status;
    const uint64_t deadline = now_us() + DMA_TIMEOUT_US;
    do {
        status = ctx->neurax->reg_status;
        if (status & STATUS_DONE)
            return 0;
        usleep_ms(1);
    } while (now_us() < deadline);

    snprintf(last_error, sizeof(last_error),
             "%s timed out (FPGA STATUS=0x%08x)", name, status);
    return -1;
}

static int begin_operation(struct neurax_uio_ctx *ctx,
                           const uint32_t *ram,
                           uint32_t operation)
{
    ctx->neurax->reg_cmd = 0;
    usleep_ms(1);
    ctx->neurax->reg_cmd = CMD_ENABLE;
    usleep_ms(1);
    ctx->neurax->reg_batch_size = 1;
    if (upload_ram(ctx, ram) < 0)
        return -1;
    usleep_ms(1);
    return start_operation(ctx, operation, "FPGA operation");
}

static int validate_float_array(const float *values, size_t count)
{
    if (!values && count != 0)
        return fail(-1, "null tensor pointer");

    for (size_t i = 0; i < count; i++) {
        int32_t ignored;
        if (float_to_q24_8(values[i], &ignored) < 0)
            return -1;
    }
    return 0;
}

static int pack_q24_8(const float *source, uint32_t *destination, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        int32_t value;
        if (float_to_q24_8(source[i], &value) < 0)
            return -1;
        destination[i] = (uint32_t)value;
    }
    return 0;
}

void *neurax_fpga_open(void)
{
    struct neurax_fpga *fpga = calloc(1, sizeof(*fpga));
    if (!fpga) {
        fail(-1, "cannot allocate Neurax FPGA context");
        return NULL;
    }

    if (neurax_uio_init(&fpga->uio) < 0) {
        free(fpga);
        fail(-1, "cannot initialize Neurax UIO; see UIO diagnostics");
        return NULL;
    }

    last_error[0] = '\0';
    return fpga;
}

void neurax_fpga_close(void *handle)
{
    struct neurax_fpga *fpga = handle;
    if (!fpga)
        return;
    neurax_uio_deinit(&fpga->uio);
    free(fpga);
}

const char *neurax_fpga_last_error(void)
{
    return last_error;
}

int neurax_fpga_relu(void *handle,
                     const float *input,
                     size_t count,
                     float *output)
{
    struct neurax_fpga *fpga = handle;
    if (!fpga || !output || count == 0)
        return fail(-1, "invalid ReLU arguments");
    if (validate_float_array(input, count) < 0)
        return -1;

    uint32_t *ram = calloc(FPGA_RAM_WORDS, sizeof(*ram));
    uint32_t *readback = malloc(FPGA_RAM_WORDS * sizeof(*readback));
    if (!ram || !readback) {
        free(readback);
        free(ram);
        return fail(-1, "cannot allocate ReLU buffers");
    }

    for (size_t offset = 0; offset < count; offset += FPGA_ACTIVATION_SIZE) {
        const uint32_t elements = (uint32_t)(
            count - offset > FPGA_ACTIVATION_SIZE
                ? FPGA_ACTIVATION_SIZE : count - offset);
        reset_ram(ram);
        if (pack_q24_8(input + offset, ram + RAM_INPUT_BASE, elements) < 0) {
            free(readback);
            free(ram);
            return -1;
        }

        fpga->uio.neurax->reg_activation_config =
            (elements << 8) | ACTIVATION_RELU;
        fpga->uio.neurax->reg_activation_alpha = 26u;
        if (begin_operation(&fpga->uio, ram, CMD_OP_ACTIVATION) < 0 ||
            read_ram_words(&fpga->uio, RAM_OUTPUT_BASE, elements,
                           readback + RAM_OUTPUT_BASE) < 0) {
            free(readback);
            free(ram);
            return -1;
        }

        for (uint32_t i = 0; i < elements; i++)
            output[offset + i] =
                (float)(int32_t)readback[RAM_OUTPUT_BASE + i] / Q24_8_SCALE;
    }

    free(readback);
    free(ram);
    return 0;
}

int neurax_fpga_conv(void *handle,
                     const float *input,
                     const float *weights,
                     const float *bias,
                     float *output,
                     uint32_t batch,
                     uint32_t input_channels,
                     uint32_t input_height,
                     uint32_t input_width,
                     uint32_t output_channels,
                     uint32_t kernel_height,
                     uint32_t kernel_width,
                     uint32_t stride_height,
                     uint32_t stride_width,
                     uint32_t pad_top,
                     uint32_t pad_left,
                     uint32_t pad_bottom,
                     uint32_t pad_right)
{
    struct neurax_fpga *fpga = handle;
    if (!fpga || !input || !weights || !output || batch != 1 ||
        input_channels != 1 || input_height == 0 || input_width == 0 ||
        output_channels == 0 || kernel_height == 0 ||
        kernel_height != kernel_width || kernel_height > 5 ||
        stride_height == 0 || stride_height != stride_width ||
        stride_height > 4 || input_height + pad_top + pad_bottom > 100 ||
        input_width + pad_left + pad_right > 100)
        return fail(-2, "Conv configuration is unsupported by this FPGA RAM layout");

    const uint32_t kernel = kernel_height;
    const uint32_t stride = stride_height;
    const uint32_t padded_height = input_height + pad_top + pad_bottom;
    const uint32_t padded_width = input_width + pad_left + pad_right;
    if (padded_height < kernel || padded_width < kernel)
        return fail(-2, "Conv kernel is larger than its padded input");

    const uint32_t output_height = (padded_height - kernel) / stride + 1;
    const uint32_t output_width = (padded_width - kernel) / stride + 1;
    const uint32_t hardware_height = (FPGA_IMAGE_HEIGHT - kernel) / stride + 1;
    const uint32_t hardware_width = (FPGA_IMAGE_WIDTH - kernel) / stride + 1;
    const uint64_t hardware_output_words =
        (uint64_t)hardware_height * hardware_width * output_channels;
    const uint64_t weight_words =
        (uint64_t)kernel * kernel * input_channels * output_channels;
    if (output_height > hardware_height || output_width > hardware_width ||
        hardware_output_words > FPGA_OUTPUT_WORDS ||
        weight_words > FPGA_WEIGHT_WORDS ||
        output_channels > FPGA_BIAS_WORDS ||
        output_channels > FPGA_MAX_CHANNELS)
        return fail(-2, "Conv output or weights exceed the FPGA RAM regions");

    const size_t input_count = (size_t)input_height * input_width;
    const size_t weight_count = (size_t)output_channels * kernel * kernel;
    if (validate_float_array(input, input_count) < 0 ||
        validate_float_array(weights, weight_count) < 0 ||
        (bias && validate_float_array(bias, output_channels) < 0))
        return -1;

    uint32_t *ram = calloc(FPGA_RAM_WORDS, sizeof(*ram));
    uint32_t *readback = malloc(FPGA_RAM_WORDS * sizeof(*readback));
    if (!ram || !readback) {
        free(readback);
        free(ram);
        return fail(-1, "cannot allocate convolution buffers");
    }

    for (uint32_t y = 0; y < input_height; y++) {
        for (uint32_t x = 0; x < input_width; x++) {
            int32_t value;
            const size_t source_index = (size_t)y * input_width + x;
            if (float_to_q24_8(input[source_index], &value) < 0) {
                free(readback);
                free(ram);
                return -1;
            }
            const size_t destination_index =
                (size_t)(y + pad_top) * FPGA_IMAGE_WIDTH + x + pad_left;
            ram[RAM_INPUT_BASE + destination_index] = (uint32_t)value;
        }
    }

    for (uint32_t kh = 0; kh < kernel; kh++) {
        for (uint32_t kw = 0; kw < kernel; kw++) {
            for (uint32_t oc = 0; oc < output_channels; oc++) {
                const size_t source_index =
                    ((size_t)oc * kernel + kh) * kernel + kw;
                const size_t destination_index =
                    ((size_t)kh * kernel + kw) * output_channels + oc;
                int32_t value;
                if (float_to_q24_8(weights[source_index], &value) < 0) {
                    free(readback);
                    free(ram);
                    return -1;
                }
                ram[RAM_WEIGHT_BASE + destination_index] = (uint32_t)value;
            }
        }
    }

    if (bias && pack_q24_8(bias, ram + RAM_BIAS_BASE, output_channels) < 0) {
        free(readback);
        free(ram);
        return -1;
    }

    fpga->uio.neurax->reg_conv_config_0 =
        (kernel << 24) | stride;
    fpga->uio.neurax->reg_conv_config_1 =
        (output_channels << 8) | input_channels;
    if (begin_operation(&fpga->uio, ram, 0) < 0) {
        free(readback);
        free(ram);
        return -1;
    }

    const uint32_t output_words = (uint32_t)hardware_output_words;
    if (read_ram_words(&fpga->uio, RAM_OUTPUT_BASE, output_words,
                       readback + RAM_OUTPUT_BASE) < 0) {
        free(readback);
        free(ram);
        return -1;
    }

    for (uint32_t y = 0; y < output_height; y++) {
        for (uint32_t x = 0; x < output_width; x++) {
            for (uint32_t oc = 0; oc < output_channels; oc++) {
                const size_t hardware_index =
                    ((size_t)y * hardware_width + x) * output_channels + oc;
                const size_t output_index =
                    ((size_t)oc * output_height + y) * output_width + x;
                output[output_index] =
                    (float)(int32_t)readback[RAM_OUTPUT_BASE + hardware_index]
                    / Q24_8_SCALE;
            }
        }
    }

    free(readback);
    free(ram);
    return 0;
}

int neurax_fpga_maxpool(void *handle,
                        const float *input,
                        float *output,
                        uint32_t batch,
                        uint32_t channels,
                        uint32_t input_height,
                        uint32_t input_width,
                        uint32_t kernel_height,
                        uint32_t kernel_width,
                        uint32_t stride_height,
                        uint32_t stride_width,
                        uint32_t pad_top,
                        uint32_t pad_left,
                        uint32_t pad_bottom,
                        uint32_t pad_right)
{
    struct neurax_fpga *fpga = handle;
    if (!fpga || !input || !output || batch != 1 || channels != 1 ||
        input_height == 0 || input_width == 0 || kernel_height == 0 ||
        kernel_height != kernel_width || kernel_height > 8 ||
        stride_height == 0 || stride_height != stride_width ||
        stride_height > 8 || input_height + pad_top + pad_bottom > 100 ||
        input_width + pad_left + pad_right > 100)
        return fail(-2, "MaxPool configuration is unsupported by this FPGA RAM layout");

    const uint32_t kernel = kernel_height;
    const uint32_t stride = stride_height;
    const uint32_t padded_height = input_height + pad_top + pad_bottom;
    const uint32_t padded_width = input_width + pad_left + pad_right;
    if (padded_height < kernel || padded_width < kernel)
        return fail(-2, "MaxPool window is larger than its padded input");

    const uint32_t output_height = (padded_height - kernel) / stride + 1;
    const uint32_t output_width = (padded_width - kernel) / stride + 1;
    const uint32_t hardware_height = (FPGA_IMAGE_HEIGHT - kernel) / stride + 1;
    const uint32_t hardware_width = (FPGA_IMAGE_WIDTH - kernel) / stride + 1;
    const uint64_t hardware_output_words =
        (uint64_t)hardware_height * hardware_width;
    if (output_height > hardware_height || output_width > hardware_width ||
        hardware_output_words > FPGA_OUTPUT_WORDS)
        return fail(-2, "MaxPool output exceeds the FPGA RAM regions");

    if (validate_float_array(input, (size_t)input_height * input_width) < 0)
        return -1;

    uint32_t *ram = calloc(FPGA_RAM_WORDS, sizeof(*ram));
    uint32_t *readback = malloc(FPGA_RAM_WORDS * sizeof(*readback));
    if (!ram || !readback) {
        free(readback);
        free(ram);
        return fail(-1, "cannot allocate MaxPool buffers");
    }

    for (uint32_t y = 0; y < padded_height; y++) {
        for (uint32_t x = 0; x < padded_width; x++) {
            const size_t index = (size_t)y * FPGA_IMAGE_WIDTH + x;
            ram[RAM_INPUT_BASE + index] = (uint32_t)INT32_MIN;
        }
    }
    for (uint32_t y = 0; y < input_height; y++) {
        for (uint32_t x = 0; x < input_width; x++) {
            int32_t value;
            if (float_to_q24_8(input[(size_t)y * input_width + x],
                               &value) < 0) {
                free(readback);
                free(ram);
                return -1;
            }
            const size_t index =
                (size_t)(y + pad_top) * FPGA_IMAGE_WIDTH + x + pad_left;
            ram[RAM_INPUT_BASE + index] = (uint32_t)value;
        }
    }

    fpga->uio.neurax->reg_pool_config =
        (1u << 24) | (POOL_TYPE_MAX << 16) | (stride << 8) | kernel;
    if (begin_operation(&fpga->uio, ram, CMD_OP_POOLING) < 0) {
        free(readback);
        free(ram);
        return -1;
    }

    if (read_ram_words(&fpga->uio, RAM_OUTPUT_BASE,
                       (uint32_t)hardware_output_words,
                       readback + RAM_OUTPUT_BASE) < 0) {
        free(readback);
        free(ram);
        return -1;
    }

    for (uint32_t y = 0; y < output_height; y++) {
        for (uint32_t x = 0; x < output_width; x++) {
            const size_t hardware_index = (size_t)y * hardware_width + x;
            const size_t output_index = (size_t)y * output_width + x;
            output[output_index] =
                (float)(int32_t)readback[RAM_OUTPUT_BASE + hardware_index]
                / Q24_8_SCALE;
        }
    }

    free(readback);
    free(ram);
    return 0;
}
