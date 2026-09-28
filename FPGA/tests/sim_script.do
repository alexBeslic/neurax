
# Load design
vsim -t ps tb_fpga_accelerator

# Add waves (optional - zakomentarisano da ubrza simulaciju)
# add wave -radix decimal sim:/tb_fpga_accelerator/*

# Run simulation
run 10000 ns

# Check if simulation completed
if {[examine -time] == "0 ns"} {
    echo "ERROR: Simulation did not advance"
    quit -f
}

# Quit
quit -f
