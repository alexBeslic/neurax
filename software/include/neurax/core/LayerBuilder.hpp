// NeuralNetworkBuilder
#ifndef NEURAX_CORE_LAYERBUILDER_HPP
#define NEURAX_CORE_LAYERBUILDER_HPP

#include "layers/ILayer.hpp"
#include "Conv2dBuilder.hpp"
#include "PoolingBuilder.hpp"

namespace neurax {
namespace core {

class Conv2dBuilder;
class PoolingBuilder;

class LayerBuilder {

    LayerBuilder() = delete;
public:

    static Conv2dBuilder conv2d();

    static PoolingBuilder pool();
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_LAYERBUILDER_HPP