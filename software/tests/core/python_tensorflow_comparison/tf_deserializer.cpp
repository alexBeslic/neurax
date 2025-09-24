#include "tf_deserializer.h"
#include <iostream>
#include "neurax/core/NeuralNetworkBuilder.hpp"
#include "neurax/core/LayerBuilder.hpp"

#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

using namespace neurax::core;
using namespace neurax::tensor;

inline int compute_same_padding(int input_size, int kernel_size, int stride) {
    // Izračunaj izlaznu dimenziju
    int out_size = static_cast<int>(std::ceil(static_cast<float>(input_size) / stride));

    // Ukupan padding koji treba dodati
    int total_pad = std::max((out_size - 1) * stride + kernel_size - input_size, 0);

    // Simetrični padding → isti sa obje strane
    return total_pad / 2;
}

/**
 * @brief Create a NeuralNetwork from TensorFlow manifest and NPZ weights
 * @param manifestFile Path to the JSON manifest file
 * @param npzFile Path to the NPZ file containing weights
 * @param input_size Input height/width (assuming square input for simplicity)
 * @return std::unique_ptr<INetwork> The constructed neural network
 */
std::unique_ptr<INetwork> buildNetworkFromTF(const std::string &manifestFile, const std::string &npzFile, int input_size) {
    TFDeserializer d(manifestFile, npzFile);
    NeuralNetworkBuilder builder;
    for (const auto &L : d.layers()) {
        if (L.type == "Conv2D") {
            auto layer = LayerBuilder::conv2d();
                             layer.stride(L.strides[0]) // pretpostavka: 2D, isti stride u H i W
                             .kernelSize(L.ksize[0]); // pretpostavka: 2D, isti ksize u H i W
            if(L.padding =="same")
                layer.padding(compute_same_padding(input_size, L.ksize[0], L.strides[0])); // pretpostavka: ulazna visina 8
            else if (L.padding == "valid")
                layer.padding(0);
            if (L.weights_name) {
                auto [shape, ptr, numel] = d.getNPZArrayViewWithNumel(*L.weights_name);
                Tensor weights = Tensor(Shape(shape), DataType::FLOAT32);
                std::memcpy(weights.data_ptr<float>(), ptr, numel * sizeof(float));
                layer.inputChanels(shape[2]); // shape: [kh, kw, in, out]
                layer.outputChanels(shape[3]);
                if (L.bias_name == std::nullopt) {
                    layer.addWeights(weights, Tensor()); // bez biasa
                } else {

                    auto [shape, ptr, numel] = d.getNPZArrayViewWithNumel(*L.bias_name);
                    Tensor bias = Tensor(Shape(shape), DataType::FLOAT32);
                    std::memcpy(weights.data_ptr<float>(), ptr, numel * sizeof(float));
                    layer.addWeights(weights, bias );
                }
            }
            builder.addLayer(layer.build());
            if (L.activation) {
                auto layerActivation = LayerBuilder::activation();
                if (L.activation == "relu")
                    layerActivation.type(neurax::hal::ActivationType::RELU);
                else if (L.activation == "sigmoid")
                    layerActivation.type(neurax::hal::ActivationType::SIGMOID);
                else if (L.activation == "tanh")
                    layerActivation.type(neurax::hal::ActivationType::TANH);
                builder.addLayer(layerActivation.build());
            }
        } else if (L.type == "Dense") {
            auto layer = LayerBuilder::dense();
            if (L.weights_name) {
                auto [shape, ptr, numel] = d.getNPZArrayViewWithNumel(*L.weights_name);
                Tensor weights = Tensor(Shape(shape), DataType::FLOAT32);
                std::memcpy(weights.data_ptr<float>(), ptr, numel * sizeof(float));
                if (L.bias_name == std::nullopt) {
                    layer.addWeights(weights, Tensor()); // bez biasa
                } else {
                    auto [shape, ptr, numel] = d.getNPZArrayViewWithNumel(*L.bias_name);
                    Tensor bias = Tensor(Shape(shape), DataType::FLOAT32);
                    std::memcpy(weights.data_ptr<float>(), ptr, numel * sizeof(float));
                    layer.addWeights(weights, bias );
                }
            }
            builder.addLayer(layer.build());
            if (L.activation) {
                auto layerActivation = LayerBuilder::activation();
                if (L.activation == "relu")
                    layerActivation.type(neurax::hal::ActivationType::RELU);
                else if (L.activation == "sigmoid")
                    layerActivation.type(neurax::hal::ActivationType::SIGMOID);
                else if (L.activation == "tanh")
                    layerActivation.type(neurax::hal::ActivationType::TANH);
                else if (L.activation == "softmax")
                    layerActivation.type(neurax::hal::ActivationType::SOFTMAX);
                builder.addLayer(layerActivation.build());
            }
        } else if (L.type == "MaxPooling2D" || L.type == "AvgPooling2D") {
            auto layer = LayerBuilder::pool();
                             layer.poolSize(L.ksize[0]) // pretpostavka: 2D, isti ksize u H i W
                             .stride(L.strides[0]); // pretpostavka: 2D, isti stride u H i W
            // if(L.padding =="SAME")
            //     layer.padding(L.ksize[0]/2);
            // else if (L.padding == "VALID")
            //     layer.padding(0);
            layer.type(L.type == "MaxPooling2D" ? neurax::hal::PoolingType::MAX : neurax::hal::PoolingType::AVERAGE);
            builder.addLayer(layer.build());
        } else if (L.type == "activation") {
            auto layer = LayerBuilder::activation();
            if (L.activation == "relu")
                layer.type(neurax::hal::ActivationType::RELU);
            else if (L.activation == "sigmoid")
                layer.type(neurax::hal::ActivationType::SIGMOID);
            else if (L.activation == "tanh")
                layer.type(neurax::hal::ActivationType::TANH);
            builder.addLayer(layer.build());
        }else if( L.type == "Flatten") {
            auto layer = LayerBuilder::flatten();
            builder.addLayer(layer.build());
        } else if (L.type == "BatchNormalization") {
            auto layer = LayerBuilder::batchnorm();
            layer.epsilon(1e-5f).momentum(0.9f); // default vrijednosti
            builder.addLayer(layer.build());
        }
        else {
            std::cerr << "Unknown layer type: " << L.type << "\n";
        }
    }
    builder.useAccelerator(neurax::hal::AcceleratorType::CPU_OPTIMIZED);
    return builder.build();
}

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

