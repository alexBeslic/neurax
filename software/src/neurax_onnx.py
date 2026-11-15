#!/usr/bin/env python3
"""Run ONNX models with supported Conv, ReLU, and MaxPool nodes on Neurax."""

import argparse
import ctypes
from pathlib import Path
from collections import Counter

import numpy as np
import onnx
from onnx.reference import ReferenceEvaluator
from onnx.reference.op_run import OpRun


ACTIVE_FPGA = None


class NeuraxFPGA:
    def __init__(self, library_path):
        self.library = ctypes.CDLL(str(library_path))
        self.library.neurax_fpga_open.restype = ctypes.c_void_p
        self.library.neurax_fpga_close.argtypes = [ctypes.c_void_p]
        self.library.neurax_fpga_last_error.restype = ctypes.c_char_p
        self.library.neurax_fpga_relu.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_float),
        ]
        self.library.neurax_fpga_conv.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_float),
        ] + [ctypes.c_uint32] * 13
        self.library.neurax_fpga_maxpool.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_float),
        ] + [ctypes.c_uint32] * 12
        self.handle = self.library.neurax_fpga_open()
        if not self.handle:
            raise RuntimeError(self._error())

    def _error(self):
        message = self.library.neurax_fpga_last_error()
        return message.decode("utf-8", errors="replace") if message else "unknown FPGA error"

    @staticmethod
    def _float_buffer(array):
        contiguous = np.ascontiguousarray(array, dtype=np.float32)
        return contiguous, contiguous.ctypes.data_as(ctypes.POINTER(ctypes.c_float))

    def relu(self, tensor):
        tensor = np.ascontiguousarray(tensor, dtype=np.float32)
        result = np.empty_like(tensor)
        status = self.library.neurax_fpga_relu(
            self.handle,
            tensor.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
            tensor.size,
            result.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
        )
        self._check(status)
        return result

    def conv(self, x, weights, bias, strides, pads):
        x, x_ptr = self._float_buffer(x)
        weights, weights_ptr = self._float_buffer(weights)
        if bias is None:
            bias_ptr = ctypes.POINTER(ctypes.c_float)()
        else:
            bias, bias_ptr = self._float_buffer(bias)
        n, channels, height, width = x.shape
        out_channels, _, kernel_h, kernel_w = weights.shape
        out_h = (height + pads[0] + pads[2] - kernel_h) // strides[0] + 1
        out_w = (width + pads[1] + pads[3] - kernel_w) // strides[1] + 1
        result = np.empty((n, out_channels, out_h, out_w), dtype=np.float32)
        status = self.library.neurax_fpga_conv(
            self.handle, x_ptr, weights_ptr, bias_ptr,
            result.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
            n, channels, height, width, out_channels, kernel_h, kernel_w,
            strides[0], strides[1], pads[0], pads[1], pads[2], pads[3],
        )
        self._check(status)
        return result

    def maxpool(self, x, kernel, strides, pads):
        x, x_ptr = self._float_buffer(x)
        n, channels, height, width = x.shape
        out_h = (height + pads[0] + pads[2] - kernel[0]) // strides[0] + 1
        out_w = (width + pads[1] + pads[3] - kernel[1]) // strides[1] + 1
        result = np.empty((n, channels, out_h, out_w), dtype=np.float32)
        status = self.library.neurax_fpga_maxpool(
            self.handle, x_ptr,
            result.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
            n, channels, height, width, kernel[0], kernel[1],
            strides[0], strides[1], pads[0], pads[1], pads[2], pads[3],
        )
        self._check(status)
        return result

    def _check(self, status):
        if status != 0:
            message = self._error()
            if status == -2:
                raise UnsupportedFPGAOperation(message)
            raise RuntimeError(message)

    def close(self):
        if self.handle:
            self.library.neurax_fpga_close(self.handle)
            self.handle = None


class UnsupportedFPGAOperation(RuntimeError):
    pass


def _spatial_pads(input_shape, kernel, strides, dilations, auto_pad, pads):
    n, channels, height, width = input_shape
    del n, channels
    effective_h = dilations[0] * (kernel[0] - 1) + 1
    effective_w = dilations[1] * (kernel[1] - 1) + 1
    if auto_pad == "VALID":
        return (0, 0, 0, 0)
    if auto_pad in ("SAME_UPPER", "SAME_LOWER"):
        out_h = (height + strides[0] - 1) // strides[0]
        out_w = (width + strides[1] - 1) // strides[1]
        total_h = max((out_h - 1) * strides[0] + effective_h - height, 0)
        total_w = max((out_w - 1) * strides[1] + effective_w - width, 0)
        if auto_pad == "SAME_LOWER":
            top, left = (total_h + 1) // 2, (total_w + 1) // 2
        else:
            top, left = total_h // 2, total_w // 2
        return (top, left, total_h - top, total_w - left)
    return tuple(int(value) for value in (pads or (0, 0, 0, 0)))


