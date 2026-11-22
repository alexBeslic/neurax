#!/usr/bin/env python3
"""Convert a raw float32 image tensor (for example output.f32) to an image."""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

try:
    import numpy as np
    from PIL import Image
except ImportError as exc:  # pragma: no cover - depends on host setup
    raise SystemExit(
        "Install image conversion dependencies with: "
        "python3 -m pip install numpy Pillow"
    ) from exc


def parse_values(text: str, name: str) -> list[float]:
    try:
        values = [float(item.strip()) for item in text.split(",") if item.strip()]
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"{name} must be comma-separated numbers") from exc
    if not values or not all(math.isfinite(value) for value in values):
        raise argparse.ArgumentTypeError(f"{name} must contain finite values")
    return values


def get_model_output(model_path: Path, requested_layout: str):
    try:
        import onnx
    except ImportError as exc:
        raise RuntimeError(
            "Reading output shape from ONNX requires onnx; install requirements-export.txt"
        ) from exc

    model = onnx.load(str(model_path), load_external_data=False)
    if len(model.graph.output) != 1:
        raise ValueError(f"expected one model output, found {len(model.graph.output)}")
    value = model.graph.output[0]
    tensor_type = value.type.tensor_type
    if tensor_type.elem_type != onnx.TensorProto.FLOAT:
        raise ValueError("model output must be float32")
    dims = []
    for dim in tensor_type.shape.dim:
        if not dim.HasField("dim_value") or dim.dim_value <= 0:
            raise ValueError("model output dimensions must be static and positive")
        dims.append(int(dim.dim_value))
    if len(dims) != 4 or dims[0] != 1:
        raise ValueError(f"expected one static rank-4 output with batch 1, got {dims}")

    layout = infer_layout(dims, requested_layout)
    if layout == "nchw":
        channels, height, width = dims[1], dims[2], dims[3]
    else:
        height, width, channels = dims[1], dims[2], dims[3]
    return layout, channels, height, width


def infer_layout(dims: list[int], requested_layout: str) -> str:
    if requested_layout != "auto":
        return requested_layout
    nchw = dims[1] in (1, 3)
    nhwc = dims[3] in (1, 3)
    if nchw == nhwc:
        raise ValueError(
            f"cannot infer image layout from shape {dims}; pass --layout nchw or nhwc"
        )
    return "nchw" if nchw else "nhwc"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="raw contiguous float32 tensor")
    parser.add_argument("output", type=Path, help="output image path, e.g. output.png")
    parser.add_argument("--model", type=Path,
                        help="ONNX model used to read its static output shape")
    parser.add_argument("--height", type=int, help="image height when --model is omitted")
    parser.add_argument("--width", type=int, help="image width when --model is omitted")
    parser.add_argument("--channels", type=int, choices=(1, 3), default=3,
                        help="image channels when --model is omitted (default: 3)")
    parser.add_argument("--layout", choices=("auto", "nchw", "nhwc"), default="auto",
                        help="tensor layout; inferred from --model when possible")
    parser.add_argument("--byte-order", choices=("little", "big"), default="little",
                        help="float32 byte order in input file (default: little)")
    parser.add_argument("--value-range", choices=("unit", "byte"), default="unit",
                        help="image value range after optional de-normalization (default: 0..1)")
    parser.add_argument("--scale", type=float, default=1.0,
                        help="reverse the input scaling: divide (tensor*std+mean) by scale")
    parser.add_argument("--mean", type=lambda text: parse_values(text, "mean"),
                        help="optional comma-separated mean(s) to reverse normalization")
    parser.add_argument("--std", type=lambda text: parse_values(text, "std"),
                        help="optional comma-separated stddev(s) to reverse normalization")
    args = parser.parse_args()

    try:
        if args.model:
            layout, channels, height, width = get_model_output(args.model, args.layout)
        else:
            if args.height is None or args.width is None:
                raise ValueError("provide --model or both --height and --width")
            if args.height <= 0 or args.width <= 0:
                raise ValueError("input height and width must be positive")
            layout = "nchw" if args.layout == "auto" else args.layout
            channels, height, width = args.channels, args.height, args.width

        if channels not in (1, 3):
            raise ValueError(f"only 1- or 3-channel images are supported, got {channels}")
        if not math.isfinite(args.scale) or args.scale == 0:
            raise ValueError("scale must be finite and non-zero")
        mean = args.mean or [0.0]
        std = args.std or [1.0]
        if len(mean) not in (1, channels) or len(std) not in (1, channels):
            raise ValueError("mean/std must have one value or one value per channel")
        if any(value <= 0 for value in std):
            raise ValueError("standard deviations must be positive")
        if len(mean) == 1:
            mean = mean * channels
        if len(std) == 1:
            std = std * channels

        dtype = "<f4" if args.byte_order == "little" else ">f4"
        tensor = np.fromfile(args.input, dtype=dtype)
        expected = channels * height * width
        if tensor.size != expected:
            raise ValueError(
                f"file has {tensor.size} float32 values; expected {expected} "
                f"for shape (1,{channels},{height},{width}) or its NHWC equivalent"
            )
        if not np.isfinite(tensor).all():
            raise ValueError("input tensor contains non-finite values")

        if layout == "nchw":
            pixels = tensor.reshape(channels, height, width).transpose(1, 2, 0)
        else:
            pixels = tensor.reshape(height, width, channels)
        pixels = (pixels * np.asarray(std, dtype=np.float32).reshape(1, 1, channels)
                  + np.asarray(mean, dtype=np.float32).reshape(1, 1, channels))
        pixels = pixels / np.float32(args.scale)
        if args.value_range == "unit":
            pixels = pixels * np.float32(255.0)
        pixels = np.clip(np.rint(pixels), 0, 255).astype(np.uint8)
        if channels == 1:
            image = Image.fromarray(pixels[:, :, 0])
        else:
            image = Image.fromarray(pixels)
        image.save(args.output)
        print(f"Wrote {args.output}: size={width}x{height}, channels={channels}, "
              f"layout={layout}")
        return 0
    except Exception as exc:
        print(f"image conversion failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
