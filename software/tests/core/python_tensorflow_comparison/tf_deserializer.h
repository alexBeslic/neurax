
// tf_deserializer.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <optional>

#include "3pp/cnpy/cnpy.h"
#include <3pp/json/include/nlohmann/json.hpp>

#include "neurax/tensor/Tensor.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/DataType.hpp"

using json = nlohmann::json;
using namespace neurax::tensor; // da bude konzistentno s tvojim kodom

struct LayerDesc {
    std::string name;
    std::string type;
    std::optional<std::string> weights_name;
    std::optional<std::string> bias_name;
    std::optional<std::string> activation;
    std::vector<int> strides;
    std::vector<int> ksize;
    std::optional<std::string> padding;
    std::vector<int> dilation;           // novi
    std::optional<int> units;            // za Dense
    std::optional<std::string> pool_type; // "max" / "avg"
};


class TFDeserializer {
public:
    // Konstruktor učita manifest json i npz fajl
    TFDeserializer(const std::string& manifest_json_path, const std::string& npz_path)
    {
        // učitaj npz
        npz_data_ = cnpy::npz_load(npz_path);
        // parse manifest
        std::ifstream f(manifest_json_path);
        if (!f.is_open()) {
            throw std::runtime_error("Failed to open manifest JSON: " + manifest_json_path);
        }
        json j;
        f >> j;
        parse_manifest(j);
    }

    // Vrati listu slojeva (u redosledu iz manifesta)
    const std::vector<LayerDesc>& layers() const { return layers_; }

    // Provjera da li npz sadrži tensor s tim imenom
    bool hasNPZ(const std::string& name) const {
        return npz_data_.find(name) != npz_data_.end();
    }

    // Vrati pointer na float podate iz npz i oblik (ne kopira)
    // lifetime pointera: validan dok je TFDeserializer objekt živ
    std::pair<std::vector<size_t>, const float*> getNPZArrayView(const std::string& name) const {
        auto it = npz_data_.find(name);
        if (it == npz_data_.end()) {
            throw std::runtime_error("NPZ array not found: " + name);
        }
        const cnpy::NpyArray& arr = it->second;
        if (arr.word_size != sizeof(float)) {
            throw std::runtime_error("Only float32 arrays supported in NPZ (arr: " + name + ")");
        }
        std::vector<size_t> shape;
        for (auto d : arr.shape) shape.push_back(static_cast<size_t>(d));
        const float* ptr = arr.data<float>();
        return {shape, ptr};
    }

    // Istu kao gore, ali vraća i numel
    std::tuple<std::vector<size_t>, const float*, size_t> getNPZArrayViewWithNumel(const std::string& name) const {
        auto [shape, ptr] = getNPZArrayView(name);
        size_t n = 1;
        for (auto d : shape) n *= d;
        return {shape, ptr, n};
    }

