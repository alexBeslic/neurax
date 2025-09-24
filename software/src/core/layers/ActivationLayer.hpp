#ifndef ACTIVATION_LAYER_HPP
#define ACTIVATION_LAYER_HPP

#include "neurax/core/layers/ILayer.hpp"
#include "neurax/hal/IAccelerator.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

namespace neurax {
namespace core {

class ActivationLayer : public ILayer {
private:
    friend class ActivationBuilder;
    neurax::hal::IAccelerator* accelerator_;
    neurax::hal::ActivationType type_;

    ActivationLayer();
    ActivationLayer(neurax::hal::ActivationType type);
    ~ActivationLayer();
public:

    // Override virtual functions from ILayer
    void forward(const Tensor& input, Tensor& output) override;
    Tensor forward(const Tensor& input) override;
    void addAccelerator(neurax::hal::IAccelerator* accelerator) override;

    void setType(neurax::hal::ActivationType type);
};

} // namespace core
} // namespace neurax

#endif // ACTIVATION_LAYER_HPP
