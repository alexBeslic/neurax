import subprocess
from pathlib import Path

from PIL import Image


# Select convolution, activation, both, or neither.
RUN_CONVOLUTION_TEST = True
RUN_ACTIVATION_TEST = True

TEST_DIR = Path(__file__).resolve().parent
PROJECT_DIR = TEST_DIR.parent
PNG_INPUT = TEST_DIR / "input.png"
INPUT_TXT = TEST_DIR / "image.txt"
OUTPUT_TXT = TEST_DIR / "output.txt"
OUTPUT_PNG = TEST_DIR / "output.png"
OUTPUT_PNG_FALLBACK = TEST_DIR / "output_fallback.png"
TESTBENCH = PROJECT_DIR / "hardware/quartus/simulation/questa/conv_activation_tb.vht"
VHDL_FILES = [
    PROJECT_DIR / "hardware/hdl/FPGA_accelerator.vhd",
    PROJECT_DIR / "hardware/hdl/convolution_block.vhd",
    PROJECT_DIR / "hardware/hdl/activation_block.vhd",
    TESTBENCH,
]

MAX_WIDTH = 128
MAX_HEIGHT = 128
CHANNELS = 3
KERNEL_RADIUS = 1
TILE_CORE_WIDTH = MAX_WIDTH - 2 * KERNEL_RADIUS
TILE_CORE_HEIGHT = MAX_HEIGHT - 2 * KERNEL_RADIUS

if not RUN_CONVOLUTION_TEST and not RUN_ACTIVATION_TEST:
    print("No tests selected; skipping simulation and image I/O.")
    raise SystemExit(0)

image = Image.open(PNG_INPUT).convert("RGB")
image_width, image_height = image.size
print(f"Loaded PNG: {image_width}x{image_height}")

subprocess.run(["vlib", "work"], cwd=TEST_DIR, check=True)
for vhdl_file in VHDL_FILES:
    subprocess.run(["vcom", "-2008", str(vhdl_file)], cwd=TEST_DIR, check=True)

output_image = Image.new("RGB", (image_width, image_height))

for core_top in range(0, image_height, TILE_CORE_HEIGHT):
    core_bottom = min(core_top + TILE_CORE_HEIGHT, image_height)
    tile_top = max(0, core_top - KERNEL_RADIUS)
    tile_bottom = min(image_height, core_bottom + KERNEL_RADIUS)

    for core_left in range(0, image_width, TILE_CORE_WIDTH):
        core_right = min(core_left + TILE_CORE_WIDTH, image_width)
        tile_left = max(0, core_left - KERNEL_RADIUS)
        tile_right = min(image_width, core_right + KERNEL_RADIUS)
        tile = image.crop((tile_left, tile_top, tile_right, tile_bottom))
        tile_width, tile_height = tile.size

        with INPUT_TXT.open("w", encoding="ascii") as image_file:
            for pixel in tile.getdata():
                for channel_value in pixel:
                    image_file.write(f"{channel_value}\n")

        subprocess.run(
            [
                "vsim", "-c",
                f"-gRUN_CONVOLUTION={str(RUN_CONVOLUTION_TEST).lower()}",
                f"-gRUN_ACTIVATION={str(RUN_ACTIVATION_TEST).lower()}",
                "-gIMAGE_MODE=true",
                f"-gIMG_WIDTH={tile_width}",
                f"-gIMG_HEIGHT={tile_height}",
                f'-gINPUT_FILE="{INPUT_TXT.name}"',
                f'-gOUTPUT_FILE="{OUTPUT_TXT.name}"',
                "conv_activation_tb",
                "-do", "run -all; quit -f",
            ],
            cwd=TEST_DIR,
            check=True,
        )

        output_values = [int(value) for value in OUTPUT_TXT.read_text(encoding="ascii").split()]
        expected_values = tile_width * tile_height * CHANNELS
        if len(output_values) != expected_values:
            raise ValueError(
                f"Tile ({tile_left},{tile_top}) expected {expected_values} channel values, "
                f"received {len(output_values)}"
            )

        for y in range(core_top, core_bottom):
            for x in range(core_left, core_right):
                tile_pixel = (y - tile_top) * tile_width + (x - tile_left)
                offset = tile_pixel * CHANNELS
                rgb = output_values[offset:offset + CHANNELS]
                output_image.putpixel(
                    (x, y), tuple(max(0, min(255, value)) for value in rgb)
                )

try:
    output_image.save(OUTPUT_PNG)
    print(f"Saved result image to {OUTPUT_PNG}")
except OSError as error:
    output_image.save(OUTPUT_PNG_FALLBACK)
    print(f"Could not replace {OUTPUT_PNG}: {error}")
    print(f"Saved result image to {OUTPUT_PNG_FALLBACK} instead")
