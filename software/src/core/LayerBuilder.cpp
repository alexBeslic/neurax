
#include "neurax/core/LayerBuilder.hpp"
#include "neurax/core/Conv2dBuilder.hpp"
#include "neurax/core/PoolingBuilder.hpp"

namespace neurax {
namespace core {

Conv2dBuilder LayerBuilder::conv2d(){
    return Conv2dBuilder();
}

PoolingBuilder LayerBuilder::pool(){
    return PoolingBuilder();
}


} // namespace core
} // namespace neurax