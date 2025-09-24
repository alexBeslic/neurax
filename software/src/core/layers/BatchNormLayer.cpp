#include "BatchNormLayer.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/hal/AcceleratorFactory.hpp"
#include "neurax/hal/AcceleratorTypes.hpp"
#include <cmath>

namespace neurax {
namespace core {

using namespace neurax::tensor;

BatchNormLayer::BatchNormLayer()
    : accelerator_(nullptr), epsilon_(1e-5f), momentum_(0.1f) {}

BatchNormLayer::~BatchNormLayer() {}

void BatchNormLayer::setEpsilon(float eps) { epsilon_ = eps; }

void BatchNormLayer::setMomentum(float momentum) { momentum_ = momentum; }

void BatchNormLayer::loadWeights(const Tensor& gamma, const Tensor& beta) {
    gamma_ = gamma;
    beta_ = beta;
}

void BatchNormLayer::addAccelerator(neurax::hal::IAccelerator* accelerator) {
    accelerator_ = accelerator;
}

void BatchNormLayer::forward(const Tensor& input, Tensor& output) {
    // Validate gamma/beta presence and shapes first (consistent errors)
    const Shape& s = input.shape();
    if (s.size() == 4) {
        size_t C = s[3];
        if (gamma_.empty() || beta_.empty()) {
            throw std::runtime_error("BatchNorm: gamma/beta not loaded");
        }
        if (gamma_.numel() != C || beta_.numel() != C) {
            throw std::runtime_error("BatchNorm: gamma/beta size mismatch with channels");
        }
    } else if (s.size() == 2) {
        size_t F = s[1];
        if (gamma_.empty() || beta_.empty()) {
            throw std::runtime_error("BatchNorm: gamma/beta not loaded");
        }
        if (gamma_.numel() != F || beta_.numel() != F) {
            throw std::runtime_error("BatchNorm: gamma/beta size mismatch with features");
        }
    } else {
        throw std::runtime_error("BatchNorm: unsupported input rank");
    }

    // Try accelerator first; if it fails or doesn't implement batchnorm, fallback to software accelerator
    if (accelerator_) {
        try {
            output = accelerator_->batchnorm(input, gamma_, beta_, epsilon_, momentum_);
            return;
        } catch (const std::exception&) {
            // fallthrough to software fallback
        }
    }

    // Use AcceleratorFactory to create a software accelerator as guaranteed fallback
    std::unique_ptr<neurax::hal::IAccelerator> sw = neurax::hal::AcceleratorFactory::create(neurax::hal::AcceleratorType::SOFTWARE_FALLBACK);
    if (!sw) {
        throw std::runtime_error("BatchNorm: failed to create software accelerator fallback");
    }
    output = sw->batchnorm(input, gamma_, beta_, epsilon_, momentum_);
}

Tensor BatchNormLayer::forward(const Tensor& input) {
    Tensor out;
    forward(input, out);
    return out;
}

} // namespace core
} // namespace neurax