int main() {
    try{
        size_t batch = 1, H = 28, W = 28, in_channels = 1;
        auto network = buildNetworkFromTF("manifest.json", "mnist_cnn_weights.npz", H);
        std::cout << "Network built successfully with " << network->getLayerCount() << " layers.\n";

        // --- Učitaj podatke ---
        std::vector<float> input   = loadBinaryFile("input.bin");
        std::vector<float> output_ref = loadBinaryFile("output_ref.bin");

         // Kreiranje Tensor objekata (NHWC + HWIO)
        Tensor input_tensor = Tensor::from_blob(
            input.data(),
            Shape({batch, H, W, in_channels}),
            DataType::FLOAT32
        );
        // measure time
        auto start = std::chrono::high_resolution_clock::now();
        Tensor out_tensor = network->infer(input_tensor);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        std::cout << "Inference time: " << duration.count() << " ms\n";
            // Provjera rezultata
            // --- Uporedi rezultate ---
        std::vector<float> output(out_tensor.data_ptr<float>(),
                                out_tensor.data_ptr<float>() + out_tensor.numel());

        if (output.size() != output_ref.size()) {
            std::cerr << "❌ Dimenzije izlaza se ne poklapaju sa referencom!\n";
            return 1;
        }

        std::cout<< "Max at index " << "[" << std::max_element(output.begin(), output.end()) - output.begin() << "]=" << *std::max_element(output.begin(), output.end()) << "\n";
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
        for(int i = 0; i< output.size(); i++)
            std::cout << output[i] << " ";
        std::cout << "\n";
        for(int i = 0; i< output_ref.size(); i++)
            std::cout << output_ref[i] << " ";
        std::cout << "\n";
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
