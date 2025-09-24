
#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/image/ImageProcessor.hpp"
#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "neurax/core/LayerBuilder.hpp"
#include "neurax/core/Conv2dBuilder.hpp"

#include <cmath>
#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>

using namespace neurax::tensor;
using namespace neurax::hal;
using namespace neurax::image;
using namespace neurax::core;


Tensor load_wights(const std::string& filename, const Shape& shape)
{
    size_t num_elements = shape.numel();
    std::vector<float> weights_data(num_elements);

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Ne mogu otvoriti fajl: " + filename);
    }
    file.read(reinterpret_cast<char*>(weights_data.data()), num_elements * sizeof(float));
    if (!file) {
        throw std::runtime_error("Greška pri čitanju fajla: " + filename);
    }
    return Tensor(shape, weights_data.data(), DataType::FLOAT32);
}

int main(int argc, char** argv)
{
    if (argc != 9) {
        std::cerr << "Upotreba: " << argv[0]
                  << " <weight_path> <batch> <H> <W> <in_channels> <out_channels> <kernel_size> <image_path>\n";
        return 1;
    }

    // Parsiranje argumenata
    std::string weight_path = argv[1];
    size_t batch        = std::stoul(argv[2]);
    size_t H            = std::stoul(argv[3]);
    size_t W            = std::stoul(argv[4]);
    size_t in_channels  = std::stoul(argv[5]);
    size_t out_channels = std::stoul(argv[6]);
    size_t kernel_size  = std::stoul(argv[7]);
    std::string image_path = argv[8];


    std::cout << "Pokrećem test sa parametrima:\n"
              << " batch=" << batch
              << " H=" << H
              << " W=" << W
              << " in_channels=" << in_channels
              << " out_channels=" << out_channels
              << " kernel_size=" << kernel_size << "\n";
    ImageProcessor processor;
    processor.load_bmp(image_path);
    auto input_tensor = processor.to_tensor();
    auto kernel = load_wights(weight_path, Shape({kernel_size, kernel_size, in_channels, out_channels}));
    NeuralNetworkBuilder builder;
    auto layer = LayerBuilder::conv2d();
    layer.kernelSize(kernel_size)
         .stride(1)
         .padding(kernel_size/2)
         .inputChanels(in_channels)
         .outputChanels(out_channels)
         .addWeights(kernel,Tensor());

    builder.useAccelerator(AcceleratorType::CPU_OPTIMIZED)
            .addLayer(layer.build());
    auto network = builder.build();

    auto start_core = std::chrono::high_resolution_clock::now();
    auto out_core = network->infer(input_tensor);
    auto end_core = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_core = end_core - start_core;
    std::cout << "Convolution time with core layer: " << duration_core.count() << " ms\n";
    processor.save_bmp(out_core, image_path + ".out.bmp");
}