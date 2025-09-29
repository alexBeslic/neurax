// NeuralNetworkBuilder
#ifndef NEURAX_CORE_LAYERBUILDER_HPP
#define NEURAX_CORE_LAYERBUILDER_HPP

#include "layers/ILayer.hpp"
#include "Conv2dBuilder.hpp"
#include "PoolingBuilder.hpp"
#include "ActivationBuilder.hpp"
#include "DenseBuilder.hpp"
#include "FlattenBuilder.hpp"
#include "BatchNormBuilder.hpp"

namespace neurax {
namespace core {


class LayerBuilder {

    LayerBuilder() = delete;
public:

    static Conv2dBuilder conv2d();

    static PoolingBuilder pool();
    static ActivationBuilder activation();
    static DenseBuilder dense();
    static FlattenBuilder flatten();
    static BatchNormBuilder batchnorm();
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_LAYERBUILDER_HPP