#ifndef DENSE_LAYER_HPP
#define DENSE_LAYER_HPP

#include "neurax/core/layers/ILayer.hpp"
#include "neurax/hal/IAccelerator.hpp"
#include <memory>

namespace neurax {

namespace hal {
    struct DenseConfig; // placeholder if needed in future
}

namespace core {

class DenseLayer : public ILayer {
private:
    friend class DenseBuilder;
    neurax::hal::IAccelerator* accelerator_;
    int units_;
    Tensor weights_;
    Tensor bias_;

    DenseLayer();
    DenseLayer(int units);

public:
    ~DenseLayer();

    void setUnits(int units);

    // Override virtual functions from ILayer
    void forward(const Tensor& input, Tensor& output) override;
    Tensor forward(const Tensor& input) override;
    void loadWeights(const Tensor& weights, const Tensor& bias);
    void addAccelerator(neurax::hal::IAccelerator* accelerator) override;
};

} // namespace core
} // namespace neurax

#endif // DENSE_LAYER_HPP
