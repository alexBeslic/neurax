#ifndef CONV2D_LAYER_HPP
#define CONV2D_LAYER_HPP

#include "neurax/core/layers/ILayer.hpp"
#include "neurax/hal/IAccelerator.hpp"
#include <vector>

namespace neurax {

namespace hal {
    struct ConvolutionConfig;
}

namespace core {

class Conv2dLayer : public ILayer {
private:
    neurax::hal::IAccelerator* accelerator_;
    neurax::hal::ConvolutionConfig* config_;
    Tensor weights_;
    Tensor bias_;

public:
    Conv2dLayer(int inChannels, int outChannels, int kernelSize, int stride = 1, int padding = 0);
    ~Conv2dLayer();

    // Override virtual functions from ILayer
    void forward(const Tensor& input, Tensor& output) override;
    Tensor forward(const Tensor& input) override;
    void loadWeights(const Tensor& weights, const Tensor& bias) override;
    void addAccelerator(neurax::hal::IAccelerator* accelerator) override;
};

} // namespace core
} // namespace neurax
#endif // CONV2D_LAYER_HPP