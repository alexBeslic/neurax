"""Runs the self-checking testbench for neurax_register_block through ModelSim/Questa.

neurax_register_block.vhd depends on `library altera; use altera.altera_syn_attributes.all;`.
That library must already be mapped in your simulator (normally provided by a
Quartus/ModelSim-Altera installation via its modelsim.ini) - this script does
not attempt to map it.

The testbench (neurax_register_block_tb.vhd) is self-checking: it exits with
the number of failed checks as its status code, so this script just reports
pass/fail based on that exit code - no image or text files are involved.
"""
import sys

import tb_common as tbc

DESIGN_FILES = [
    "..\\hardware\\hdl\\FPGA_accelerator.vhd",
    "..\\hardware\\hdl\\neurax_register_pkg.vhd",
    "..\\hardware\\hdl\\neurax_register_block.vhd",
]
TB_FILE = "..\\hardware\\quartus\\simulation\\questa\\neurax_register_block_tb.vhd"
TB_ENTITY = "neurax_register_block_tb"


def main():
    print("Running neurax_register_block testbench...")
    passed = tbc.run_self_checking(DESIGN_FILES, TB_FILE, TB_ENTITY)
    sys.exit(0 if passed else 1)


if __name__ == "__main__":
    main()
