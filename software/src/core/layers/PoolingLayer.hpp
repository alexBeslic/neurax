
#ifndef POOLING_LAYER_HPP
#define POOLING_LAYER_HPP

#include "neurax/core/layers/ILayer.hpp"
#include "neurax/hal/IAccelerator.hpp"
#include <vector>

namespace neurax {

namespace core {

class PoolingLayer : public ILayer {
private:
    friend class PoolingBuilder;
    neurax::hal::IAccelerator* accelerator_;
    std::unique_ptr<neurax::hal::PoolingConfig> config_;

    PoolingLayer();

public:
    ~PoolingLayer();

    void setPoolSize(size_t size);
    void setStride(size_t stride);
    void setType(neurax::hal::PoolingType type);
    // Override virtual functions from ILayer
    void forward(const Tensor& input, Tensor& output) override;
    Tensor forward(const Tensor& input) override;
    void addAccelerator(neurax::hal::IAccelerator* accelerator) override;
};

} // namespace core
} // namespace neurax
#endif // POOLING_LAYER_HPP