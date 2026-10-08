"""Runs activation_block through ModelSim/Questa on an RGB test image.

activation_block only accepts tensor_size <= MAX_TENSOR_SIZE (4096), so the
flattened image is processed in chunks by activation_tb. The raw output is
saved to activation_output.txt as one integer per line.

Requires activation_tb.vhd (in this directory) and vcom/vsim (ModelSim or
Questa) on PATH.
"""
from pathlib import Path

import tb_common as tbc

PNG_INPUT = "input.png"
OUTPUT_PNG = "output_act.png"
OUTPUT_TXT = "activation_output.txt"
DESIGN_FILES = ["..\\hardware\\hdl\\FPGA_accelerator.vhd", "..\\hardware\\hdl\\activation_block.vhd"]
TB_FILE = "..\\hardware\\quartus\\simulation\\questa\\activation_tb.vhd"
TB_ENTITY = "activation_tb"

CHANNELS = 3
CHUNK_SIZE = 4096  # must divide TOTAL_ELEMENTS and be <= MAX_TENSOR_SIZE
ACT_TYPE = "RELU"  # RELU | SIGMOID | TANH | LINEAR | LEAKY_RELU | ELU
ALPHA_Q88 = 26     # ~0.1 in Q8.8, used by LEAKY_RELU / ELU


def main():
    img = tbc.load_image(PNG_INPUT)
    width, height = img.size
    print(f"Loaded PNG: {width}x{height}")

    total_elements = width * height * CHANNELS
    assert total_elements % CHUNK_SIZE == 0, "CHUNK_SIZE must evenly divide the flattened image size"

    values = tbc.image_to_interleaved(img)
    tbc.write_values("input.txt", values)
    simulation_output = Path("output.txt")
    output_txt = Path(OUTPUT_TXT)
    for path in (simulation_output, output_txt):
        if path.exists():
            path.unlink()

    generics = {
        "TOTAL_ELEMENTS": total_elements,
        "CHUNK_SIZE": CHUNK_SIZE,
        "ACT_TYPE_STR": ACT_TYPE,
        "ALPHA_RAW": ALPHA_Q88,
    }

    print("Running activation testbench...")
    tbc.compile_and_run(DESIGN_FILES, TB_FILE, TB_ENTITY, generics)

    if not simulation_output.is_file():
        raise RuntimeError("Activation testbench did not produce output.txt")
    out_values = [
        int(value)
        for value in simulation_output.read_text(encoding="ascii").split()
    ]
    if len(out_values) != total_elements:
        raise ValueError(
            f"Activation testbench expected {total_elements} outputs, "
            f"received {len(out_values)}"
        )
    tbc.write_values(OUTPUT_TXT, out_values)

    out_img = tbc.interleaved_to_image(out_values, width, height, CHANNELS)
    out_img.save(OUTPUT_PNG)
    print(f"Saved output image to {OUTPUT_PNG}")
    print(f"Saved raw output values to {OUTPUT_TXT}")


if __name__ == "__main__":
    main()
