#ifndef FLATTEN_LAYER_CPP
#define FLATTEN_LAYER_CPP

#include "FlattenLayer.hpp"
#include "neurax/tensor/Shape.hpp"

namespace neurax {
namespace core {

using namespace neurax::tensor;

FlattenLayer::FlattenLayer() : accelerator_(nullptr) {}

FlattenLayer::~FlattenLayer() {}

void FlattenLayer::forward(const Tensor& input, Tensor& output) {
    // compute flattened shape: keep batch dim if present (dim count >=2)
    const Shape& in_shape = input.shape();

    Shape out_shape;

    if (in_shape.size() >= 2) {
        // keep first dim as batch, flatten rest
        size_t batch = in_shape[0];
        size_t prod = 1;
        for (size_t i = 1; i < in_shape.size(); ++i) {
            prod *= in_shape[i];
        }
        output = input.reshape(Shape({batch, prod}));
    } else if (in_shape.size() == 1) {
        // already 1D - keep as-is
        output = input; // no reshape needed
    }
}

Tensor FlattenLayer::forward(const Tensor& input) {
    Tensor out;
    forward(input, out);
    return out;
}

void FlattenLayer::addAccelerator(neurax::hal::IAccelerator* accelerator) {
    this->accelerator_ = accelerator; // not used for flatten but keep for interface
}

} // namespace core
} // namespace neurax

#endif // FLATTEN_LAYER_CPP
