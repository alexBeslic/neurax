#include "neurax/core/DenseBuilder.hpp"
#include "layers/DenseLayer.hpp"

namespace neurax {
namespace core {

DenseBuilder::DenseBuilder() {
    layer_ = new DenseLayer();
}

ILayer* DenseBuilder::build() {
    return layer_;
}

DenseBuilder& DenseBuilder::units(size_t units) {
    layer_->setUnits(static_cast<int>(units));
    return *this;
}

DenseBuilder& DenseBuilder::addWeights(const Tensor& weights, const Tensor& bias) {
    layer_->loadWeights(weights, bias);
    return *this;
}

} // namespace core
} // namespace neurax
