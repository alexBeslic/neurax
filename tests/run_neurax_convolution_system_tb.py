"""Run an end-to-end image -> RAM -> convolution -> RAM -> image test.

The integrated design reserves RAM words 10000 and 13000 for weights and bias,
so the grayscale input is resized, preserving aspect ratio, to fit within
100x100. The image and returned signed convolution values are saved as PNG/TXT.
"""
from pathlib import Path
import subprocess

from PIL import Image


TEST_DIR = Path(__file__).resolve().parent
PROJECT_DIR = TEST_DIR.parent
IMAGE_FILE = TEST_DIR / "input.png"
INPUT_IMAGE_FILE = TEST_DIR / "neurax_conv_input.png"
INPUT_VALUES_FILE = TEST_DIR / "integration_input.txt"
SIM_OUTPUT_FILE = TEST_DIR / "integration_output.txt"
OUTPUT_VALUES_FILE = TEST_DIR / "neurax_conv_output.txt"
OUTPUT_IMAGE_FILE = TEST_DIR / "neurax_conv_output.png"
TB_ENTITY = "neurax_convolution_system_tb"
TB_FILE = PROJECT_DIR / "hardware" / "quartus" / "simulation" / "questa" / (
    "neurax_convolution_system_tb.vhd"
)
DESIGN_FILES = [
    PROJECT_DIR / "hardware" / "hdl" / "FPGA_accelerator.vhd",
    PROJECT_DIR / "hardware" / "hdl" / "neurax_register_pkg.vhd",
    PROJECT_DIR / "hardware" / "hdl" / "neurax_register_block.vhd",
    PROJECT_DIR / "hardware" / "hdl" / "neurax_data_interface.vhd",
    PROJECT_DIR / "hardware" / "hdl" / "convolution_block.vhd",
    PROJECT_DIR / "hardware" / "hdl" / "neurax.vhd",
    TB_FILE,
]

MAX_INPUT_DIMENSION = 100
EDGE_WEIGHTS = (0, -256, 0, -256, 1024, -256, 0, -256, 0)


def prepare_input_image():
    with Image.open(IMAGE_FILE) as source:
        image = source.convert("L")
    image.thumbnail(
        (MAX_INPUT_DIMENSION, MAX_INPUT_DIMENSION),
        Image.Resampling.LANCZOS,
    )
    width, height = image.size
    if width < 3 or height < 3:
        raise ValueError("Input image must be at least 3x3 for the 3x3 kernel")
    if width * height > 10000:
        raise ValueError("Input image exceeds the RAM region before the weight table")

    image.save(INPUT_IMAGE_FILE)
    pixels = list(image.getdata())
    INPUT_VALUES_FILE.write_text(
        "".join(f"{pixel}\n" for pixel in pixels),
        encoding="ascii",
    )
    print(f"Prepared grayscale input: {width}x{height}")
    print(f"Saved testbench input pixels to {INPUT_VALUES_FILE.name}")
    return width, height, pixels


def expected_edge_output(width, height, pixels):
    output = []
    for y in range(height):
        for x in range(width):
            center = pixels[y * width + x]
            accumulated = EDGE_WEIGHTS[4] * center
            neighbors = (
                (y - 1, x, EDGE_WEIGHTS[1]),
                (y, x - 1, EDGE_WEIGHTS[3]),
                (y, x + 1, EDGE_WEIGHTS[5]),
                (y + 1, x, EDGE_WEIGHTS[7]),
            )
            for neighbor_y, neighbor_x, weight in neighbors:
                if 0 <= neighbor_y < height and 0 <= neighbor_x < width:
                    accumulated += weight * pixels[neighbor_y * width + neighbor_x]
            output.append(accumulated >> 8)
    return output


def run_simulation(width, height):
    subprocess.run(["vlib", "work"], cwd=TEST_DIR, check=False)
    for source in DESIGN_FILES:
        subprocess.run(
            ["vcom", "-2008", str(source)],
            cwd=TEST_DIR,
            check=True,
        )

    if SIM_OUTPUT_FILE.exists():
        SIM_OUTPUT_FILE.unlink()

    command = [
        "vsim",
        "-c",
        TB_ENTITY,
        f"-gINPUT_WIDTH={width}",
        f"-gINPUT_HEIGHT={height}",
        "-do",
        "run -all; quit",
    ]
    result = subprocess.run(
        command,
        cwd=TEST_DIR,
        check=False,
        capture_output=True,
        text=True,
    )
    output = result.stdout + result.stderr
    print(output, end="")
    if (
        result.returncode != 0
        or "neurax_convolution_system_tb: ALL CHECKS PASSED" not in output
        or "CHECK(S) FAILED" in output
    ):
        raise RuntimeError("Integrated convolution simulation failed")
    if not SIM_OUTPUT_FILE.is_file():
        raise RuntimeError("Testbench completed without creating its RAM readback file")


def main():
    width, height, pixels = prepare_input_image()
    run_simulation(width, height)

    actual = [
        int(value)
        for value in SIM_OUTPUT_FILE.read_text(encoding="ascii").split()
    ]
    expected_count = width * height
    if len(actual) != expected_count:
        raise ValueError(
            f"Expected {expected_count} RAM output words, received {len(actual)}"
        )

    OUTPUT_VALUES_FILE.write_text(
        "".join(f"{value}\n" for value in actual),
        encoding="ascii",
    )
    output_image = Image.new("L", (width, height))
    output_image.putdata([max(0, min(255, value)) for value in actual])
    output_image.save(OUTPUT_IMAGE_FILE)
    print(f"Read and saved {len(actual)} output pixels from RAM")
    print(f"Saved raw RAM output to {OUTPUT_VALUES_FILE.name}")
    print(f"Saved output image to {OUTPUT_IMAGE_FILE.name}")

    expected = expected_edge_output(width, height, pixels)
    mismatches = [
        (index, expected_value, actual_value)
        for index, (expected_value, actual_value) in enumerate(zip(expected, actual))
        if expected_value != actual_value
    ]
    if mismatches:
        index, expected_value, actual_value = mismatches[0]
        x = index % width
        y = index // width
        raise ValueError(
            f"Convolution reference mismatch at ({x},{y}): expected "
            f"{expected_value}, received {actual_value}; {len(mismatches)} "
            f"of {expected_count} pixels differ. RAM output files were saved."
        )

    print(f"Verified all {len(actual)} output pixels against the CPU reference")


if __name__ == "__main__":
    main()
