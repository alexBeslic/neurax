"""Runs pooling_block through ModelSim/Questa on an RGB test image.

The block's internal width/height counters support tiles up to 128x128. Larger
images are pooled in aligned tiles, with all RGB channels processed in
parallel. The assembled raw output is saved as interleaved RGB values.

Requires pooling_tb.vhd (in this directory) and vcom/vsim (ModelSim or Questa)
on PATH.
"""
from pathlib import Path

import tb_common as tbc

PNG_INPUT = "input.png"
OUTPUT_PNG = "output_pool.png"
OUTPUT_TXT = "output_pool.txt"
DESIGN_FILES = ["..\\hardware\\hdl\\FPGA_accelerator.vhd", "..\\hardware\\hdl\\pooling_block.vhd"]
TB_FILE = "..\\hardware\\quartus\\simulation\\questa\\pooling_tb.vhd"
TB_ENTITY = "pooling_tb"

CHANNELS = 3
POOL_SIZE = 2
STRIDE = 2
POOL_TYPE = "MAX_POOL"  # MAX_POOL | AVERAGE_POOL | MIN_POOL | SUM_POOL
MAX_TILE_WIDTH = 128
MAX_TILE_HEIGHT = 128


def main():
    img = tbc.load_image(PNG_INPUT)
    width, height = img.size
    print(f"Loaded PNG: {width}x{height}")
    values = tbc.image_to_interleaved(img)

    out_h = (height - POOL_SIZE) // STRIDE + 1
    out_w = (width - POOL_SIZE) // STRIDE + 1
    if out_h <= 0 or out_w <= 0:
        raise ValueError("Input image must be at least as large as the pooling window")

    max_tile_out_h = (MAX_TILE_HEIGHT - POOL_SIZE) // STRIDE + 1
    max_tile_out_w = (MAX_TILE_WIDTH - POOL_SIZE) // STRIDE + 1
    out_values = [0] * (out_h * out_w * CHANNELS)
    tile_output_path = Path("output.txt")

    for output_top in range(0, out_h, max_tile_out_h):
        tile_out_h = min(max_tile_out_h, out_h - output_top)
        input_top = output_top * STRIDE
        tile_height = (tile_out_h - 1) * STRIDE + POOL_SIZE

        for output_left in range(0, out_w, max_tile_out_w):
            tile_out_w = min(max_tile_out_w, out_w - output_left)
            input_left = output_left * STRIDE
            tile_width = (tile_out_w - 1) * STRIDE + POOL_SIZE
            tile_values = []
            for y in range(input_top, input_top + tile_height):
                row_start = (y * width + input_left) * CHANNELS
                row_end = row_start + tile_width * CHANNELS
                tile_values.extend(values[row_start:row_end])
            tbc.write_values("input.txt", tile_values)

            generics = {
                "INPUT_HEIGHT": tile_height,
                "INPUT_WIDTH": tile_width,
                "CHANNELS": CHANNELS,
                "POOL_SIZE": POOL_SIZE,
                "STRIDE": STRIDE,
                "POOL_TYPE_STR": POOL_TYPE,
                "OUTPUT_HEIGHT": tile_out_h,
                "OUTPUT_WIDTH": tile_out_w,
            }

            if tile_output_path.exists():
                tile_output_path.unlink()
            print(
                f"Running pooling tile ({input_left},{input_top}) "
                f"size {tile_width}x{tile_height}..."
            )
            tbc.compile_and_run(DESIGN_FILES, TB_FILE, TB_ENTITY, generics)

            if not tile_output_path.is_file():
                raise RuntimeError(
                    f"Pooling testbench did not produce output.txt for "
                    f"tile ({input_left},{input_top})"
                )
            tile_values_out = [
                int(value)
                for value in tile_output_path.read_text(encoding="ascii").split()
            ]
            expected_count = tile_out_h * tile_out_w * CHANNELS
            if len(tile_values_out) != expected_count:
                raise ValueError(
                    f"Pooling tile ({input_left},{input_top}) expected "
                    f"{expected_count} output values, received {len(tile_values_out)}"
                )

            for tile_y in range(tile_out_h):
                source_start = tile_y * tile_out_w * CHANNELS
                destination_start = (
                    (output_top + tile_y) * out_w + output_left
                ) * CHANNELS
                count = tile_out_w * CHANNELS
                out_values[destination_start:destination_start + count] = (
                    tile_values_out[source_start:source_start + count]
                )

    tbc.write_values(OUTPUT_TXT, out_values)
    out_img = tbc.interleaved_to_image(out_values, out_w, out_h, CHANNELS)
    out_img.save(OUTPUT_PNG)
    print(f"Saved output image to {OUTPUT_PNG}")
    print(f"Saved raw RGB output values to {OUTPUT_TXT}")


if __name__ == "__main__":
    main()
