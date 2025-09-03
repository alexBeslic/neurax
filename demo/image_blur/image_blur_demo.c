/*
 * NEURAX Image Blur Demo
 * Demonstrates image processing using NEURAX convolution operations
 * Applies Gaussian blur filter to BMP images
 * 
 * Author: NEURAX Team
 */

#include "neurax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

// BMP file structures
#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} bmp_header_t;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} bmp_info_header_t;
#pragma pack(pop)

// Function prototypes
static int load_bmp_image(const char* filename, neurax_tensor_t** tensor);
static int save_bmp_image(const char* filename, neurax_tensor_t* tensor);
static int create_blur_kernel(neurax_tensor_t** kernel, int kernel_size, float sigma);
static int apply_blur_filter(neurax_device_t* device, neurax_tensor_t* input, 
                           neurax_tensor_t* kernel, neurax_tensor_t** output);
static void print_usage(const char* program_name);
static void create_sample_image(const char* filename);

int main(int argc, char* argv[]) {
    printf("NEURAX Image Blur Demo\n");
    printf("======================\n\n");
    
    // Parse command line arguments
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    int kernel_size = 5;  // Default blur kernel size
    float sigma = 1.0f;   // Default Gaussian sigma
    
    // Parse optional parameters
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--kernel-size") == 0 && i + 1 < argc) {
            kernel_size = atoi(argv[++i]);
            if (kernel_size < 3 || kernel_size > 11 || kernel_size % 2 == 0) {
                printf("Error: Kernel size must be odd and between 3-11\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--sigma") == 0 && i + 1 < argc) {
            sigma = atof(argv[++i]);
            if (sigma <= 0.0f) {
                printf("Error: Sigma must be positive\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--create-sample") == 0) {
            printf("Creating sample image...\n");
            create_sample_image(input_file);
            printf("Sample image created: %s\n", input_file);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    printf("Processing: %s -> %s\n", input_file, output_file);
    printf("Blur parameters: kernel_size=%d, sigma=%.2f\n\n", kernel_size, sigma);
    
    // Initialize NEURAX device
    neurax_config_t config = {
        .base_address = 0x43C00000,
        .memory_size = 0x10000,
        .use_hardware = true,
        .max_kernel_size = 11,
        .num_multipliers = 64,
        .data_type = NEURAX_DATA_FLOAT32
    };
    
    neurax_device_t* device = NULL;
    neurax_tensor_t* input_tensor = NULL;
    neurax_tensor_t* blur_kernel = NULL;
    neurax_tensor_t* output_tensor = NULL;
    int result = 0;
    
    printf("Initializing NEURAX device...\n");
    neurax_error_t error = neurax_init(&config, &device);
    if (error != NEURAX_SUCCESS) {
        printf("Failed to initialize device: %s\n", neurax_get_error_string(error));
        return 1;
    }
    
    printf("Loading input image...\n");
    if (load_bmp_image(input_file, &input_tensor) != 0) {
        printf("Failed to load input image: %s\n", input_file);
        result = 1;
        goto cleanup;
    }
    
    printf("Image loaded: %ux%ux%u\n", 
           input_tensor->width, input_tensor->height, input_tensor->channels);
    
    printf("Creating blur kernel (%dx%d, sigma=%.2f)...\n", kernel_size, kernel_size, sigma);
    if (create_blur_kernel(&blur_kernel, kernel_size, sigma) != 0) {
        printf("Failed to create blur kernel\n");
        result = 1;
        goto cleanup;
    }
    
    printf("Applying blur filter...\n");
    clock_t start_time = clock();
    if (apply_blur_filter(device, input_tensor, blur_kernel, &output_tensor) != 0) {
        printf("Failed to apply blur filter\n");
        result = 1;
        goto cleanup;
    }
    clock_t end_time = clock();
    
    double process_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0;
    printf("Blur processing completed in %.2f ms\n", process_time);
    
    printf("Saving output image...\n");
    if (save_bmp_image(output_file, output_tensor) != 0) {
        printf("Failed to save output image: %s\n", output_file);
        result = 1;
        goto cleanup;
    }
    
    printf("\nImage blur completed successfully!\n");
    printf("Output saved to: %s\n", output_file);
    
cleanup:
    if (input_tensor) neurax_tensor_destroy(input_tensor);
    if (blur_kernel) neurax_tensor_destroy(blur_kernel);
    if (output_tensor) neurax_tensor_destroy(output_tensor);
    if (device) neurax_cleanup(device);
    
    return result;
}

static int load_bmp_image(const char* filename, neurax_tensor_t** tensor) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return -1;
    }
    
    bmp_header_t header;
    bmp_info_header_t info;
    
    // Read BMP header
    if (fread(&header, sizeof(header), 1, file) != 1) {
        printf("Error: Cannot read BMP header\n");
        fclose(file);
        return -1;
    }
    
    if (header.type != 0x4D42) { // "BM"
        printf("Error: Not a valid BMP file\n");
        fclose(file);
        return -1;
    }
    
    // Read info header
    if (fread(&info, sizeof(info), 1, file) != 1) {
        printf("Error: Cannot read BMP info header\n");
        fclose(file);
        return -1;
    }
    
    if (info.bits_per_pixel != 24 && info.bits_per_pixel != 32) {
        printf("Error: Only 24-bit and 32-bit BMP files supported\n");
        fclose(file);
        return -1;
    }
    
    int width = info.width;
    int height = abs(info.height);
    int channels = (info.bits_per_pixel == 24) ? 3 : 4;
    
    // Create tensor
    neurax_error_t error = neurax_tensor_create(width, height, channels, 1, 
                                              NEURAX_DATA_FLOAT32, tensor);
    if (error != NEURAX_SUCCESS) {
        printf("Error: Cannot create tensor\n");
        fclose(file);
        return -1;
    }
    
    // Read pixel data
    fseek(file, header.offset, SEEK_SET);
    
    int bytes_per_pixel = channels;
    int row_padding = (4 - (width * bytes_per_pixel) % 4) % 4;
    
    uint8_t* row_buffer = malloc(width * bytes_per_pixel + row_padding);
    if (!row_buffer) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        neurax_tensor_destroy(*tensor);
        return -1;
    }
    
    float* tensor_data = (float*)(*tensor)->data;
    
    for (int y = 0; y < height; y++) {
        if (fread(row_buffer, width * bytes_per_pixel + row_padding, 1, file) != 1) {
            printf("Error: Cannot read pixel data\n");
            free(row_buffer);
            fclose(file);
            neurax_tensor_destroy(*tensor);
            return -1;
        }
        
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                int tensor_idx = (y * width + x) * channels + c;
                int bmp_idx;
                
                if (channels == 4) {
                    // BGRA in BMP → RGBA in tensor
                    if (c == 0) bmp_idx = x * bytes_per_pixel + 2; // R ← R
                    else if (c == 1) bmp_idx = x * bytes_per_pixel + 1; // G ← G  
                    else if (c == 2) bmp_idx = x * bytes_per_pixel + 0; // B ← B
                    else bmp_idx = x * bytes_per_pixel + 3; // A ← A
                } else {
                    // BGR in BMP → RGB in tensor
                    bmp_idx = x * bytes_per_pixel + (channels - 1 - c);
                }
                
                tensor_data[tensor_idx] = row_buffer[bmp_idx] / 255.0f;
            }
        }
    }
    
    free(row_buffer);
    fclose(file);
    return 0;
}