    // Kopiraj sadržaj iz NPZ u postojeći neurax::tensor::Tensor (pretpostavlja float32)
    // Ovo će raditi memcpy u dest tensor; provjerava dimenzije.
    void fillTensorFromNPZ(Tensor& dest, const std::string& npz_name) const {
        auto it = npz_data_.find(npz_name);
        if (it == npz_data_.end()) {
            throw std::runtime_error("NPZ array not found: " + npz_name);
        }
        const cnpy::NpyArray& arr = it->second;
        if (arr.word_size != sizeof(float)) {
            throw std::runtime_error("Only float32 arrays supported in NPZ");
        }

        // shape iz arr
        std::vector<size_t> arr_shape;
        for (auto d : arr.shape) arr_shape.push_back(static_cast<size_t>(d));

        // tvoj Shape API:
        const auto& dest_dims = dest.shape().dims(); // pretpostavka: Shape::dims() -> vector<size_t>
        if (dest_dims.size() != arr_shape.size()) {
            throw std::runtime_error("Dest tensor rank mismatch for " + npz_name);
        }
        for (size_t i = 0; i < arr_shape.size(); ++i) {
            if (dest_dims[i] != arr_shape[i]) {
                throw std::runtime_error("Dest tensor shape mismatch at dim " + std::to_string(i) +
                                         " for " + npz_name + ": dest=" + std::to_string(dest_dims[i]) +
                                         " arr=" + std::to_string(arr_shape[i]));
            }
        }

        size_t n = 1;
        for (auto d : arr_shape) n *= d;
        std::memcpy(dest.data_ptr<float>(), arr.data<float>(), n * sizeof(float));
    }

private:
    // parse manifest JSON into internal LayerDesc-safely (null-tolerant)
    void parse_manifest(const json& j) {
        if (!j.is_array()) {
            throw std::runtime_error("Manifest JSON must be an array of layer entries");
        }
        for (const auto& entry : j) {
            LayerDesc ld;
            // name
            if (entry.contains("name") && entry["name"].is_string())
                ld.name = entry["name"].get<std::string>();
            else
                ld.name = "";

            // type
            if (entry.contains("type") && entry["type"].is_string())
                ld.type = entry["type"].get<std::string>();
            else
                ld.type = "";

            // weights
            if (entry.contains("weights") && entry["weights"].is_string())
                ld.weights_name = entry["weights"].get<std::string>();
            else
                ld.weights_name = std::nullopt;

            // bias
            if (entry.contains("bias") && entry["bias"].is_string())
                ld.bias_name = entry["bias"].get<std::string>();
            else
                ld.bias_name = std::nullopt;

            // activation
            if (entry.contains("activation") && entry["activation"].is_string())
                ld.activation = entry["activation"].get<std::string>();
            else
                ld.activation = std::nullopt;

            // ksize (optional array of ints)
            ld.ksize.clear();
            if (entry.contains("params") && entry["params"].contains("kernel_size") && entry["params"]["kernel_size"].is_array()) {
                for (const auto& v : entry["params"]["kernel_size"]) {
                    if (v.is_number_integer()) ld.ksize.push_back(v.get<int>());
                }
            }

            // pool_size (optional array of ints) - novi
            if (entry.contains("params") && entry["params"].contains("pool_size") && entry["params"]["pool_size"].is_array()) {
                for (const auto& v : entry["params"]["pool_size"]) {
                    if (v.is_number_integer()) ld.ksize.push_back(v.get<int>());
                }
            }
            // strides (optional array of ints)
            ld.strides.clear();
            if (entry.contains("params") && entry["params"].contains("strides") && entry["params"]["strides"].is_array()) {
                for (const auto& v : entry["params"]["strides"]) {
                    if (v.is_number_integer()) ld.strides.push_back(v.get<int>());
                }
            }

            // padding (optional string)
            if(entry.contains("params") && entry["params"].contains("padding") && entry["params"]["padding"].is_string())
                ld.padding = entry["params"]["padding"].get<std::string>();
            else
                ld.padding = std::nullopt;

            // dilation_rate
            ld.dilation.clear();
            if(entry.contains("params") && entry["params"].contains("dilation_rate") && entry["params"]["dilation_rate"].is_array()){
                for (const auto& v : entry["params"]["dilation_rate"]) {
                    if (v.is_number_integer()) ld.dilation.push_back(v.get<int>());
                }
            }

            if(entry.contains("params") && entry["params"].contains("units") && entry["params"]["units"].is_number_integer()){
                ld.units = entry["params"]["units"].get<int>();
            } else {
                ld.units = std::nullopt;
            }

            // pool_type
            if (entry.contains("pool_type") && entry["pool_type"].is_string()) {
                ld.pool_type = entry["pool_type"].get<std::string>();
            }


            layers_.push_back(std::move(ld));
        }
    }

private:
    // cnpy::npz_t is std::map<std::string, NpyArray>
    std::map<std::string, cnpy::NpyArray> npz_data_;
    std::vector<LayerDesc> layers_;
};
