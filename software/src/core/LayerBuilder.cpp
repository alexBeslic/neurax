
#include "neurax/core/LayerBuilder.hpp"
#include "neurax/core/Conv2dBuilder.hpp"
#include "neurax/core/PoolingBuilder.hpp"
#include "neurax/core/ActivationBuilder.hpp"

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


} // namespace core
} // namespace neurax