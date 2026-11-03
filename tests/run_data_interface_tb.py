"""Runs the self-checking testbench for neurax_data_interface through ModelSim/Questa.

Targets neurax_data_interface.vhd (the rd_start_i/rd_busy_o/rd_done_o variant
actually instantiated by neurax.vhd -- neurax_data_interface_new.vhd is an
mSGDMA-compatible variant that isn't wired into the top level yet).

This module instantiates an Altera `altsyncram` (library altera_mf), so that
simulation library must already be mapped in your simulator (normally
provided by a Quartus/ModelSim-Altera install) - this script does not attempt
to map it.

The testbench (neurax_data_interface_tb.vhd) is self-checking: it exits with
the number of failed checks as its status code, so this script just reports
pass/fail based on that exit code - no image or text files are involved.
"""
import sys

import tb_common as tbc

DESIGN_FILES = ["..\\hardware\\hdl\\neurax_data_interface.vhd"]
TB_FILE = "..\\hardware\\quartus\\simulation\\questa\\neurax_data_interface_tb.vhd"
TB_ENTITY = "neurax_data_interface_tb"


def main():
    print("Running neurax_data_interface testbench...")
    passed = tbc.run_self_checking(DESIGN_FILES, TB_FILE, TB_ENTITY)
    sys.exit(0 if passed else 1)


if __name__ == "__main__":
    main()
