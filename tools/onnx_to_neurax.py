#!/usr/bin/env python3
"""Convert a supported static-shape ONNX inference graph to an NXB1 bundle."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

try:
    import numpy as np
    import onnx
    from onnx import numpy_helper
except ImportError as exc:  # pragma: no cover - exercised on host setup
    raise SystemExit("Install exporter dependency with: python3 -m pip install onnx numpy") from exc

OPCODES = {
    "LRN": 0,
    "Gemm": 1,
    "Softmax": 2,
    "Dropout": 3,
    "Reshape": 4,
    "Flatten": 4,
    "Conv": 5,
    "Relu": 6,
    "MaxPool": 7,
    "Identity": 3,
}
U32_NONE = 0xFFFFFFFF
MAX_RANK = 8


def attrs_for(node):
    return {attr.name: onnx.helper.get_attribute_value(attr) for attr in node.attribute}


def static_dims(value_info):
    tensor_type = value_info.type.tensor_type
    if not tensor_type.HasField("shape"):
        raise ValueError(f"missing inferred shape for value {value_info.name!r}")
    if len(tensor_type.shape.dim) == 0:
        return [1]
    dims = []
    for dim in tensor_type.shape.dim:
        if not dim.HasField("dim_value") or dim.dim_value <= 0:
            raise ValueError(f"dynamic/empty shape is unsupported for {value_info.name!r}")
        dims.append(int(dim.dim_value))
    if not dims or len(dims) > MAX_RANK:
        raise ValueError(f"rank must be 1..{MAX_RANK} for {value_info.name!r}")
    return dims


def conv_pads(auto_pad, pads, input_dims, output_dims, kernel, dilation, stride):
    if auto_pad in (b"NOTSET", b"VALID", "NOTSET", "VALID"):
        if auto_pad in (b"VALID", "VALID"):
            return [0, 0, 0, 0]
        if len(pads) != 4:
            raise ValueError("Conv pads must contain [top,left,bottom,right]")
        return [int(v) for v in pads]
    if auto_pad not in (b"SAME_UPPER", b"SAME_LOWER", "SAME_UPPER", "SAME_LOWER"):
        raise ValueError(f"unsupported Conv auto_pad={auto_pad!r}")

    totals = []
    for axis in range(2):
        effective = dilation[axis] * (kernel[axis] - 1) + 1
        needed = max((output_dims[axis + 2] - 1) * stride[axis] + effective
                     - input_dims[axis + 2], 0)
        totals.append(needed)
    lower = auto_pad in (b"SAME_LOWER", "SAME_LOWER")
    top = (totals[0] + 1) // 2 if lower else totals[0] // 2
    left = (totals[1] + 1) // 2 if lower else totals[1] // 2
    return [top, left, totals[0] - top, totals[1] - left]


def serialize_bundle(model, output_path):
    graph = model.graph
    try:
        model = onnx.shape_inference.infer_shapes(model)
    except Exception as exc:
        raise ValueError(f"ONNX shape inference failed: {exc}") from exc
    graph = model.graph

    info = {}
    for value in list(graph.input) + list(graph.value_info) + list(graph.output):
        if value.type.HasField("tensor_type") and value.type.tensor_type.elem_type:
            info[value.name] = value
    initializers = {tensor.name: tensor for tensor in graph.initializer}
    initializer_arrays = {name: numpy_helper.to_array(tensor)
                          for name, tensor in initializers.items()}

    def dims_of(name):
        if name in initializer_arrays:
            dims = list(initializer_arrays[name].shape)
            if dims:
                return [int(dim) for dim in dims]
        if name in info:
            return static_dims(info[name])
        raise ValueError(f"missing shape information for {name!r}")

    reshape_shape_names = set()
    dropout_optional_names = set()
    runtime_input_names = set()
    for node in graph.node:
        if node.op_type == "Reshape":
            runtime_input_names.update(name for name in node.input[:1] if name)
        elif node.op_type == "Dropout":
            runtime_input_names.update(name for name in node.input[:1] if name)
            dropout_optional_names.update(name for name in node.input[1:] if name)
        else:
            runtime_input_names.update(name for name in node.input if name)
        if node.op_type == "Reshape" and len(node.input) > 1:
            shape_name = node.input[1]
            if shape_name not in initializers:
                raise ValueError(f"Reshape {node.name!r} shape must be a constant initializer")
            shape_array = initializer_arrays[shape_name]
            if shape_array.dtype.kind not in "iu":
                raise ValueError(f"Reshape shape {shape_name!r} must be integer")
            reshape_shape_names.add(shape_name)

    # Shape tensors are consumed as metadata rather than runtime float tensors.
    excluded_values = reshape_shape_names | {
        name for name in dropout_optional_names
        if name not in runtime_input_names
    }
    excluded_initializers = excluded_values & set(initializers)
    value_names = []
    for value in graph.input:
        if (value.name not in initializers and value.name not in excluded_values
                and value.name not in value_names):
            value_names.append(value.name)
    for name in initializers:
        if name not in excluded_initializers and name not in value_names:
            value_names.append(name)
    for node in graph.node:
        for name in node.input:
            if name and name not in excluded_values and name not in value_names:
                value_names.append(name)
        if node.output and node.output[0] and node.output[0] not in value_names:
            value_names.append(node.output[0])
    for value in graph.output:
        if value.name not in value_names:
            value_names.append(value.name)
    ids = {name: index for index, name in enumerate(value_names)}
    consumed_or_exported = {
        name for node in graph.node for name in node.input if name
    } | {value.name for value in graph.output}
    for node in graph.node:
        for omitted_output in node.output[1:]:
            if omitted_output and omitted_output in consumed_or_exported:
                raise ValueError(
                    f"operator {node.name!r} has a used secondary output, which is unsupported"
                )

    tensor_records = []
    for name in value_names:
        if name in initializers:
            array = initializer_arrays[name]
            if array.dtype.kind != "f":
                raise ValueError(f"initializer {name!r} must be floating point")
            array = np.asarray(array, dtype="<f4", order="C")
            dims = list(array.shape)
            if not dims:
                dims = [1]
            if len(dims) > MAX_RANK or any(int(dim) <= 0 for dim in dims):
                raise ValueError(f"invalid initializer shape for {name!r}: {dims}")
            payload = array.tobytes(order="C")
            count = int(array.size)
            initialized = True
        else:
            if name not in info:
                raise ValueError(f"missing shape/type information for {name!r}")
            value_info = info[name]
            tensor_type = value_info.type.tensor_type
            if tensor_type.elem_type != onnx.TensorProto.FLOAT:
                raise ValueError(f"runtime tensor {name!r} must be float32")
            dims = static_dims(value_info)
            count = int(np.prod(dims, dtype=object))
            payload = b""
            initialized = False
        tensor_records.append((dims, count, initialized, payload))

    graph_inputs = [ids[value.name] for value in graph.input
                    if value.name not in initializers and value.name not in excluded_values]
    graph_outputs = [ids[value.name] for value in graph.output]
    if not graph_outputs:
        raise ValueError("model must declare at least one graph output")

    opset = next((entry.version for entry in model.opset_import
                  if entry.domain in ("", "ai.onnx")), 1)
    node_records = []
    for node in graph.node:
        if node.op_type not in OPCODES:
            raise ValueError(f"unsupported ONNX operator {node.op_type!r} at {node.name!r}")
        op = node.op_type
        attrs = attrs_for(node)
        output_name = node.output[0] if node.output else ""
        if not output_name or output_name not in ids:
            raise ValueError(f"operator {node.name!r} has no supported output")
        inputs = [name for name in node.input if name]
        if op in ("Identity", "Dropout"):
            inputs = inputs[:1]
        elif op == "Reshape":
            inputs = inputs[:1]
        elif op in ("Conv", "Gemm"):
            inputs = inputs[:3]
        else:
            inputs = inputs[:1]
        if any(name not in ids for name in inputs):
            raise ValueError(f"operator {node.name!r} references an omitted input")
        input_ids = [ids[name] for name in inputs]
        output_id = ids[output_name]
        encoded_attr = b""

        if op == "LRN":
            encoded_attr = struct.pack("<Ifff", int(attrs.get("size", 1)),
                                       float(attrs.get("alpha", 1e-4)),
                                       float(attrs.get("beta", 0.75)),
                                       float(attrs.get("bias", 1.0)))
        elif op == "Gemm":
            encoded_attr = struct.pack("<IIff", int(attrs.get("transA", 0)),
                                       int(attrs.get("transB", 0)),
                                       float(attrs.get("alpha", 1.0)),
                                       float(attrs.get("beta", 1.0)))
        elif op == "Softmax":
            default_axis = 1 if opset < 13 else -1
            encoded_attr = struct.pack("<q", int(attrs.get("axis", default_axis)))
        elif op == "Reshape":
            shape_name = node.input[1]
            shape_spec = [int(value) for value in initializer_arrays[shape_name].reshape(-1)]
            allowzero = int(attrs.get("allowzero", 0))
            if not shape_spec or len(shape_spec) > MAX_RANK:
                raise ValueError(f"invalid Reshape rank at {node.name!r}")
            encoded_attr = struct.pack("<II", len(shape_spec), allowzero)
            encoded_attr += struct.pack("<" + "q" * len(shape_spec), *shape_spec)
        elif op == "Flatten":
            # Flatten has no corresponding hardware operation; represent its
            # contiguous metadata-only reshape explicitly.
            if len(inputs) != 1:
                raise ValueError("Flatten must have exactly one input")
            input_dims = dims_of(inputs[0])
            axis = int(attrs.get("axis", 1))
            if axis < 0:
                axis += len(input_dims)
            if axis < 0 or axis > len(input_dims):
                raise ValueError(f"invalid Flatten axis at {node.name!r}")
            left = int(np.prod(input_dims[:axis], dtype=np.int64))
            right = int(np.prod(input_dims[axis:], dtype=np.int64))
            dims = [left, right]
            encoded_attr = struct.pack("<II", len(dims), 1) + struct.pack("<qq", *dims)
        elif op == "Conv":
            if len(inputs) not in (2, 3):
                raise ValueError(f"Conv {node.name!r} requires weights and optional bias")
            in_dims = dims_of(inputs[0])
            weight_dims = dims_of(inputs[1])
            out_dims = dims_of(output_name)
            if len(in_dims) != 4 or len(weight_dims) != 4 or len(out_dims) != 4:
                raise ValueError(f"Conv {node.name!r} must use 2D NCHW tensors")
            kernel = [int(v) for v in attrs.get("kernel_shape", weight_dims[2:])]
            strides = [int(v) for v in attrs.get("strides", [1, 1])]
            dilations = [int(v) for v in attrs.get("dilations", [1, 1])]
            if len(kernel) != 2 or len(strides) != 2 or len(dilations) != 2:
                raise ValueError(f"Conv {node.name!r} has unsupported spatial attributes")
            if (kernel[0] != kernel[1] or strides[0] != strides[1] or
                    strides[0] < 1 or strides[0] > 4 or
                    dilations[0] < 1 or dilations[1] < 1):
                raise ValueError(f"Conv {node.name!r} requires square kernels and equal stride")
            group = int(attrs.get("group", 1))
            effective = [dilations[i] * (kernel[i] - 1) + 1 for i in range(2)]
            if effective[0] != effective[1] or effective[0] > 11 or group < 1:
                raise ValueError(f"Conv {node.name!r} exceeds the supported effective kernel/group limits")
            if (in_dims[1] % group != 0 or weight_dims[0] % group != 0 or
                    weight_dims[1] != in_dims[1] // group or
                    weight_dims[2:] != kernel):
                raise ValueError(f"Conv {node.name!r} weight/group shapes are inconsistent")
            pads = conv_pads(attrs.get("auto_pad", b"NOTSET"),
                             attrs.get("pads", [0, 0, 0, 0]),
                             in_dims, out_dims, kernel, dilations, strides)
            values = [group, kernel[0], kernel[1], strides[0], strides[1],
                      dilations[0], dilations[1], *pads, 100, 100, 16, 65536]
            encoded_attr = struct.pack("<15I", *values)
        elif op == "MaxPool":
            kernel = [int(v) for v in attrs.get("kernel_shape", [])]
            strides = [int(v) for v in attrs.get("strides", kernel)]
            dilations = [int(v) for v in attrs.get("dilations", [1, 1])]
            pads = [int(v) for v in attrs.get("pads", [0, 0, 0, 0])]
            auto_pad = attrs.get("auto_pad", b"NOTSET")
            if len(kernel) != 2 or len(strides) != 2 or len(dilations) != 2 or len(pads) != 4:
                raise ValueError(f"MaxPool {node.name!r} has unsupported spatial attributes")
            if any(value <= 0 for value in kernel + strides):
                raise ValueError(f"MaxPool {node.name!r} kernel and stride values must be positive")
            if auto_pad not in (b"NOTSET", b"VALID", "NOTSET", "VALID"):
                raise ValueError(f"MaxPool {node.name!r} requires explicit/VALID padding")
            if int(attrs.get("ceil_mode", 0)) != 0 or int(attrs.get("storage_order", 0)) != 0:
                raise ValueError(f"MaxPool {node.name!r} uses unsupported ceil/storage-order settings")
            if kernel[0] != kernel[1] or strides[0] != strides[1] or dilations != [1, 1]:
                raise ValueError(f"MaxPool {node.name!r} requires square windows, equal stride, dilation 1")
            if (any(pad < 0 for pad in pads) or
                    pads[0] >= kernel[0] or pads[2] >= kernel[0] or
                    pads[1] >= kernel[1] or pads[3] >= kernel[1]):
                raise ValueError(f"MaxPool {node.name!r} padding must be nonnegative and smaller than the window")
            in_dims = dims_of(inputs[0])
            out_dims = dims_of(output_name)
            if len(in_dims) != 4 or len(out_dims) != 4:
                raise ValueError(f"MaxPool {node.name!r} must use 2D NCHW tensors")
            expected_h = (in_dims[2] + pads[0] + pads[2] - kernel[0]) // strides[0] + 1
            expected_w = (in_dims[3] + pads[1] + pads[3] - kernel[1]) // strides[1] + 1
            if (expected_h <= 0 or expected_w <= 0 or
                    out_dims != [in_dims[0], in_dims[1], expected_h, expected_w]):
                raise ValueError(f"MaxPool {node.name!r} output shape does not match supported floor-mode geometry")
            values = [kernel[0], strides[0], 1, *pads, int(attrs.get("ceil_mode", 0))]
            encoded_attr = struct.pack("<8I", *values)
        elif op == "Dropout":
            # This bundle is inference-only. Dynamic/true training mode is rejected.
            if len(node.input) > 2 and node.input[2]:
                training_name = node.input[2]
                if training_name not in initializers:
                    raise ValueError(f"Dropout {node.name!r} training mode must be constant false")
                training = bool(np.asarray(initializer_arrays[training_name]).reshape(-1)[0])
                if training:
                    raise ValueError(f"Dropout {node.name!r} is in training mode")
        elif op == "Identity":
            pass
        elif op == "Relu":
            pass

        opcode = OPCODES[op]
        node_records.append((opcode, input_ids, output_id, encoded_attr, node.name or op))

    output_path = Path(output_path)
    with output_path.open("wb") as bundle:
        bundle.write(struct.pack("<4sIIIIII", b"NXB1", 1, len(tensor_records),
                                 len(node_records), len(graph_inputs),
                                 len(graph_outputs), 0))
        for dims, count, initialized, payload in tensor_records:
            bundle.write(struct.pack("<BBHQ", len(dims), int(initialized), 0, count))
            bundle.write(struct.pack("<" + "Q" * len(dims), *dims))
            bundle.write(payload)
        for opcode, inputs, output_id, encoded_attr, _name in node_records:
            padded_inputs = inputs + [U32_NONE] * (3 - len(inputs))
            bundle.write(struct.pack("<BBHI3I", opcode, len(inputs), 0,
                                     output_id, *padded_inputs))
            bundle.write(encoded_attr)
        if graph_inputs:
            bundle.write(struct.pack("<" + "I" * len(graph_inputs), *graph_inputs))
        bundle.write(struct.pack("<" + "I" * len(graph_outputs), *graph_outputs))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", type=Path, help="input ONNX model (external weights are loaded)")
    parser.add_argument("output", type=Path, help="output NXB1 bundle")
    args = parser.parse_args()
    try:
        model = onnx.load(str(args.model), load_external_data=True)
        serialize_bundle(model, args.output)
    except Exception as exc:
        print(f"ONNX conversion failed: {exc}", file=sys.stderr)
        return 1
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
