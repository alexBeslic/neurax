#ifndef NEURAX_CORE_POOLINGBUILDER_HPP
#define NEURAX_CORE_POOLINGBUILDER_HPP

#include "layers/ILayer.hpp"

namespace neurax {
namespace core {

class PoolingLayer;

class PoolingBuilder {
    friend class LayerBuilder;
    PoolingLayer* layer_;

    PoolingBuilder();
    PoolingBuilder(const PoolingBuilder&) = delete;
    PoolingBuilder(PoolingBuilder&&) = delete;
    PoolingBuilder& operator=(const PoolingBuilder&) = delete;

public:

    ILayer* build();

    PoolingBuilder& poolSize(size_t size);
    PoolingBuilder& stride(size_t stride);
    PoolingBuilder& type(neurax::hal::PoolingType type);

};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_CONV2DBUILDER_HPP