static int save_bmp_image(const char* filename, neurax_tensor_t* tensor) {
    if (!tensor || tensor->data_type != NEURAX_DATA_FLOAT32) {
        return -1;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Error: Cannot create output file %s\n", filename);
        return -1;
    }
    
    int width = tensor->width;
    int height = tensor->height;
    int channels = tensor->channels;
    int bytes_per_pixel = (channels >= 3) ? 3 : 1; // Save as 24-bit RGB (drop alpha)
    int row_padding = (4 - (width * bytes_per_pixel) % 4) % 4;
    int image_size = height * (width * bytes_per_pixel + row_padding);
    
    // Write BMP header
    bmp_header_t header = {
        .type = 0x4D42,
        .size = sizeof(header) + sizeof(bmp_info_header_t) + image_size,
        .reserved1 = 0,
        .reserved2 = 0,
        .offset = sizeof(header) + sizeof(bmp_info_header_t)
    };
    fwrite(&header, sizeof(header), 1, file);
    
    // Write info header
    bmp_info_header_t info = {
        .size = sizeof(bmp_info_header_t),
        .width = width,
        .height = height,
        .planes = 1,
        .bits_per_pixel = bytes_per_pixel * 8,
        .compression = 0,
        .image_size = image_size,
        .x_pixels_per_meter = 2835,
        .y_pixels_per_meter = 2835,
        .colors_used = 0,
        .colors_important = 0
    };
    fwrite(&info, sizeof(info), 1, file);
    
    // Write pixel data
    uint8_t* row_buffer = calloc(width * bytes_per_pixel + row_padding, 1);
    if (!row_buffer) {
        fclose(file);
        return -1;
    }
    
    float* tensor_data = (float*)tensor->data;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Only save RGB channels (0, 1, 2), ignore alpha if present
            for (int c = 0; c < bytes_per_pixel; c++) {
                int tensor_idx = (y * width + x) * channels + c;
                int bmp_idx = x * bytes_per_pixel + (bytes_per_pixel - 1 - c); // RGB to BGR
                
                float value = tensor_data[tensor_idx];
                value = (value < 0.0f) ? 0.0f : (value > 1.0f) ? 1.0f : value;
                row_buffer[bmp_idx] = (uint8_t)(value * 255.0f);
            }
        }
        fwrite(row_buffer, width * bytes_per_pixel + row_padding, 1, file);
    }
    
    free(row_buffer);
    fclose(file);
    return 0;
}

