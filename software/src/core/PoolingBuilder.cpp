

#include "neurax/core/PoolingBuilder.hpp"
#include "layers/PoolingLayer.hpp"

namespace neurax {
namespace core {

PoolingBuilder::PoolingBuilder() {
    layer_ = new PoolingLayer();
}

PoolingBuilder& PoolingBuilder::poolSize(size_t size) {
    layer_->setPoolSize(size);
    return *this;
}
PoolingBuilder& PoolingBuilder::stride(size_t stride) {
    layer_->setStride(stride);
    return *this;
}
PoolingBuilder& PoolingBuilder::type(neurax::hal::PoolingType type) {
    layer_->setType(type);
    return *this;
}

ILayer* PoolingBuilder::build() {
    return layer_;
}

} // namespace core
} // namespace neurax