class Conv(OpRun):
    op_domain = ""

    def _run(self, x, W, B=None, auto_pad="NOTSET", dilations=None, group=1,
             kernel_shape=None, pads=None, strides=None):
        if ACTIVE_FPGA is None:
            raise RuntimeError("Neurax FPGA backend is not initialized")
        x = np.asarray(x, dtype=np.float32)
        weights = np.asarray(W, dtype=np.float32)
        strides = tuple(strides or (1, 1))
        dilations = tuple(dilations or (1, 1))
        auto_pad = auto_pad.decode() if isinstance(auto_pad, bytes) else auto_pad
        if group != 1 or dilations != (1, 1):
            raise UnsupportedFPGAOperation("FPGA Conv supports group=1 and dilation=1 only")
        if x.ndim != 4 or weights.ndim != 4:
            raise UnsupportedFPGAOperation("FPGA Conv expects NCHW input and OIHW weights")
        if weights.shape[1] != x.shape[1]:
            raise ValueError("Conv input channels do not match the weight tensor")
        if kernel_shape and tuple(kernel_shape) != tuple(weights.shape[2:]):
            raise ValueError("Conv kernel_shape does not match the weight tensor")
        spatial_pads = _spatial_pads(
            x.shape, weights.shape[2:], strides, dilations, auto_pad, pads
        )
        result = ACTIVE_FPGA.conv(x, weights, B, strides, spatial_pads)
        print(f"[FPGA Conv] {tuple(x.shape)} x {tuple(weights.shape)} -> {result.shape}")
        return (result,)


class Relu(OpRun):
    op_domain = ""

    def _run(self, X):
        if ACTIVE_FPGA is None:
            raise RuntimeError("Neurax FPGA backend is not initialized")
        result = ACTIVE_FPGA.relu(np.asarray(X, dtype=np.float32))
        print(f"[FPGA ReLU] {tuple(np.shape(X))}")
        return (result,)


class MaxPool(OpRun):
    op_domain = ""

    def _run(self, X, auto_pad="NOTSET", ceil_mode=0, dilations=None,
             kernel_shape=None, pads=None, storage_order=0, strides=None):
        if ACTIVE_FPGA is None:
            raise RuntimeError("Neurax FPGA backend is not initialized")
        if ceil_mode or storage_order:
            raise UnsupportedFPGAOperation("FPGA MaxPool requires ceil_mode=0 and storage_order=0")
        dilations = tuple(dilations or (1, 1))
        if dilations != (1, 1):
            raise UnsupportedFPGAOperation("FPGA MaxPool supports dilation=1 only")
        kernel = tuple(kernel_shape or ())
        if len(kernel) != 2:
            raise ValueError("MaxPool requires a two-dimensional kernel_shape")
        strides = tuple(strides or kernel)
        x = np.asarray(X, dtype=np.float32)
        auto_pad = auto_pad.decode() if isinstance(auto_pad, bytes) else auto_pad
        spatial_pads = _spatial_pads(
            x.shape, kernel, strides, dilations, auto_pad, pads
        )
        result = ACTIVE_FPGA.maxpool(x, kernel, strides, spatial_pads)
        print(f"[FPGA MaxPool] {tuple(x.shape)} -> {result.shape}")
        return (result,)


def main():
    parser = argparse.ArgumentParser(
        description="Evaluate an ONNX model, dispatching supported layers to Neurax FPGA."
    )
    parser.add_argument("model", type=Path, help="ONNX model path")
    parser.add_argument("--input", type=Path, required=True,
                        help="NumPy .npy input tensor in ONNX input layout")
    parser.add_argument("--output", type=Path, default=Path("neurax_output.npy"),
                        help="Path for the first ONNX output tensor")
    parser.add_argument("--library", type=Path,
                        default=Path(__file__).with_name("libneurax_fpga.so"),
                        help="Path to the compiled Neurax UIO shared library")
    args = parser.parse_args()

    for path in (args.model, args.input, args.library):
        if not path.is_file():
            raise FileNotFoundError(f"File not found: {path}")

    model = onnx.load(str(args.model))
    onnx.checker.check_model(model)
    graph_inputs = [
        value for value in model.graph.input
        if value.name not in {initializer.name for initializer in model.graph.initializer}
    ]
    if len(graph_inputs) != 1:
        raise ValueError("This runner expects exactly one non-initializer ONNX input")
    tensor = np.load(args.input, allow_pickle=False)
    if not np.issubdtype(tensor.dtype, np.floating):
        tensor = tensor.astype(np.float32)
    tensor = np.ascontiguousarray(tensor, dtype=np.float32)

    fpga = NeuraxFPGA(args.library)
    global ACTIVE_FPGA
    ACTIVE_FPGA = fpga
    try:
        operators = Counter(node.op_type for node in model.graph.node)
        accelerated_ops = {"Conv", "Relu", "MaxPool"}
        print(f"Model: {args.model}")
        print(f"ONNX nodes routed to FPGA handlers: {sum(operators[name] for name in accelerated_ops)}")
        print(f"Other ONNX nodes evaluated on CPU: {sum(operators.values()) - sum(operators[name] for name in accelerated_ops)}")
        evaluator = ReferenceEvaluator(
            model,
            new_ops=[Conv, Relu, MaxPool],
        )
        outputs = evaluator.run(None, {graph_inputs[0].name: tensor})
        if not outputs:
            raise RuntimeError("ONNX model returned no outputs")
        np.save(args.output, outputs[0], allow_pickle=False)
        print(f"Saved first model output to {args.output}")
    finally:
        fpga.close()
        ACTIVE_FPGA = None


if __name__ == "__main__":
    main()
