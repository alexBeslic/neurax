/**
 * @file ImageProcessor.cpp
 * @brief Implementation of ImageProcessor class
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 1.0
 */

#include "neurax/image/ImageProcessor.hpp"
#include "neurax/tensor/DataType.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace neurax {
namespace image {

using neurax::tensor::Tensor;
using neurax::tensor::Shape;
using neurax::tensor::DataType;

ImageProcessor::ImageProcessor() : width_(0), height_(0), channels_(3) {}

bool ImageProcessor::load_bmp(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    // Read BMP header
    file.read(reinterpret_cast<char*>(&bmp_header_), sizeof(bmp_header_));
    if (bmp_header_.file_type != 0x4D42) {
        std::cerr << "Error: Invalid BMP file format" << std::endl;
        return false;
    }
    
    // Read BMP info header
    file.read(reinterpret_cast<char*>(&bmp_info_header_), sizeof(bmp_info_header_));
    
    // Basic validation
    if (bmp_info_header_.bit_count != 24 && bmp_info_header_.bit_count != 32) {
        std::cerr << "Error: Only 24-bit and 32-bit BMP files are supported" << std::endl;
        return false;
    }
    
    width_ = static_cast<uint32_t>(bmp_info_header_.width);
    height_ = static_cast<uint32_t>(std::abs(bmp_info_header_.height));
    channels_ = bmp_info_header_.bit_count / 8;
    
    // Move to pixel data
    file.seekg(bmp_header_.offset_data, std::ios::beg);
    
    // Calculate row padding (BMP rows are padded to 4-byte boundary)
    uint32_t bytes_per_row = width_ * channels_;
    uint32_t padding = (4 - (bytes_per_row % 4)) % 4;
    uint32_t padded_row_size = bytes_per_row + padding;
    
    // Read pixel data
    image_data_.resize(height_ * bytes_per_row);
    std::vector<uint8_t> row_buffer(padded_row_size);
    
    for (uint32_t y = 0; y < height_; ++y) {
        file.read(reinterpret_cast<char*>(row_buffer.data()), padded_row_size);
        std::memcpy(&image_data_[y * bytes_per_row], row_buffer.data(), bytes_per_row);
    }
    
    file.close();
    
    std::cout << "✅ Loaded BMP image: " << width_ << "x" << height_ 
              << "x" << channels_ << " (" << image_data_.size() << " bytes)\n";
    return true;
}

Tensor ImageProcessor::to_tensor() const {
    if (image_data_.empty()) {
        throw std::runtime_error("No image data loaded");
    }
    
    // Convert BGRA/BGR to RGBA and normalize to [0,1]
    Shape shape({1, static_cast<size_t>(height_), static_cast<size_t>(width_), 4});  // NHWC format with alpha
    Tensor tensor = Tensor::zeros(shape, DataType::FLOAT32);
    auto* tensor_data = tensor.data_ptr<float>();
    
    for (uint32_t y = 0; y < height_; ++y) {
        for (uint32_t x = 0; x < width_; ++x) {
            uint32_t img_idx = (y * width_ + x) * channels_;
            uint32_t tensor_idx = (y * width_ + x) * 4;
            
            // BMP stores as BGR(A), convert to RGBA
            tensor_data[tensor_idx + 0] = static_cast<float>(image_data_[img_idx + 2]) / 255.0f; // R
            tensor_data[tensor_idx + 1] = static_cast<float>(image_data_[img_idx + 1]) / 255.0f; // G
            tensor_data[tensor_idx + 2] = static_cast<float>(image_data_[img_idx + 0]) / 255.0f; // B
            
            // Alpha channel - use from source if available, otherwise set to 1.0 (opaque)
            if (channels_ >= 4) {
                tensor_data[tensor_idx + 3] = static_cast<float>(image_data_[img_idx + 3]) / 255.0f; // A
            } else {
                tensor_data[tensor_idx + 3] = 1.0f; // Opaque
            }
        }
    }
    
    return tensor;
}

bool ImageProcessor::save_bmp(const Tensor& tensor, const std::string& filename) {
    const auto& shape = tensor.shape();
    if (shape.size() != 4 || shape[0] != 1 || shape[3] != 4) {
        std::cerr << "Error: Invalid tensor format for image saving (expected RGBA)" << std::endl;
        return false;
    }
    
    uint32_t tensor_height = static_cast<uint32_t>(shape[1]);
    uint32_t tensor_width = static_cast<uint32_t>(shape[2]);
    
    // Create new headers for 32-bit BGRA output BMP
    BMPHeader out_header;
    BMPInfoHeader out_info_header;
    
    out_header.file_type = 0x4D42;
    out_header.reserved1 = 0;
    out_header.reserved2 = 0;
    out_header.offset_data = sizeof(BMPHeader) + sizeof(BMPInfoHeader);
    
    out_info_header.size = sizeof(BMPInfoHeader);
    out_info_header.width = static_cast<int32_t>(tensor_width);
    out_info_header.height = static_cast<int32_t>(tensor_height);
    out_info_header.planes = 1;
    out_info_header.bit_count = 32;  // 32-bit with alpha
    out_info_header.compression = 0;
    out_info_header.size_image = tensor_width * tensor_height * 4;
    out_info_header.x_pixels_per_meter = 3780;
    out_info_header.y_pixels_per_meter = 3780;
    out_info_header.colors_used = 0;
    out_info_header.colors_important = 0;
    
    out_header.file_size = out_header.offset_data + out_info_header.size_image;
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create output file " << filename << std::endl;
        return false;
    }
    
    // Write headers
    file.write(reinterpret_cast<const char*>(&out_header), sizeof(out_header));
    file.write(reinterpret_cast<const char*>(&out_info_header), sizeof(out_info_header));
    
    // Convert tensor back to BGRA format and write
    const float* tensor_data = tensor.data_ptr<float>();
    std::vector<uint8_t> image_buffer(tensor_width * tensor_height * 4);
    
    for (uint32_t y = 0; y < tensor_height; ++y) {
        for (uint32_t x = 0; x < tensor_width; ++x) {
            uint32_t tensor_idx = (y * tensor_width + x) * 4;
            uint32_t buffer_idx = (y * tensor_width + x) * 4;
            
            // Convert RGBA back to BGRA and denormalize
            float r = tensor_data[tensor_idx + 0];
            float g = tensor_data[tensor_idx + 1];
            float b = tensor_data[tensor_idx + 2];
            float a = tensor_data[tensor_idx + 3];
            
            // Clamp values to [0,1] and convert to [0,255]
            image_buffer[buffer_idx + 0] = static_cast<uint8_t>(std::round(std::min(1.0f, std::max(0.0f, b)) * 255.0f)); // B
            image_buffer[buffer_idx + 1] = static_cast<uint8_t>(std::round(std::min(1.0f, std::max(0.0f, g)) * 255.0f)); // G
            image_buffer[buffer_idx + 2] = static_cast<uint8_t>(std::round(std::min(1.0f, std::max(0.0f, r)) * 255.0f)); // R
            image_buffer[buffer_idx + 3] = static_cast<uint8_t>(std::round(std::min(1.0f, std::max(0.0f, a)) * 255.0f)); // A
        }
    }
    
    file.write(reinterpret_cast<const char*>(image_buffer.data()), image_buffer.size());
    file.close();
    std::cout << "✅ Saved processed image: " << filename << std::endl;
    return true;
}

void ImageProcessor::clear() {
    image_data_.clear();
    width_ = 0;
    height_ = 0;
    channels_ = 3;
}

} // namespace image
} // namespace neurax
