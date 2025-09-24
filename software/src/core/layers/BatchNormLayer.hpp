#ifndef BATCHNORM_LAYER_HPP
#define BATCHNORM_LAYER_HPP

#include "neurax/core/layers/ILayer.hpp"
#include "neurax/hal/IAccelerator.hpp"
#include "neurax/tensor/Tensor.hpp"
#include <memory>

namespace neurax {
namespace core {

class BatchNormLayer : public ILayer {
private:
    friend class BatchNormBuilder;
    neurax::hal::IAccelerator* accelerator_;

    // Parameters: scale (gamma) and shift (beta)
    neurax::tensor::Tensor gamma_;
    neurax::tensor::Tensor beta_;

    // Running statistics (optional)
    neurax::tensor::Tensor running_mean_;
    neurax::tensor::Tensor running_var_;

    float epsilon_;
    float momentum_; // for running stats update

    BatchNormLayer();

public:
    ~BatchNormLayer();

    void setEpsilon(float eps);
    void setMomentum(float momentum);

    // Override virtual functions from ILayer
    void forward(const Tensor& input, Tensor& output) override;
    Tensor forward(const Tensor& input) override;
    void loadWeights(const Tensor& gamma, const Tensor& beta);
    void addAccelerator(neurax::hal::IAccelerator* accelerator) override;
};

} // namespace core
} // namespace neurax

#endif // BATCHNORM_LAYER_HPP
