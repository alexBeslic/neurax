
/**
 * @file ILayer.hpp
 * @brief Main layers
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

#ifndef NEURAX_CORE_ILAYER_HPP
#define NEURAX_CORE_ILAYER_HPP

#include "neurax/tensor/Tensor.hpp"
#include "neurax/hal/IAccelerator.hpp"

namespace neurax {

namespace core {

using namespace neurax::tensor;

class ILayer
{
public:
    virtual void forward(const Tensor& input, Tensor& output) = 0;
    virtual Tensor forward(const Tensor& input) = 0;
    virtual void addAccelerator(neurax::hal::IAccelerator* accelerator) = 0;
};


} // namespace core
} // namespace neurax

#endif /* NEURAX_CORE_ILAYER_HPP */