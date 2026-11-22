#!/usr/bin/env python3
"""Preprocess an image into the runner's raw little-endian float32 input file."""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

try:
    import numpy as np
    from PIL import Image, ImageOps
except ImportError as exc:  # pragma: no cover - depends on host setup
    raise SystemExit(
        "Install image conversion dependencies with: "
        "python3 -m pip install numpy Pillow"
    ) from exc

_BILINEAR = getattr(getattr(Image, "Resampling", Image), "BILINEAR")


def parse_values(text: str, name: str) -> list[float]:
    try:
        values = [float(item.strip()) for item in text.split(",") if item.strip()]
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"{name} must be comma-separated numbers") from exc
    if not values or not all(math.isfinite(value) for value in values):
        raise argparse.ArgumentTypeError(f"{name} must contain finite values")
    return values


def get_model_input(model_path: Path, requested_layout: str):
    try:
        import onnx
    except ImportError as exc:
        raise RuntimeError(
            "Reading input shape from ONNX requires onnx; install requirements-export.txt"
        ) from exc

    model = onnx.load(str(model_path), load_external_data=False)
    initializer_names = {tensor.name for tensor in model.graph.initializer}
    graph_inputs = [value for value in model.graph.input
                    if value.name not in initializer_names]
    if len(graph_inputs) != 1:
        raise ValueError(f"expected one model input, found {len(graph_inputs)}")
    value = graph_inputs[0]
    tensor_type = value.type.tensor_type
    if tensor_type.elem_type != onnx.TensorProto.FLOAT:
        raise ValueError("model input must be float32")
    dims = []
    for dim in tensor_type.shape.dim:
        if not dim.HasField("dim_value") or dim.dim_value <= 0:
            raise ValueError("model input dimensions must be static and positive")
        dims.append(int(dim.dim_value))
    if len(dims) != 4 or dims[0] != 1:
        raise ValueError(f"expected one static rank-4 input with batch 1, got {dims}")

    if requested_layout == "auto":
        nchw = dims[1] in (1, 3)
        nhwc = dims[3] in (1, 3)
        if nchw == nhwc:
            raise ValueError(
                f"cannot infer image layout from shape {dims}; pass --layout nchw or nhwc"
            )
        layout = "nchw" if nchw else "nhwc"
    else:
        layout = requested_layout

    if layout == "nchw":
        channels, height, width = dims[1], dims[2], dims[3]
    else:
        height, width, channels = dims[1], dims[2], dims[3]
    if channels not in (1, 3):
        raise ValueError(f"only 1- or 3-channel images are supported, model has {channels}")
    return layout, channels, height, width


def center_crop_resize(image: Image.Image, height: int, width: int,
                       resize_shorter: int | None) -> Image.Image:
    if resize_shorter is None:
        return image.resize((width, height), _BILINEAR)

    original_width, original_height = image.size
    scale = resize_shorter / min(original_width, original_height)
    resized_width = max(width, int(round(original_width * scale)))
    resized_height = max(height, int(round(original_height * scale)))
    image = image.resize((resized_width, resized_height), _BILINEAR)
    left = (resized_width - width) // 2
    top = (resized_height - height) // 2
    return image.crop((left, top, left + width, top + height))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path, help="input image")
    parser.add_argument("output", type=Path, help="output input.f32 path")
    parser.add_argument("--model", type=Path,
                        help="ONNX model used to read its static input shape")
    parser.add_argument("--height", type=int, help="input height when --model is omitted")
    parser.add_argument("--width", type=int, help="input width when --model is omitted")
    parser.add_argument("--channels", type=int, choices=(1, 3), default=3,
                        help="image channels when --model is omitted (default: 3)")
    parser.add_argument("--layout", choices=("auto", "nchw", "nhwc"), default="auto",
                        help="tensor layout; inferred from --model when possible")
    parser.add_argument("--resize-shorter", type=int, default=256,
                        help="resize shorter image edge before center crop; use 0 for direct resize")
    parser.add_argument("--scale", type=float, default=1.0 / 255.0,
                        help="multiply pixels by this value before normalization (default: 1/255)")
    parser.add_argument("--mean", type=lambda text: parse_values(text, "mean"),
                        help="comma-separated channel means; default is ImageNet RGB or 0 for grayscale")
    parser.add_argument("--std", type=lambda text: parse_values(text, "std"),
                        help="comma-separated channel stddevs; default is ImageNet RGB or 1 for grayscale")
    args = parser.parse_args()

    try:
        if args.model:
            layout, channels, height, width = get_model_input(args.model, args.layout)
        else:
            if not args.height or not args.width:
                raise ValueError("provide --model or both --height and --width")
            layout = "nchw" if args.layout == "auto" else args.layout
            channels, height, width = args.channels, args.height, args.width
        if height <= 0 or width <= 0:
            raise ValueError("input height and width must be positive")
        if not math.isfinite(args.scale):
            raise ValueError("scale must be finite")

        default_mean = [0.485, 0.456, 0.406] if channels == 3 else [0.0]
        default_std = [0.229, 0.224, 0.225] if channels == 3 else [1.0]
        mean = args.mean or default_mean
        std = args.std or default_std
        if len(mean) not in (1, channels) or len(std) not in (1, channels):
            raise ValueError("mean/std must have one value or one value per channel")
        if any(value <= 0 for value in std):
            raise ValueError("standard deviations must be positive")
        if len(mean) == 1:
            mean = mean * channels
        if len(std) == 1:
            std = std * channels

        mode = "RGB" if channels == 3 else "L"
        with Image.open(args.image) as source:
            image = ImageOps.exif_transpose(source).convert(mode)
        resize_shorter = None if args.resize_shorter == 0 else args.resize_shorter
        if resize_shorter is not None and resize_shorter <= 0:
            raise ValueError("--resize-shorter must be positive or 0")
        image = center_crop_resize(image, height, width, resize_shorter)

        pixels = np.asarray(image, dtype=np.float32)
        if channels == 1:
            pixels = pixels[:, :, np.newaxis]
        pixels = pixels * np.float32(args.scale)
        pixels = (pixels - np.asarray(mean, dtype=np.float32).reshape(1, 1, channels)) \
                 / np.asarray(std, dtype=np.float32).reshape(1, 1, channels)
        if layout == "nchw":
            pixels = np.transpose(pixels, (2, 0, 1))[np.newaxis, ...]
        else:
            pixels = pixels[np.newaxis, ...]
        pixels = np.ascontiguousarray(pixels, dtype="<f4")
        if not np.isfinite(pixels).all():
            raise ValueError("preprocessed image contains non-finite values")
        pixels.tofile(args.output)
        print(f"Wrote {args.output}: shape={tuple(pixels.shape)}, "
              f"values={pixels.size}, bytes={pixels.nbytes}, layout={layout}")
        return 0
    except Exception as exc:
        print(f"image conversion failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
