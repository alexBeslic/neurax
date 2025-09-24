#ifndef FLATTEN_LAYER_HPP
#define FLATTEN_LAYER_HPP

#include "neurax/core/layers/ILayer.hpp"
#include "neurax/hal/IAccelerator.hpp"

namespace neurax {
namespace core {

class FlattenLayer : public ILayer {
private:
    friend class FlattenBuilder;
    neurax::hal::IAccelerator* accelerator_;

    FlattenLayer();

public:
    ~FlattenLayer();

    // Override virtual functions from ILayer
    void forward(const Tensor& input, Tensor& output) override;
    Tensor forward(const Tensor& input) override;
    void addAccelerator(neurax::hal::IAccelerator* accelerator) override;
};

} // namespace core
} // namespace neurax

#endif // FLATTEN_LAYER_HPP
