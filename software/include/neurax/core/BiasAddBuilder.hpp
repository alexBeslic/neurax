#ifndef NEURAX_CORE_BIASADDBUILDER_HPP
#define NEURAX_CORE_BIASADDBUILDER_HPP

#include "layers/ILayer.hpp"
#include "neurax/tensor/Tensor.hpp"

namespace neurax {
namespace core {

class BiasAddLayer;

class BiasAddBuilder {
    friend class LayerBuilder;
    BiasAddLayer* layer_;

    BiasAddBuilder();
    BiasAddBuilder(const BiasAddBuilder&) = delete;
    BiasAddBuilder(BiasAddBuilder&&) = delete;
    BiasAddBuilder& operator=(const BiasAddBuilder&) = delete;

public:
    ILayer* build();

    BiasAddBuilder& setBias(const tensor::Tensor& bias);
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_BIASADDBUILDER_HPP
