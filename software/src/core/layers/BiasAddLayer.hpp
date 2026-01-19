#ifndef BIAS_ADD_LAYER_HPP
#define BIAS_ADD_LAYER_HPP

#include "neurax/core/layers/ILayer.hpp"
#include "neurax/hal/IAccelerator.hpp"
#include "neurax/tensor/Tensor.hpp"

namespace neurax {
namespace core {

class BiasAddLayer : public ILayer {
private:
    friend class BiasAddBuilder;
    neurax::hal::IAccelerator* accelerator_;
    tensor::Tensor bias_;

    BiasAddLayer();
    ~BiasAddLayer();

public:
    // Override virtual functions from ILayer
    void forward(const Tensor& input, Tensor& output) override;
    Tensor forward(const Tensor& input) override;
    void addAccelerator(neurax::hal::IAccelerator* accelerator) override;

    void setBias(const tensor::Tensor& bias);
    const tensor::Tensor& getBias() const { return bias_; }
};

} // namespace core
} // namespace neurax

#endif // BIAS_ADD_LAYER_HPP
