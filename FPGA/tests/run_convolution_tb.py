import subprocess

pixels = [
    42, 7, 199, 34,
    123, 88, 190, 56,
    77, 240, 10, 99,
    12, 67, 18, 150
]

with open("image.txt", "w", newline="\n") as f:
    for p in pixels:
        f.write(f"{p}\n")

# Paths to your files
vhdl_files = [
    "../FPGA_accelerator.vhd",
    "../convolution_block.vhd",
    "../simulation/questa/FPGA_accelerator.vht"
]
subprocess.run(["vlib", "work"], check=True)
# Compile all VHDL files
for vhdl in vhdl_files:
    subprocess.run(["vcom", "-2008", vhdl], check=True)

# Run the simulation
subprocess.run(["vsim", "-c", "FPGA_accelerator_vhd_tst", "-do", "run -all; quit"], check=True)

# Print output.txt results
with open("output.txt") as f:
    print("Simulation output.txt:")
    print(f.read())