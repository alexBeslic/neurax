#include <cmath>
#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>

#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "neurax/core/LayerBuilder.hpp"

using namespace neurax::tensor;
using namespace neurax::core;

// ---------------- Pomoćna funkcija ----------------
std::vector<float> loadBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Ne mogu otvoriti fajl: " + filename);
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize % sizeof(float) != 0) {
        throw std::runtime_error("Fajl " + filename + " nije ispravan float binarni niz.");
    }

    size_t numElements = fileSize / sizeof(float);
    std::vector<float> data(numElements);

    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    return data;
}

// ---------------- MAIN ----------------
int main(int argc, char* argv[]) {
    if (argc != 7) {
        std::cerr << "Upotreba: " << argv[0]
                  << " <batch> <H> <W> <in_channels> <pool_size> <stride>\n";
        return 1;
    }

    // Parsiranje argumenata
    size_t batch        = std::stoul(argv[1]);
    size_t H            = std::stoul(argv[2]);
    size_t W            = std::stoul(argv[3]);
    size_t in_channels  = std::stoul(argv[4]);
    size_t pool_size    = std::stoul(argv[5]);
    size_t stride       = std::stoul(argv[6]);

    std::cout << "Pokrećem test sa parametrima:\n"
              << " batch=" << batch
              << " H=" << H
              << " W=" << W
              << " in_channels=" << in_channels
              << " pool_size=" << pool_size
              << " stride=" << stride << "\n";

    // --- Učitaj podatke ---
    std::vector<float> input   = loadBinaryFile("input.bin");
    std::vector<float> output_ref = loadBinaryFile("output.bin");

    // Kreiranje Tensor objekata (NHWC + HWIO)
    Tensor input_tensor = Tensor::from_blob(
        input.data(),
        Shape({batch, H, W, in_channels}),
        DataType::FLOAT32
    );

    // --- Gradnja mreže ---
    PoolingBuilder poolbuilder = LayerBuilder::pool();
    poolbuilder.poolSize(pool_size)
                .stride(stride)
                .type(neurax::hal::PoolingType::MAX);
    NeuralNetworkBuilder builder;
    builder.addLayer(poolbuilder.build());
    builder.useAccelerator(neurax::hal::AcceleratorType::CPU_OPTIMIZED);
    std::unique_ptr<INetwork> network = builder.build();

    // --- Izvrši inference ---
    Tensor out_core = network->infer(input_tensor);

    // --- Uporedi rezultate ---
    std::vector<float> output(out_core.data_ptr<float>(),
                              out_core.data_ptr<float>() + out_core.numel());

    if (output.size() != output_ref.size()) {
        std::cout << "❌ Izlaz ima " << output.size()
                  << " elemenata, a referenca ima " << output_ref.size() << "!\n";
        std::cerr << "❌ Dimenzije izlaza se ne poklapaju sa referencom!\n";
        return 1;
    }

    bool ok = true;
    for (size_t i = 0; i < output.size(); i++) {
        if (std::fabs(output[i] - output_ref[i]) > 1e-5) {
            std::cerr << "❌ Razlika na indeksu " << i
                      << ": " << output[i] << " vs " << output_ref[i] << "\n";
            ok = false;
        }
    }

    if (ok) {
        std::cout << "✅ Izlaz se poklapa sa referentnim rezultatima.\n";
    }

    return ok ? 0 : 1;
}
