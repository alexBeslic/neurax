#ifndef NEURAX_CORE_FLATTENBUILDER_HPP
#define NEURAX_CORE_FLATTENBUILDER_HPP

#include "layers/ILayer.hpp"

namespace neurax {
namespace core {

class FlattenLayer;

class FlattenBuilder{
    friend class LayerBuilder;
    FlattenLayer* layer_;

    FlattenBuilder();
    FlattenBuilder(const FlattenBuilder&) = delete;
    FlattenBuilder(FlattenBuilder&&) = delete;
    FlattenBuilder& operator=(const FlattenBuilder&) = delete;

public:

    ILayer* build();

};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_FLATTENBUILDER_HPP