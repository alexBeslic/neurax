#ifndef NEURAX_CORE_DENSEBUILDER_HPP
#define NEURAX_CORE_DENSEBUILDER_HPP

#include "layers/ILayer.hpp"

namespace neurax {
namespace core {

class DenseLayer;

class DenseBuilder {
    friend class LayerBuilder;
    DenseLayer* layer_;

    DenseBuilder();
    DenseBuilder(const DenseBuilder&) = delete;
    DenseBuilder(DenseBuilder&&) = delete;
    DenseBuilder& operator=(const DenseBuilder&) = delete;

public:
    ILayer* build();

    DenseBuilder& units(size_t units);
    DenseBuilder& addWeights(const Tensor& weights, const Tensor& bias);
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_DENSEBUILDER_HPP
