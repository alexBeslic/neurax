// NeuralNetworkBuilder
#ifndef NEURAX_CORE_LAYERBUILDER_HPP
#define NEURAX_CORE_LAYERBUILDER_HPP

#include "layers/ILayer.hpp"
#include "Conv2dBuilder.hpp"
#include "PoolingBuilder.hpp"
#include "ActivationBuilder.hpp"

namespace neurax {
namespace core {


class LayerBuilder {

    LayerBuilder() = delete;
public:

    static Conv2dBuilder conv2d();

    static PoolingBuilder pool();
    static ActivationBuilder activation();
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_LAYERBUILDER_HPP