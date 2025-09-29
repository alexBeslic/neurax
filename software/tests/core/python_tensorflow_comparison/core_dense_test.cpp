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
    if (argc != 4) {
        std::cerr << "Upotreba: " << argv[0]
                  << " <batch> <in_features> <out_features>\n";
        return 1;
    }

    size_t batch = std::stoul(argv[1]);
    size_t in_features = std::stoul(argv[2]);
    size_t out_features = std::stoul(argv[3]);

    std::cout << "Pokrećem dense test: batch=" << batch
              << " in_features=" << in_features
              << " out_features=" << out_features << "\n";

    std::vector<float> input = loadBinaryFile("input.bin");
    std::vector<float> weights = loadBinaryFile("weights.bin");
    std::vector<float> bias = loadBinaryFile("bias.bin");
    std::vector<float> output_ref = loadBinaryFile("output_ref.bin");

    Tensor input_tensor = Tensor::from_blob(
        input.data(),
        Shape({batch, in_features}),
        DataType::FLOAT32
    );

    Tensor weights_tensor = Tensor::from_blob(
        weights.data(),
        Shape({in_features, out_features}),
        DataType::FLOAT32
    );

    Tensor bias_tensor = Tensor::from_blob(
        bias.data(),
        Shape({out_features}),
        DataType::FLOAT32
    );

    // --- Gradnja mreže ---
    DenseBuilder dense = LayerBuilder::dense();
    dense.units(out_features).addWeights(weights_tensor, bias_tensor);

    NeuralNetworkBuilder builder;
    builder.addLayer(dense.build());
    builder.useAccelerator(neurax::hal::AcceleratorType::CPU_OPTIMIZED);
    std::unique_ptr<INetwork> network = builder.build();

    // --- Izvrši inference ---
    Tensor out_core = network->infer(input_tensor);

    std::vector<float> output(out_core.data_ptr<float>(),
                              out_core.data_ptr<float>() + out_core.numel());

    if (output.size() != output_ref.size()) {
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
