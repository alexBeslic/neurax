
#include "neurax/core/LayerBuilder.hpp"
#include "neurax/core/Conv2dBuilder.hpp"
#include "neurax/core/PoolingBuilder.hpp"
#include "neurax/core/ActivationBuilder.hpp"
#include "neurax/core/DenseBuilder.hpp"
#include "neurax/core/FlattenBuilder.hpp"

namespace neurax {
namespace core {

Conv2dBuilder LayerBuilder::conv2d(){
    return Conv2dBuilder();
}

PoolingBuilder LayerBuilder::pool(){
    return PoolingBuilder();
}

ActivationBuilder LayerBuilder::activation(){
    return ActivationBuilder();
}

DenseBuilder LayerBuilder::dense(){
    return DenseBuilder();
}

FlattenBuilder LayerBuilder::flatten(){
    return FlattenBuilder();
}

BatchNormBuilder LayerBuilder::batchnorm(){
    return BatchNormBuilder();
}


} // namespace core
} // namespace neurax