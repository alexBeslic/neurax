#ifndef NEURAX_CORE_BATCHNORMBUILDER_HPP
#define NEURAX_CORE_BATCHNORMBUILDER_HPP

#include "layers/ILayer.hpp"

namespace neurax {
namespace core {

class BatchNormLayer;

class BatchNormBuilder {
    friend class LayerBuilder;
    BatchNormLayer* layer_;

    BatchNormBuilder();
    BatchNormBuilder(const BatchNormBuilder&) = delete;
    BatchNormBuilder(BatchNormBuilder&&) = delete;
    BatchNormBuilder& operator=(const BatchNormBuilder&) = delete;

public:
    ILayer* build();

    BatchNormBuilder& epsilon(float eps);
    BatchNormBuilder& momentum(float momentum);
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_BATCHNORMBUILDER_HPP
