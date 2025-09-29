#ifndef NEURAX_CORE_ACTIVATIONBUILDER_HPP
#define NEURAX_CORE_ACTIVATIONBUILDER_HPP

#include "layers/ILayer.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"

namespace neurax {
namespace core {

class ActivationLayer;

class ActivationBuilder {
    friend class LayerBuilder;
    ActivationLayer* layer_;

    ActivationBuilder();
    ActivationBuilder(const ActivationBuilder&) = delete;
    ActivationBuilder(ActivationBuilder&&) = delete;
    ActivationBuilder& operator=(const ActivationBuilder&) = delete;

public:
    ILayer* build();

    ActivationBuilder& type(neurax::hal::ActivationType type);
};

} // namespace core
} // namespace neurax

#endif // NEURAX_CORE_ACTIVATIONBUILDER_HPP