static int create_blur_kernel(neurax_tensor_t** kernel, int kernel_size, float sigma) {
    neurax_error_t error = neurax_tensor_create(kernel_size, kernel_size, 1, 1, 
                                              NEURAX_DATA_FLOAT32, kernel);
    if (error != NEURAX_SUCCESS) {
        return -1;
    }
    
    float* kernel_data = (float*)(*kernel)->data;
    float sum = 0.0f;
    int center = kernel_size / 2;
    
    // Generate Gaussian kernel
    for (int y = 0; y < kernel_size; y++) {
        for (int x = 0; x < kernel_size; x++) {
            int dx = x - center;
            int dy = y - center;
            float distance_sq = dx * dx + dy * dy;
            float value = expf(-distance_sq / (2.0f * sigma * sigma));
            
            kernel_data[y * kernel_size + x] = value;
            sum += value;
        }
    }
    
    // Normalize kernel
    for (int i = 0; i < kernel_size * kernel_size; i++) {
        kernel_data[i] /= sum;
    }
    
    return 0;
}

static int apply_blur_filter(neurax_device_t* device, neurax_tensor_t* input, 
                           neurax_tensor_t* kernel, neurax_tensor_t** output) {
    int kernel_size = kernel->width;
    int padding = kernel_size / 2;
    
    // Output dimensions (same as input with padding)
    int out_width = input->width;
    int out_height = input->height;
    int out_channels = input->channels;
    
    neurax_error_t error = neurax_tensor_create(out_width, out_height, out_channels, 1,
                                              NEURAX_DATA_FLOAT32, output);
    if (error != NEURAX_SUCCESS) {
        return -1;
    }
    
    // Apply blur to each channel separately
    for (int c = 0; c < input->channels; c++) {
        // Create single-channel tensors
        neurax_tensor_t* input_channel = NULL;
        neurax_tensor_t* output_channel = NULL;
        
        error = neurax_tensor_create(input->width, input->height, 1, 1,
                                   NEURAX_DATA_FLOAT32, &input_channel);
        if (error != NEURAX_SUCCESS) {
            neurax_tensor_destroy(*output);
            return -1;
        }
        
        error = neurax_tensor_create(out_width, out_height, 1, 1,
                                   NEURAX_DATA_FLOAT32, &output_channel);
        if (error != NEURAX_SUCCESS) {
            neurax_tensor_destroy(input_channel);
            neurax_tensor_destroy(*output);
            return -1;
        }
        
        // Copy channel data
        float* input_data = (float*)input->data;
        float* channel_data = (float*)input_channel->data;
        
        for (int i = 0; i < input->width * input->height; i++) {
            channel_data[i] = input_data[i * input->channels + c];
        }
        
        // Configure convolution
        neurax_conv_config_t conv_config = {
            .kernel_width = kernel_size,
            .kernel_height = kernel_size,
            .stride_x = 1,
            .stride_y = 1,
            .padding_x = padding,
            .padding_y = padding,
            .input_channels = 1,
            .output_channels = 1,
            .use_bias = false,
            .activation = NEURAX_ACTIVATION_LINEAR
        };
        
        // Apply convolution
        error = neurax_conv2d(device, input_channel, kernel, NULL, &conv_config, output_channel);
        if (error != NEURAX_SUCCESS) {
            neurax_tensor_destroy(input_channel);
            neurax_tensor_destroy(output_channel);
            neurax_tensor_destroy(*output);
            return -1;
        }
        
        // Copy result back to output tensor
        float* output_data = (float*)(*output)->data;
        float* result_data = (float*)output_channel->data;
        
        for (int i = 0; i < out_width * out_height; i++) {
            output_data[i * out_channels + c] = result_data[i];
        }
        
        neurax_tensor_destroy(input_channel);
        neurax_tensor_destroy(output_channel);
    }
    
    return 0;
}

