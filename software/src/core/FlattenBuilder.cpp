

#include "neurax/core/FlattenBuilder.hpp"
#include "layers/FlattenLayer.hpp"

namespace neurax {
namespace core {

FlattenBuilder::FlattenBuilder() {
    layer_ = new FlattenLayer();
}

ILayer* FlattenBuilder::build() {
    return layer_;
}

} // namespace core
} // namespace neurax