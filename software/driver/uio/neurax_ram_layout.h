/* Runtime RAM layout helpers shared by the Neurax userspace tests. */
#ifndef NEURAX_RAM_LAYOUT_H
#define NEURAX_RAM_LAYOUT_H

#include <stdint.h>

#define FPGA_RAM_WORDS 65536u
#define FPGA_RAM_BYTES (FPGA_RAM_WORDS * sizeof(uint32_t))
#define FPGA_RAM_MAX_WORD_ADDR (FPGA_RAM_WORDS - 1u)

struct neurax_ram_layout {
    uint32_t input_base;
    uint32_t input_words;
    uint32_t weight_base;
    uint32_t weight_words;
    uint32_t bias_base;
    uint32_t bias_words;
    uint32_t output_base;
    uint32_t output_words;
    uint32_t end_word;
};

static inline int neurax_layout_finish(struct neurax_ram_layout *layout,
                                       uint64_t input_words,
                                       uint64_t weight_words,
                                       uint64_t bias_words,
                                       uint64_t output_words)
{
    const uint64_t total_words = input_words + weight_words + bias_words + output_words;
    if (!layout || input_words == 0 || output_words == 0 ||
        total_words > FPGA_RAM_WORDS)
        return -1;

    layout->input_base = 0;
    layout->input_words = (uint32_t)input_words;
    layout->weight_base = (uint32_t)input_words;
    layout->weight_words = (uint32_t)weight_words;
    layout->bias_base = (uint32_t)(input_words + weight_words);
    layout->bias_words = (uint32_t)bias_words;
    layout->output_base = (uint32_t)(input_words + weight_words + bias_words);
    layout->output_words = (uint32_t)output_words;
    layout->end_word = (uint32_t)total_words;
    return 0;
}

static inline int neurax_layout_conv(struct neurax_ram_layout *layout,
                                    uint32_t height, uint32_t width,
                                    uint32_t kernel, uint32_t stride,
                                    uint32_t padding, uint32_t input_channels,
                                    uint32_t output_channels, uint32_t batch)
{
    int64_t out_height, out_width;
    uint64_t input_words, weight_words, output_words;

    if (height == 0 || height > 128 || width == 0 || width > 128 ||
        kernel == 0 || kernel > 11 || stride == 0 || stride > 4 ||
        padding > 5 || input_channels == 0 || input_channels > 16 ||
        output_channels == 0 || output_channels > 16 || batch == 0 || batch > 4 ||
        (uint64_t)height + 2u * padding < kernel ||
        (uint64_t)width + 2u * padding < kernel)
        return -1;

    out_height = ((int64_t)height + 2 * padding - kernel) / stride + 1;
    out_width = ((int64_t)width + 2 * padding - kernel) / stride + 1;
    if (out_height <= 0 || out_height > 128 || out_width <= 0 || out_width > 128)
        return -1;

    input_words = (uint64_t)height * width * input_channels * batch;
    weight_words = (uint64_t)kernel * kernel * input_channels * output_channels;
    output_words = (uint64_t)out_height * (uint64_t)out_width * output_channels * batch;
    return neurax_layout_finish(layout, input_words, weight_words,
                                output_channels, output_words);
}

static inline int neurax_layout_pool(struct neurax_ram_layout *layout,
                                    uint32_t height, uint32_t width,
                                    uint32_t window, uint32_t stride,
                                    uint32_t channels, uint32_t batch)
{
    uint64_t input_words, output_words;
    uint32_t out_height, out_width;

    if (height == 0 || height > 128 || width == 0 || width > 128 ||
        window == 0 || window > 8 || stride == 0 || stride > 8 ||
        window > height || window > width || channels == 0 || channels > 16 ||
        batch == 0 || batch > 4)
        return -1;

    out_height = (height - window) / stride + 1;
    out_width = (width - window) / stride + 1;
    input_words = (uint64_t)height * width * channels * batch;
    output_words = (uint64_t)out_height * out_width * channels * batch;
    return neurax_layout_finish(layout, input_words, 0, 0, output_words);
}

static inline int neurax_layout_activation(struct neurax_ram_layout *layout,
                                           uint32_t tensor_words)
{
    if (tensor_words == 0 || tensor_words > 65535u)
        return -1;
    return neurax_layout_finish(layout, tensor_words, 0, 0, tensor_words);
}

#endif /* NEURAX_RAM_LAYOUT_H */
