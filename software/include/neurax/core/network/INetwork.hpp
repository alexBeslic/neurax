/**
 * @file ILayer.hpp
 * @brief Main layers
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_CORE_INETWORK_HPP
#define NEURAX_CORE_INETWORK_HPP

#include <vector>
#include <memory>
#include "neurax/core/layers/ILayer.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/hal/IAccelerator.hpp"

namespace neurax {

namespace core {

using namespace neurax::tensor;

class INetwork
{
public:
    virtual void addLayer(std::unique_ptr<ILayer> layer) = 0;
    virtual void removeLayer(size_t index) = 0;
    virtual std::unique_ptr<ILayer>& getLayer(size_t index) = 0;
    virtual size_t getLayerCount() const = 0;
    virtual neurax::hal::IAccelerator* getAccelerator() = 0;
    virtual void addAccelerator(std::unique_ptr<neurax::hal::IAccelerator> accelerator) = 0;

    virtual void infer(const Tensor& input, Tensor& output) = 0;
    virtual Tensor infer(const Tensor& input) = 0;

};


} // namespace core
} // namespace neurax

#endif /* NEURAX_CORE_INETWORK_HPP */