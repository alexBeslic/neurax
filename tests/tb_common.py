"""Shared helpers for running the neurax accelerator block testbenches through ModelSim/Questa.

Conventions used by all run_*_tb.py scripts:
- Pixel values (0-255) are used directly as Q8.8 fixed-point raw codes: since
  DATA_WIDTH=16 / FRAC_WIDTH=8, an 8-bit pixel written verbatim represents the
  fraction pixel/256. Convolution/activation weights that also need to be
  fixed point (e.g. kernel taps, alpha) are chosen so the math cancels back
  out to a plain pixel value on the output side - see the comments in each
  run_*_tb.py script for the exact scaling used.
- Text files used for testbench I/O are one signed decimal integer per line.
"""
import subprocess

from PIL import Image


def load_image(path):
    """Load an image file and convert it to RGB."""
    return Image.open(path).convert("RGB")


def write_values(path, values):
    """Write a flat list of integers to a text file, one per line."""
    with open(path, "w") as f:
        for v in values:
            f.write(f"{v}\n")


def read_values(path, expected_len=None):
    """Read a text file of one integer per line into a list."""
    values = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line:
                values.append(int(line))

    if expected_len is not None and len(values) != expected_len:
        print(f"Warning: {path} has {len(values)} values, expected {expected_len}")
        if len(values) < expected_len:
            values += [0] * (expected_len - len(values))
        else:
            values = values[:expected_len]

    return values


def image_to_planes(img):
    """Return (r, g, b) flattened row-major plane lists, each of length W*H."""
    w, h = img.size
    r, g, b = [], [], []
    for y in range(h):
        for x in range(w):
            pr, pg, pb = img.getpixel((x, y))
            r.append(pr)
            g.append(pg)
            b.append(pb)
    return r, g, b


def image_to_interleaved(img):
    """Return a flattened [h][w][c] interleaved RGB list (matches pooling/activation addressing)."""
    w, h = img.size
    values = []
    for y in range(h):
        for x in range(w):
            values.extend(img.getpixel((x, y)))
    return values


def planes_to_image(planes, width, height):
    """Build an RGB image from a dict of channel_index -> flattened W*H plane list."""
    out = Image.new("RGB", (width, height))
    for y in range(height):
        for x in range(width):
            idx = y * width + x
            pixel = tuple(max(0, min(255, planes[c][idx])) for c in range(3))
            out.putpixel((x, y), pixel)
    return out


def interleaved_to_image(values, width, height, channels=3):
    """Build an RGB image from a flattened [h][w][c] interleaved list."""
    out = Image.new("RGB", (width, height))
    for y in range(height):
        for x in range(width):
            idx = (y * width + x) * channels
            pixel = tuple(max(0, min(255, values[idx + c])) for c in range(channels))
            out.putpixel((x, y), pixel)
    return out


def _generic_arg(name, value):
    if isinstance(value, str):
        return f'-g{name}="{value}"'
    if isinstance(value, bool):
        return f"-g{name}={'1' if value else '0'}"
    return f"-g{name}={value}"


def compile_and_run(design_files, tb_file, tb_entity, generics=None, do_cmds="run -all; quit"):
    """Compile the given VHDL sources plus the testbench, then run it under vsim."""
    subprocess.run(["vlib", "work"], check=False)

    for vhdl_file in design_files:
        subprocess.run(["vcom", "-2008", vhdl_file], check=True)
    subprocess.run(["vcom", "-2008", tb_file], check=True)

    cmd = ["vsim", "-c", tb_entity]
    if generics:
        for name, value in generics.items():
            cmd.append(_generic_arg(name, value))
    cmd += ["-do", do_cmds]

    subprocess.run(cmd, check=True)


def run_self_checking(design_files, tb_file, tb_entity, generics=None):
    """Compile+run a self-checking testbench (one that calls std.env.stop(error_count)).

    Returns True if the simulation exited with status 0 (no failed checks),
    False otherwise. Does not raise on a non-zero vsim exit code, so callers
    can report pass/fail without a Python traceback.
    """
    subprocess.run(["vlib", "work"], check=False)

    for vhdl_file in design_files:
        subprocess.run(["vcom", "-2008", vhdl_file], check=True)
    subprocess.run(["vcom", "-2008", tb_file], check=True)

    cmd = ["vsim", "-c", tb_entity]
    if generics:
        for name, value in generics.items():
            cmd.append(_generic_arg(name, value))
    cmd += ["-do", "run -all; quit"]

    result = subprocess.run(cmd, check=False, capture_output=True, text=True)
    output = result.stdout + result.stderr
    print(output, end="")

    if result.returncode == 0 and "ALL CHECKS PASSED" in output and "CHECK(S) FAILED" not in output:
        print(f"{tb_entity}: PASSED")
    else:
        print(f"{tb_entity}: FAILED (see transcript above)")

    return result.returncode == 0 and "ALL CHECKS PASSED" in output and "CHECK(S) FAILED" not in output
