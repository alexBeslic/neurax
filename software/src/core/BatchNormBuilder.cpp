#include "neurax/core/BatchNormBuilder.hpp"
#include "layers/BatchNormLayer.hpp"

namespace neurax {
namespace core {

BatchNormBuilder::BatchNormBuilder() {
    layer_ = new BatchNormLayer();
}

ILayer* BatchNormBuilder::build() {
    return layer_;
}

BatchNormBuilder& BatchNormBuilder::epsilon(float eps) {
    layer_->setEpsilon(eps);
    return *this;
}

BatchNormBuilder& BatchNormBuilder::momentum(float momentum) {
    layer_->setMomentum(momentum);
    return *this;
}

} // namespace core
} // namespace neurax
