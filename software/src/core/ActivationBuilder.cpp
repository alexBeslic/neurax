#include "neurax/core/ActivationBuilder.hpp"
#include "layers/ActivationLayer.hpp"

namespace neurax {
namespace core {

ActivationBuilder::ActivationBuilder() {
    layer_ = new ActivationLayer();
}

ActivationBuilder& ActivationBuilder::type(neurax::hal::ActivationType type) {
    layer_->setType(type);
    return *this;
}

ILayer* ActivationBuilder::build() {
    return layer_;
}

} // namespace core
} // namespace neurax
