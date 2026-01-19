#include "neurax/core/BiasAddBuilder.hpp"
#include "layers/BiasAddLayer.hpp"

namespace neurax {
namespace core {

BiasAddBuilder::BiasAddBuilder() {
    layer_ = new BiasAddLayer();
}

BiasAddBuilder& BiasAddBuilder::setBias(const tensor::Tensor& bias) {
    layer_->setBias(bias);
    return *this;
}

ILayer* BiasAddBuilder::build() {
    return layer_;
}

} // namespace core
} // namespace neurax
