"""Runs convolution_block through ModelSim/Questa and saves image and raw output.

The convolution block supports input tiles up to 128x128. Larger images are
processed in overlapping tiles, one color plane at a time. Raw convolution
values are saved to output_conv.txt as interleaved RGB values, one integer per
line.

Requires convolution_tb.vhd (in this directory) and vcom/vsim (ModelSim or
Questa) on PATH.
"""
from pathlib import Path

import tb_common as tbc

PNG_INPUT = "input.png"
OUTPUT_PNG = "output_conv.png"
OUTPUT_TXT = "output_conv.txt"
DESIGN_FILES = ["..\\hardware\\hdl\\FPGA_accelerator.vhd", "..\\hardware\\hdl\\convolution_block.vhd"]
TB_FILE = "..\\hardware\\quartus\\simulation\\questa\\convolution_tb.vhd"
TB_ENTITY = "convolution_tb"

KERNEL_SIZE = 3
STRIDE = 1
PADDING = 1

# 3x3 four-neighbor Laplacian edge detector in Q8.8 fixed point.
# The positive center and negative neighbors highlight intensity changes.
EDGE_DETECT_Q88 = [0,   -256, 0,
                   -256, 1024, -256,
                    0,   -256, 0]
BIAS_Q88 = [0]
MAX_TILE_WIDTH = 128
MAX_TILE_HEIGHT = 128


def run_channel(width, height, plane, channel_name):
    tbc.write_values("weight.txt", EDGE_DETECT_Q88)
    tbc.write_values("bias.txt", BIAS_Q88)

    out_h = (height + 2 * PADDING - KERNEL_SIZE) // STRIDE + 1
    out_w = (width + 2 * PADDING - KERNEL_SIZE) // STRIDE + 1
    output_plane = [0] * (out_h * out_w)
    core_height = MAX_TILE_HEIGHT - 2 * PADDING
    core_width = MAX_TILE_WIDTH - 2 * PADDING
    output_path = Path("output.txt")

    if core_height <= 0 or core_width <= 0:
        raise ValueError("Padding leaves no room for an output tile")

    for core_top in range(0, height, core_height):
        core_bottom = min(core_top + core_height, height)
        tile_top = max(0, core_top - PADDING)
        tile_bottom = min(height, core_bottom + PADDING)

        for core_left in range(0, width, core_width):
            core_right = min(core_left + core_width, width)
            tile_left = max(0, core_left - PADDING)
            tile_right = min(width, core_right + PADDING)
            tile_height = tile_bottom - tile_top
            tile_width = tile_right - tile_left
            tile_plane = [
                plane[y * width + x]
                for y in range(tile_top, tile_bottom)
                for x in range(tile_left, tile_right)
            ]
            tbc.write_values("input.txt", tile_plane)

            tile_out_h = (tile_height + 2 * PADDING - KERNEL_SIZE) // STRIDE + 1
            tile_out_w = (tile_width + 2 * PADDING - KERNEL_SIZE) // STRIDE + 1
            generics = {
                "INPUT_HEIGHT": tile_height,
                "INPUT_WIDTH": tile_width,
                "INPUT_CHANNELS": 1,
                "OUTPUT_CHANNELS": 1,
                "KERNEL_SIZE": KERNEL_SIZE,
                "STRIDE": STRIDE,
                "PADDING": PADDING,
                "OUTPUT_HEIGHT": tile_out_h,
                "OUTPUT_WIDTH": tile_out_w,
            }

            if output_path.exists():
                output_path.unlink()
            print(
                f"Running convolution testbench for {channel_name} channel "
                f"tile ({tile_left},{tile_top}) size {tile_width}x{tile_height}..."
            )
            tbc.compile_and_run(DESIGN_FILES, TB_FILE, TB_ENTITY, generics)

            if not output_path.is_file():
                raise RuntimeError(
                    f"Convolution testbench did not produce output.txt for "
                    f"{channel_name} tile ({tile_left},{tile_top})"
                )
            values = [
                int(value)
                for value in output_path.read_text(encoding="ascii").split()
            ]
            expected_count = tile_out_h * tile_out_w
            if len(values) != expected_count:
                raise ValueError(
                    f"{channel_name} tile ({tile_left},{tile_top}) expected "
                    f"{expected_count} outputs, received {len(values)}"
                )

            for y in range(core_top, core_bottom):
                for x in range(core_left, core_right):
                    tile_index = (y - tile_top) * tile_out_w + (x - tile_left)
                    output_plane[y * out_w + x] = values[tile_index]

    return output_plane, out_w, out_h


def main():
    img = tbc.load_image(PNG_INPUT)
    width, height = img.size
    print(f"Loaded PNG: {width}x{height}")
    output_txt_path = Path(OUTPUT_TXT)
    if output_txt_path.exists():
        output_txt_path.unlink()

    r, g, b = tbc.image_to_planes(img)

    out_planes = {}
    out_w = out_h = None
    for idx, (name, plane) in enumerate([("R", r), ("G", g), ("B", b)]):
        values, out_w, out_h = run_channel(width, height, plane, name)
        out_planes[idx] = values

    output_values = []
    for pixel_index in range(out_w * out_h):
        for channel_index in range(3):
            output_values.append(out_planes[channel_index][pixel_index])
    tbc.write_values(OUTPUT_TXT, output_values)

    out_img = tbc.planes_to_image(out_planes, out_w, out_h)
    out_img.save(OUTPUT_PNG)
    print(f"Saved output image to {OUTPUT_PNG}")
    print(f"Saved raw RGB output values to {OUTPUT_TXT}")


if __name__ == "__main__":
    main()
