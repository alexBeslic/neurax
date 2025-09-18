

#include "neurax/core/LayerBuilder.hpp"
#include "layers/Conv2dLayer.hpp"

namespace neurax {
namespace core {


LayerBuilder& LayerBuilder::conv2d(){
    layer_ = new Conv2dLayer(4,4,3,1,1);
    return *this;
}
LayerBuilder& LayerBuilder::addWeights(const Tensor& weights, const Tensor& bias) {
    layer_->loadWeights(weights, bias);
    return *this;
}

} // namespace core
} // namespace neurax