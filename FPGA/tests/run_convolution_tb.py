import subprocess
from PIL import Image

# --- CONFIG ---
PNG_INPUT  = "input.png"
IMAGE_TXT  = "image.txt"
OUTPUT_TXT = "output.txt"
OUTPUT_PNG = "output.png"
VHDL_FILES = ["../activation_block.vhd", "../FPGA_accelerator.vhd", "../convolution_block.vhd", "../simulation/questa/FPGA_accelerator.vht"]
TB_ENTITY  = "tb_conv_file"
BPP        = 8
CHANNELS   = 3

## --- Load PNG ---
img = Image.open(PNG_INPUT).convert("RGB")
IMG_WIDTH, IMG_HEIGHT = img.size
print(f"Loaded PNG: {IMG_WIDTH}x{IMG_HEIGHT}")

# --- Flatten pixels ---
pixels = []
for y in range(IMG_HEIGHT):
    for x in range(IMG_WIDTH):
        r, g, b = img.getpixel((x, y))
        pixels.extend([r, g, b])

# --- Write to image.txt ---
with open(IMAGE_TXT, "w") as f:
    for p in pixels:
        f.write(f"{p}\n")
print(f"Wrote {len(pixels)} pixel values to {IMAGE_TXT}")

# --- Compile VHDL ---
subprocess.run(["vlib", "work"], check=True)
for vhdl in VHDL_FILES:
    subprocess.run(["vcom", "-2008", vhdl], check=True)

# --- Run simulation ---
subprocess.run(["vsim", "-c", TB_ENTITY, "-do", "run -all; quit"], check=True)

# --- Read output.txt ---
lines = []
with open(OUTPUT_TXT, "r") as f:
    for l in f.readlines():
        vals = l.strip().split()
        if len(vals) == 3:
            lines.extend([int(v) for v in vals])

expected_len = IMG_WIDTH*IMG_HEIGHT*CHANNELS
if len(lines) != expected_len:
    print("Warning: output length does not match image dimensions!")
    if len(lines) < expected_len:
        print("Padding output with zeros.")
        lines += [0] * (expected_len - len(lines))

# --- Convert to PNG ---
output_img = Image.new("RGB", (IMG_WIDTH, IMG_HEIGHT))

for y in range(IMG_HEIGHT):
    for x in range(IMG_WIDTH):
        idx = (y*IMG_WIDTH + x) * CHANNELS
        r = lines[idx]
        g = lines[idx+1]
        b = lines[idx+2]

        # Clamp to 0-255 for visualization
        r = max(0, min(255, r))
        g = max(0, min(255, g))
        b = max(0, min(255, b))

        output_img.putpixel((x, y), (r, g, b))

output_img.save(OUTPUT_PNG)
print(f"Saved output image to {OUTPUT_PNG}")
