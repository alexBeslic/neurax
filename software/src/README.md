# ONNX to Neurax FPGA runner

`neurax_onnx.py` loads and checks an ONNX model, then evaluates its graph with
ONNX `ReferenceEvaluator`. Conv, ReLU, and MaxPool nodes call the Neurax
accelerator through the existing UIO/mSGDMA interface; other ONNX operators
run in the evaluator on the CPU. Input and output tensors are NumPy `.npy`
files in the ONNX tensor layout (Conv/MaxPool use NCHW).

## Build the ARM shared library

Build from the repository root with the ARM toolchain used by
`software/driver/uio`:

```sh
make -C software/src
```

Copy `libneurax_fpga.so` and `neurax_onnx.py` to the target. Install Python 3
packages `numpy` and `onnx`, ensure the `uio` and `neurax_uio` drivers are
loaded, and run as a user with access to `/dev/mem` and `/dev/uio*`:

```sh
python3 software/src/neurax_onnx.py model.onnx --input input.npy
```

Use `--library` to select another shared-library path and `--output` to set the
output `.npy` filename. Each FPGA tensor element is truncated to signed Q24.8;
the model is therefore quantized to 8 fractional bits at every dispatched
layer.

## Current hardware limits

The deployed RTL uses a fixed 100x100 input address stride and RAM regions of
10,000 input words, 3,000 weight words, 16 bias words, and 9,984 output words.
Accordingly, the initial backend accepts only single-batch, single-input-
channel Conv and single-channel MaxPool tensors that fit those regions. Conv
also requires square kernels (at most 5x5), equal strides (at most 4), and
dilation 1; MaxPool requires square windows (at most 8x8), equal strides, and
dilation 1. ReLU tensors are split into 4096-element accelerator operations.
Unsupported FPGA shapes and hardware errors are reported rather than silently
being simulated on the CPU.

ONNX `SAME_*` and explicit spatial padding are placed in the input RAM image;
Conv/MaxPool results are cropped to the ONNX output dimensions. Quantization
and these RTL memory limits should be considered when choosing a model.
