
#include "neurax/hal/AcceleratorFactory.hpp"
#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/image/ImageProcessor.hpp"
#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "neurax/core/LayerBuilder.hpp"

#include <cmath>
#include <iostream>
#include <chrono>

using namespace neurax::tensor;
using namespace neurax::hal;
using namespace neurax::image;
using namespace neurax::core;

Tensor create_gaussian_blur_kernel()
{
    size_t input_channels = 4;   // RGBA
    size_t output_channels = 4;  // RGBA
    unsigned long size = 3;
    Tensor kernel({size, size, input_channels, output_channels}, DataType::FLOAT32);
    auto data = kernel.data_ptr<float>();

    float sigma = 1.0f;
    int half = size / 2;

    // 2D Gaussian kernel
    std::vector<float> gaussian(size * size);
    float sum = 0.0f;
    for (int x = -half; x <= half; ++x) {
        for (int y = -half; y <= half; ++y) {
            float value = std::exp(-(x * x + y * y) / (2 * sigma * sigma));
            value /= (2.0f * static_cast<float>(M_PI) * sigma * sigma);
            gaussian[(x + half) * size + (y + half)] = value;
            sum += value;
        }
    }
    for (auto &v : gaussian) v /= sum; // normalizacija na 1

    // Popuni kernel: RGB dobija Gaussian, A dobija identity
    for (int out_ch = 0; out_ch < (int)output_channels; out_ch++) {
        for (int in_ch = 0; in_ch < (int)input_channels; in_ch++) {
            for (int x = 0; x < (int)size; ++x) {
                for (int y = 0; y < (int)size; ++y) {
                    size_t weight_idx =
                        (((x) * size + (y)) * input_channels + in_ch) * output_channels + out_ch;

                    if (out_ch < 3 && in_ch == out_ch) {
                        // Blur samo ako je isti kanal (R->R, G->G, B->B)
                        data[weight_idx] = gaussian[x * size + y];
                    } else if (out_ch == 3 && in_ch == 3) {
                        // A kanal = identity (centar = 1)
                        data[weight_idx] = (x == half && y == half) ? 1.0f : 0.0f;
                    } else {
                        // nema miješanja kanala
                        data[weight_idx] = 0.0f;
                    }
                }
            }
        }
    }

    return kernel;
}


int main()
{
    ImageProcessor processor;
    ConvolutionConfig c(3,1,1,4,4); // 3x3 kernel, stride 1, padding 1, 4 in/out channels
    processor.load_bmp("examples/data/images/Lenna.bmp");
    auto input_tensor = processor.to_tensor();
    auto accelerator = AcceleratorFactory::create(AcceleratorType::SOFTWARE_FALLBACK);
    accelerator->initialize();
    Tensor blur_kernel = create_gaussian_blur_kernel();
    auto start = std::chrono::high_resolution_clock::now();
    auto out_hal = accelerator->convolution(input_tensor,blur_kernel,Tensor::zeros({1,3,3,1}, DataType::FLOAT32),c);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Convolution time with hal layer: " << duration.count() << " ms\n";

    LayerBuilder layerBuilder;
    layerBuilder.conv2d()
                .addWeights(blur_kernel,Tensor::zeros({1,3,3,1}));
    NeuralNetworkBuilder builder;
    builder.addAccelerator(accelerator.get())
        .addLayer(layerBuilder.build());
    INetwork* network = builder.build();
    start = std::chrono::high_resolution_clock::now();
    auto out_core = network->infer(input_tensor);
    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    std::cout << "Convolution time with core layer: " << duration.count() << " ms\n";

    accelerator->cleanup();
    processor.save_bmp(out_hal, "examples/data/images/Lenna_out_hal.bmp");
    processor.save_bmp(out_core, "examples/data/images/Lenna_out_core.bmp");
}