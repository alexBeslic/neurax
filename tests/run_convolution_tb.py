import subprocess
from pathlib import Path
from PIL import Image

# --- CONFIG ---
TEST_DIR    = Path(__file__).resolve().parent
PROJECT_DIR = TEST_DIR.parent
PNG_INPUT   = TEST_DIR / "input.png"
IMAGE_TXT   = TEST_DIR / "image.txt"
OUTPUT_TXT  = TEST_DIR / "output.txt"
OUTPUT_PNG  = TEST_DIR / "output.png"
VHDL_FILES  = [
    PROJECT_DIR / "hardware/hdl/FPGA_accelerator.vhd",
    PROJECT_DIR / "hardware/hdl/convolution_block.vhd",
    PROJECT_DIR / "hardware/quartus/simulation/questa/convolution_io_tb.vht",
]
TB_ENTITY  = "simple_conv_test"
CHANNELS   = 3
MAX_WIDTH  = 128
MAX_HEIGHT = 128
KERNEL_RADIUS = 1
TILE_CORE_WIDTH = MAX_WIDTH - 2 * KERNEL_RADIUS
TILE_CORE_HEIGHT = MAX_HEIGHT - 2 * KERNEL_RADIUS

## --- Load PNG ---
img = Image.open(PNG_INPUT).convert("RGB")
IMG_WIDTH, IMG_HEIGHT = img.size
print(f"Loaded PNG: {IMG_WIDTH}x{IMG_HEIGHT}")

# --- Compile VHDL ---
subprocess.run(["vlib", "work"], cwd=TEST_DIR, check=True)
for vhdl in VHDL_FILES:
    subprocess.run(["vcom", "-2008", str(vhdl)], cwd=TEST_DIR, check=True)

# --- Run simulation ---
output_img = Image.new("RGB", (IMG_WIDTH, IMG_HEIGHT))

for core_top in range(0, IMG_HEIGHT, TILE_CORE_HEIGHT):
    core_bottom = min(core_top + TILE_CORE_HEIGHT, IMG_HEIGHT)
    tile_top = max(0, core_top - KERNEL_RADIUS)
    tile_bottom = min(IMG_HEIGHT, core_bottom + KERNEL_RADIUS)

    for core_left in range(0, IMG_WIDTH, TILE_CORE_WIDTH):
        core_right = min(core_left + TILE_CORE_WIDTH, IMG_WIDTH)
        tile_left = max(0, core_left - KERNEL_RADIUS)
        tile_right = min(IMG_WIDTH, core_right + KERNEL_RADIUS)
        tile = img.crop((tile_left, tile_top, tile_right, tile_bottom))
        tile_width, tile_height = tile.size

        # Write HWC channel values expected by the testbench.
        with IMAGE_TXT.open("w") as f:
            for pixel in tile.getdata():
                for channel_value in pixel:
                    f.write(f"{channel_value}\n")

        subprocess.run(
            [
                "vsim", "-c",
                f"-gIMG_WIDTH={tile_width}",
                f"-gIMG_HEIGHT={tile_height}",
                "-gKERNEL_TYPE=4",
                TB_ENTITY,
                "-do", "run -all; quit -f",
            ],
            cwd=TEST_DIR,
            check=True,
        )

        tile_output = []
        with OUTPUT_TXT.open("r") as f:
            for line_number, line in enumerate(f, start=1):
                values = line.strip().split()
                if len(values) != CHANNELS:
                    raise ValueError(
                        f"{OUTPUT_TXT}:{line_number}: expected {CHANNELS} channel values"
                    )
                tile_output.append(tuple(int(value) for value in values))

        expected_pixels = tile_width * tile_height
        if len(tile_output) != expected_pixels:
            raise ValueError(
                f"Expected {expected_pixels} output pixels, received {len(tile_output)}"
            )

        # Discard the overlap; the full-image edges retain the testbench's zero padding.
        for y in range(core_top, core_bottom):
            for x in range(core_left, core_right):
                tile_index = (y - tile_top) * tile_width + (x - tile_left)
                rgb = tile_output[tile_index]
                output_img.putpixel(
                    (x, y), tuple(max(0, min(255, value)) for value in rgb)
                )

output_img.save(OUTPUT_PNG)
print(f"Saved output image to {OUTPUT_PNG}")