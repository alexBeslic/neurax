/**
 * @file Network.hpp
 * @brief Implementation of the Network class
 *
 * @author NEURAX Development Team
 * @date September 2025
 * @version 2.0
 */

 #ifndef NEURAX_CORE_NETWORK_HPP
 #define NEURAX_CORE_NETWORK_HPP

#include "neurax/core/network/INetwork.hpp"
#include <vector>
namespace neurax {

namespace core {

class NeuralNetwork : public INetwork
{
private:
    neurax::hal::IAccelerator* accelerator_;
    std::vector<std::unique_ptr<ILayer>> layers_;
public:
    void addLayer(std::unique_ptr<ILayer> layer) override;
    void removeLayer(size_t index) override;
    std::unique_ptr<ILayer>& getLayer(size_t index) override;
    size_t getLayerCount() const override;
    void addAccelerator(neurax::hal::IAccelerator* accelerator) override { accelerator_ = accelerator; }
    neurax::hal::IAccelerator* getAccelerator() override;
    void infer(const Tensor& input, Tensor& output) override;
    Tensor infer(const Tensor& input) override;
};


} // namespace core
} // namespace neurax

#endif /* NEURAX_CORE_NETWORK_HPP */