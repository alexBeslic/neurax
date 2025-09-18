/**
 * @file ImageProcessor.hpp
 * @brief Image processing utilities for NEURAX Neural Network Accelerator
 *
 * This module provides image loading, processing, and conversion utilities
 * for integration with NEURAX tensor operations.
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#ifndef NEURAX_IMAGE_PROCESSOR_HPP
#define NEURAX_IMAGE_PROCESSOR_HPP

#include "../tensor/Tensor.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace neurax {
namespace image {

// BMP file header structures
#pragma pack(push, 1)
struct BMPHeader {
    uint16_t file_type{0x4D42};          // File type always BM which is 0x4D42
    uint32_t file_size{0};               // Size of the file (in bytes)
    uint16_t reserved1{0};               // Reserved, always 0
    uint16_t reserved2{0};               // Reserved, always 0
    uint32_t offset_data{0};             // Start position of pixel data (bytes from the beginning of the file)
};

struct BMPInfoHeader {
    uint32_t size{0};                    // Size of this header (in bytes)
    int32_t width{0};                    // width of bitmap in pixels
    int32_t height{0};                   // height of bitmap in pixels
    uint16_t planes{1};                  // No. of planes for the target device, this is always 1
    uint16_t bit_count{0};               // No. of bits per pixel
    uint32_t compression{0};             // 0 or 3 - uncompressed. THIS PROGRAM CONSIDERS ONLY UNCOMPRESSED BMP images
    uint32_t size_image{0};              // 0 - for uncompressed images
    int32_t x_pixels_per_meter{0};
    int32_t y_pixels_per_meter{0};
    uint32_t colors_used{0};             // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
    uint32_t colors_important{0};        // No. of colors used for displaying the bitmap. If 0 all colors are required
};
#pragma pack(pop)

/**
 * @brief Image processing class for loading, converting, and saving images
 *
 * The ImageProcessor class provides functionality for:
 * - Loading BMP images (24-bit and 32-bit)
 * - Converting images to NEURAX tensor format (NHWC with normalization)
 * - Saving tensors back to BMP format
 * - Handling BGRA ↔ RGBA conversions
 *
 * Example usage:
 * @code
 * neurax::image::ImageProcessor processor;
 * processor.load_bmp("input.bmp");
 * auto tensor = processor.to_tensor();
 * // ... process tensor ...
 * processor.save_bmp(result_tensor, "output.bmp");
 * @endcode
 */
class ImageProcessor {
private:
    BMPHeader bmp_header_;
    BMPInfoHeader bmp_info_header_;
    std::vector<uint8_t> image_data_;
    uint32_t width_;
    uint32_t height_;
    uint32_t channels_;

public:
    /**
     * @brief Default constructor
     */
    ImageProcessor();

    /**
     * @brief Load BMP image from file
     * @param filename Path to BMP file
     * @return True if successful, false otherwise
     *
     * Supports 24-bit BGR and 32-bit BGRA BMP files.
     * Automatically handles row padding as per BMP specification.
     */
    bool load_bmp(const std::string& filename);

    /**
     * @brief Convert loaded image to NEURAX tensor format
     * @return 4D tensor in NHWC format [1, height, width, 4]
     * @throws std::runtime_error if no image data is loaded
     *
     * Converts BGR(A) to RGBA format and normalizes pixel values to [0,1].
     * Alpha channel is set to 1.0 for 24-bit images (opaque).
     */
    neurax::tensor::Tensor to_tensor() const;

    /**
     * @brief Save tensor as BMP image
     * @param tensor 4D tensor in NHWC format [1, height, width, 4]
     * @param filename Output BMP file path
     * @return True if successful, false otherwise
     *
     * Converts RGBA tensor back to BGRA format and saves as 32-bit BMP.
     * Denormalizes values from [0,1] to [0,255] and clamps to valid range.
     */
    bool save_bmp(const neurax::tensor::Tensor& tensor, const std::string& filename);

    /**
     * @brief Get image width in pixels
     * @return Image width
     */
    uint32_t width() const { return width_; }

    /**
     * @brief Get image height in pixels
     * @return Image height
     */
    uint32_t height() const { return height_; }

    /**
     * @brief Get number of channels (3 for BGR, 4 for BGRA)
     * @return Number of channels
     */
    uint32_t channels() const { return channels_; }

    /**
     * @brief Check if image data is loaded
     * @return True if image is loaded, false otherwise
     */
    bool is_loaded() const { return !image_data_.empty(); }

    /**
     * @brief Get raw image data
     * @return Reference to raw pixel data vector
     */
    const std::vector<uint8_t>& get_image_data() const { return image_data_; }

    /**
     * @brief Clear loaded image data
     */
    void clear();
};

} // namespace image
} // namespace neurax

#endif /* NEURAX_IMAGE_PROCESSOR_HPP */
