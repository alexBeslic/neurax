#!/usr/bin/env python3
"""Prepare input.png for the board UIO convolution test and render its RAM dump.

Usage:
  python3 tests/neurax_uio_edge_image.py prepare
  # Copy software/driver/uio/input_pixels.bin beside the test binary on the board.
  # Run neurax_uio_test_conv there, then copy its res.txt back to this workspace.
  python3 tests/neurax_uio_edge_image.py render

The accelerator has a fixed 100x100 input plane. The image is converted to
luminance and resized to 100x100; C loads the resulting 8-bit pixels as signed
Q24.8 values. The output is a no-padding 3x3 four-neighbor Laplacian plane
(98x98), rendered as absolute edge strength normalized to 8-bit grayscale.
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
INPUT_PNG = ROOT / "tests" / "input.png"
PREPARED_PNG = ROOT / "tests" / "neurax_input_100x100.png"
REFERENCE_PNG = ROOT / "tests" / "neurax_edge_reference.png"
PIXELS_BIN = ROOT / "software" / "driver" / "uio" / "input_pixels.bin"
RAM_DUMP = ROOT / "res.txt"
OUTPUT_PNG = ROOT / "tests" / "neurax_edge_output.png"
INPUT_WIDTH = 100
INPUT_HEIGHT = 100
OUTPUT_WIDTH = INPUT_WIDTH - 2
OUTPUT_HEIGHT = INPUT_HEIGHT - 2
RAM_OUTPUT_BASE = 13016
OUTPUT_WORDS = OUTPUT_WIDTH * OUTPUT_HEIGHT
RAM_LINE = re.compile(r"^Read at (\d+): 0x([0-9a-fA-F]{8})$")


def prepare() -> None:
    resampling = getattr(Image, "Resampling", Image)
    with Image.open(INPUT_PNG) as source:
        image = source.convert("L").resize(
            (INPUT_WIDTH, INPUT_HEIGHT), resampling.LANCZOS
        )
    PREPARED_PNG.parent.mkdir(parents=True, exist_ok=True)
    PIXELS_BIN.parent.mkdir(parents=True, exist_ok=True)
    image.save(PREPARED_PNG)
    pixel_bytes = image.tobytes()
    PIXELS_BIN.write_bytes(pixel_bytes)

    pixels = list(pixel_bytes)
    reference = Image.new("L", (OUTPUT_WIDTH, OUTPUT_HEIGHT))
    reference.putdata(
        [
            min(255, (abs(expected_laplacian(pixels, y, x) // 256) * 255 + 510) // 1020)
            for y in range(OUTPUT_HEIGHT)
            for x in range(OUTPUT_WIDTH)
        ]
    )
    reference.save(REFERENCE_PNG)
    print(f"Prepared {INPUT_PNG.name} as {INPUT_WIDTH}x{INPUT_HEIGHT} grayscale.")
    print(f"Input preview: {PREPARED_PNG}")
    print(f"CPU-reference edge image: {REFERENCE_PNG}")
    print(f"Copy this file beside the board test binary: {PIXELS_BIN}")


def byte_swap32(value: int) -> int:
    return int.from_bytes(value.to_bytes(4, "little"), "big")


def read_ram_dump(path: Path) -> dict[int, int]:
    words: dict[int, int] = {}
    with path.open("r", encoding="ascii") as stream:
        for line in stream:
            match = RAM_LINE.match(line.strip())
            if match:
                words[int(match.group(1))] = int(match.group(2), 16)
    return words


def expected_laplacian(pixels: list[int], y: int, x: int) -> int:
    center = pixels[(y + 1) * INPUT_WIDTH + x + 1]
    neighbors = (
        pixels[y * INPUT_WIDTH + x + 1]
        + pixels[(y + 1) * INPUT_WIDTH + x]
        + pixels[(y + 1) * INPUT_WIDTH + x + 2]
        + pixels[(y + 2) * INPUT_WIDTH + x + 1]
    )
    return (4 * center - neighbors) * 256


def render() -> None:
    if not PREPARED_PNG.exists():
        prepare()
    with Image.open(PREPARED_PNG) as source:
        pixels = list(source.convert("L").getdata())

    words = read_ram_dump(RAM_DUMP)
    missing = [
        address
        for address in range(RAM_OUTPUT_BASE, RAM_OUTPUT_BASE + OUTPUT_WORDS)
        if address not in words
    ]
    if missing:
        raise ValueError(
            f"RAM dump is missing output address {missing[0]} "
            f"({len(missing)} of {OUTPUT_WORDS} output words absent)"
        )

    logical_outputs: list[int] = []
    image_pixels: list[int] = []
    mismatches: list[tuple[int, int, int]] = []
    for index in range(OUTPUT_WORDS):
        y, x = divmod(index, OUTPUT_WIDTH)
        raw_port_a = words[RAM_OUTPUT_BASE + index]
        logical = byte_swap32(raw_port_a)
        if logical & 0x80000000:
            logical -= 1 << 32
        expected = expected_laplacian(pixels, y, x)
        logical_outputs.append(logical)
        if logical != expected and len(mismatches) < 10:
            mismatches.append((index, expected, logical))

        response = logical // 256
        intensity = min(255, (abs(response) * 255 + 510) // 1020)
        image_pixels.append(intensity)

    result_image = Image.new("L", (OUTPUT_WIDTH, OUTPUT_HEIGHT))
    result_image.putdata(image_pixels)
    result_image.save(OUTPUT_PNG)
    print(f"Saved edge image: {OUTPUT_PNG} ({OUTPUT_WIDTH}x{OUTPUT_HEIGHT})")

    if mismatches:
        index, expected, actual = mismatches[0]
        y, x = divmod(index, OUTPUT_WIDTH)
        raise ValueError(
            f"CPU-reference mismatch at output ({x},{y}): "
            f"expected Q24.8 {expected}, got {actual}; "
            f"first {len(mismatches)} mismatches listed by the RAM data."
        )
    print(f"Verified all {len(logical_outputs)} output pixels against the CPU reference.")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "render"))
    args = parser.parse_args()
    if args.action == "prepare":
        prepare()
    else:
        render()


if __name__ == "__main__":
    main()