static void create_sample_image(const char* filename) {
    // Create a simple test pattern image
    const int width = 256;
    const int height = 256;
    const int channels = 3;
    
    neurax_tensor_t* tensor = NULL;
    neurax_error_t error = neurax_tensor_create(width, height, channels, 1,
                                              NEURAX_DATA_FLOAT32, &tensor);
    if (error != NEURAX_SUCCESS) {
        printf("Error creating sample image tensor\n");
        return;
    }
    
    float* data = (float*)tensor->data;
    
    // Create a pattern with circles and gradients
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * channels;
            
            // Create circular patterns
            float cx = width / 2.0f;
            float cy = height / 2.0f;
            float dist = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
            
            // Red channel - circular gradient
            data[idx + 0] = 0.5f + 0.5f * sinf(dist * 0.1f);
            
            // Green channel - checkerboard
            data[idx + 1] = ((x / 16) + (y / 16)) % 2 ? 0.8f : 0.2f;
            
            // Blue channel - diagonal gradient
            data[idx + 2] = (float)(x + y) / (width + height);
        }
    }
    
    save_bmp_image(filename, tensor);
    neurax_tensor_destroy(tensor);
}

static void print_usage(const char* program_name) {
    printf("Usage: %s <input.bmp> <output.bmp> [options]\n", program_name);
    printf("Options:\n");
    printf("  --kernel-size <size>  Blur kernel size (3-11, odd numbers, default: 5)\n");
    printf("  --sigma <value>       Gaussian sigma (default: 1.0)\n");
    printf("  --create-sample       Create a sample input image instead of processing\n");
    printf("  --help                Show this help\n");
    printf("\nExamples:\n");
    printf("  %s input.bmp output.bmp\n", program_name);
    printf("  %s input.bmp output.bmp --kernel-size 7 --sigma 1.5\n", program_name);
    printf("  %s sample.bmp output.bmp --create-sample\n", program_name);
}
