
# (C) 2001-2026 Altera Corporation. All rights reserved.
# Your use of Altera Corporation's design tools, logic functions and 
# other software and tools, and its AMPP partner logic functions, and 
# any output files any of the foregoing (including device programming 
# or simulation files), and any associated documentation or information 
# are expressly subject to the terms and conditions of the Altera 
# Program License Subscription Agreement, Altera MegaCore Function 
# License Agreement, or other applicable license agreement, including, 
# without limitation, that your use is for the sole purpose of 
# programming logic devices manufactured by Altera and sold by Altera 
# or its authorized distributors. Please refer to the applicable 
# agreement for further details.

# ACDS 24.1 1077 linux 2026.03.14.16:26:46

# ----------------------------------------
# xcelium - auto-generated simulation script

# ----------------------------------------
# This script provides commands to simulate the following IP detected in
# your Quartus project:
#     soc_system
# 
# Altera recommends that you source this Quartus-generated IP simulation
# script from your own customized top-level script, and avoid editing this
# generated script.
# 
# Xcelium Simulation Script.
# To write a top-level shell script that compiles Intel simulation libraries
# and the Quartus-generated IP in your project, along with your design and
# testbench files, copy the text from the TOP-LEVEL TEMPLATE section below
# into a new file, e.g. named "xcelium_sim.sh", and modify text as directed.
# 
# You can also modify the simulation flow to suit your needs. Set the
# following variables to 1 to disable their corresponding processes:
# - SKIP_FILE_COPY: skip copying ROM/RAM initialization files
# - SKIP_DEV_COM: skip compiling the Quartus EDA simulation library
# - SKIP_COM: skip compiling Quartus-generated IP simulation files
# - SKIP_ELAB and SKIP_SIM: skip elaboration and simulation
# 
# ----------------------------------------
# # TOP-LEVEL TEMPLATE - BEGIN
# #
# # QSYS_SIMDIR is used in the Quartus-generated IP simulation script to
# # construct paths to the files required to simulate the IP in your Quartus
# # project. By default, the IP script assumes that you are launching the
# # simulator from the IP script location. If launching from another
# # location, set QSYS_SIMDIR to the output directory you specified when you
# # generated the IP script, relative to the directory from which you launch
# # the simulator. In this case, you must also copy the generated files
# # "cds.lib" and "hdl.var" - plus the directory "cds_libs" if generated - 
# # into the location from which you launch the simulator, or incorporate
# # into any existing library setup.
# #
# # Run Quartus-generated IP simulation script once to compile Quartus EDA
# # simulation libraries and Quartus-generated IP simulation files, and copy
# # any ROM/RAM initialization files to the simulation directory.
# # - If necessary, specify any compilation options:
# #   USER_DEFINED_COMPILE_OPTIONS
# #   USER_DEFINED_VHDL_COMPILE_OPTIONS applied to vhdl compiler
# #   USER_DEFINED_VERILOG_COMPILE_OPTIONS applied to verilog compiler
# #
# source <script generation output directory>/xcelium/xcelium_setup.sh \
# SKIP_ELAB=1 \
# SKIP_SIM=1 \
# USER_DEFINED_COMPILE_OPTIONS=<compilation options for your design> \
# USER_DEFINED_VHDL_COMPILE_OPTIONS=<VHDL compilation options for your design> \
# USER_DEFINED_VERILOG_COMPILE_OPTIONS=<Verilog compilation options for your design> \
# QSYS_SIMDIR=<script generation output directory>
# #
# # Compile all design files and testbench files, including the top level.
# # (These are all the files required for simulation other than the files
# # compiled by the IP script)
# #
# xmvlog <compilation options> <design and testbench files>
# #
# # TOP_LEVEL_NAME is used in this script to set the top-level simulation or
# # testbench module/entity name.
# #
# # Run the IP script again to elaborate and simulate the top level:
# # - Specify TOP_LEVEL_NAME and USER_DEFINED_ELAB_OPTIONS.
# # - Override the default USER_DEFINED_SIM_OPTIONS. For example, to run
# #   until $finish(), set to an empty string: USER_DEFINED_SIM_OPTIONS="".
# #
# source <script generation output directory>/xcelium/xcelium_setup.sh \
# SKIP_FILE_COPY=1 \
# SKIP_DEV_COM=1 \
# SKIP_COM=1 \
# TOP_LEVEL_NAME=<simulation top> \
# USER_DEFINED_ELAB_OPTIONS=<elaboration options for your design> \
# USER_DEFINED_SIM_OPTIONS=<simulation options for your design>
# #
# # TOP-LEVEL TEMPLATE - END
# ----------------------------------------
# 
# IP SIMULATION SCRIPT
# ----------------------------------------
# If soc_system is one of several IP cores in your
# Quartus project, you can generate a simulation script
# suitable for inclusion in your top-level simulation
# script by running the following command line:
# 
# ip-setup-simulation --quartus-project=<quartus project>
# 
# ip-setup-simulation will discover the Altera IP
# within the Quartus project, and generate a unified
# script which supports all the Altera IP within the design.
# ----------------------------------------
# ACDS 24.1 1077 linux 2026.03.14.16:26:46
# ----------------------------------------
# initialize variables
TOP_LEVEL_NAME="soc_system"
QSYS_SIMDIR="./../"
QUARTUS_INSTALL_DIR="/root/intelFPGA_lite/24.1std/quartus/"
SKIP_FILE_COPY=0
SKIP_DEV_COM=0
SKIP_COM=0
SKIP_ELAB=0
SKIP_SIM=0
USER_DEFINED_ELAB_OPTIONS=""
USER_DEFINED_SIM_OPTIONS="-input \"@run 100; exit\""

# ----------------------------------------
# overwrite variables - DO NOT MODIFY!
# This block evaluates each command line argument, typically used for 
# overwriting variables. An example usage:
#   sh <simulator>_setup.sh SKIP_SIM=1
for expression in "$@"; do
  eval $expression
  if [ $? -ne 0 ]; then
    echo "Error: This command line argument, \"$expression\", is/has an invalid expression." >&2
    exit $?
  fi
done

# ----------------------------------------
# initialize simulation properties - DO NOT MODIFY!
ELAB_OPTIONS=""
SIM_OPTIONS=""
if [[ `xmsim -version` != *"xmsim(64)"* ]]; then
  :
else
  :
fi

# ----------------------------------------
# create compilation libraries
mkdir -p ./libraries/work/
mkdir -p ./libraries/altera_common_sv_packages/
mkdir -p ./libraries/error_adapter_0/
mkdir -p ./libraries/border/
mkdir -p ./libraries/channel_adapter_0/
mkdir -p ./libraries/rsp_mux/
mkdir -p ./libraries/cmd_mux/
mkdir -p ./libraries/cmd_demux_001/
mkdir -p ./libraries/cmd_demux/
mkdir -p ./libraries/router_002/
mkdir -p ./libraries/router/
mkdir -p ./libraries/hps_0_f2h_axi_slave_agent/
mkdir -p ./libraries/DMA_neurax_write_mm_read_agent/
mkdir -p ./libraries/DMA_neurax_write_mm_read_translator/
mkdir -p ./libraries/avalon_st_adapter_003/
mkdir -p ./libraries/avalon_st_adapter/
mkdir -p ./libraries/DMA_neurax_write_descriptor_slave_rsp_width_adapter/
mkdir -p ./libraries/rsp_mux_002/
mkdir -p ./libraries/rsp_demux_001/
mkdir -p ./libraries/rsp_demux/
mkdir -p ./libraries/cmd_mux_001/
mkdir -p ./libraries/cmd_demux_002/
mkdir -p ./libraries/DMA_neurax_read_csr_burst_adapter/
mkdir -p ./libraries/hps_0_h2f_lw_axi_master_wr_limiter/
mkdir -p ./libraries/router_007/
mkdir -p ./libraries/router_005/
mkdir -p ./libraries/router_004/
mkdir -p ./libraries/DMA_neurax_read_csr_agent_rsp_fifo/
mkdir -p ./libraries/DMA_neurax_read_csr_agent/
mkdir -p ./libraries/hps_0_h2f_axi_master_agent/
mkdir -p ./libraries/DMA_neurax_read_csr_translator/
mkdir -p ./libraries/hps_io/
mkdir -p ./libraries/fpga_interfaces/
mkdir -p ./libraries/read_mstr_internal/
mkdir -p ./libraries/write_mstr_internal/
mkdir -p ./libraries/dispatcher_internal/
mkdir -p ./libraries/rst_controller/
mkdir -p ./libraries/avalon_st_adapter_001/
mkdir -p ./libraries/irq_mapper/
mkdir -p ./libraries/mm_interconnect_1/
mkdir -p ./libraries/mm_interconnect_0/
mkdir -p ./libraries/pll_0/
mkdir -p ./libraries/neurax_avalon_accelerator_0/
mkdir -p ./libraries/hps_0/
mkdir -p ./libraries/DMA_neurax_write/
mkdir -p ./libraries/DMA_neurax_read/
mkdir -p ./libraries/altera_ver/
mkdir -p ./libraries/lpm_ver/
mkdir -p ./libraries/sgate_ver/
mkdir -p ./libraries/altera_mf_ver/
mkdir -p ./libraries/cyclonev_ver/
mkdir -p ./libraries/cyclonev_hssi_ver/
mkdir -p ./libraries/cyclonev_pcie_hip_ver/
mkdir -p ./libraries/altera/
mkdir -p ./libraries/lpm/
mkdir -p ./libraries/sgate/
mkdir -p ./libraries/altera_mf/
mkdir -p ./libraries/altera_lnsim/
mkdir -p ./libraries/cyclonev/
mkdir -p ./libraries/cyclonev_hssi/

# ----------------------------------------
# copy RAM/ROM files to simulation directory

# ----------------------------------------
# compile device library files
if [ $SKIP_DEV_COM -eq 0 ]; then
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_primitives.v"                      -work altera_ver           
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/220model.v"                               -work lpm_ver              
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/sgate.v"                                  -work sgate_ver            
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_mf.v"                              -work altera_mf_ver        
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/cadence/cyclonev_hmi_atoms_ncrypt.v"      -work cyclonev_ver         
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_atoms.v"                         -work cyclonev_ver         
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_hssi_atoms.v"                    -work cyclonev_hssi_ver    
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/cadence/cyclonev_pcie_hip_atoms_ncrypt.v" -work cyclonev_pcie_hip_ver
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_pcie_hip_atoms.v"                -work cyclonev_pcie_hip_ver
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_syn_attributes.vhd"                -work altera               
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_standard_functions.vhd"            -work altera               
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/alt_dspbuilder_package.vhd"               -work altera               
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_europa_support_lib.vhd"            -work altera               
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_primitives_components.vhd"         -work altera               
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_primitives.vhd"                    -work altera               
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/220pack.vhd"                              -work lpm                  
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/220model.vhd"                             -work lpm                  
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/sgate_pack.vhd"                           -work sgate                
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/sgate.vhd"                                -work sgate                
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_mf_components.vhd"                 -work altera_mf            
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_mf.vhd"                            -work altera_mf            
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_lnsim.sv"                          -work altera_lnsim         
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_lnsim_components.vhd"              -work altera_lnsim         
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/cadence/cyclonev_atoms_ncrypt.v"          -work cyclonev             
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_atoms.vhd"                       -work cyclonev             
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_components.vhd"                  -work cyclonev             
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_hssi_components.vhd"             -work cyclonev_hssi        
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QUARTUS_INSTALL_DIR/eda/sim_lib/cadence/cyclonev_hssi_atoms_ncrypt.v"     -work cyclonev_hssi        
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_hssi_atoms.vhd"                  -work cyclonev_hssi        
fi

# ----------------------------------------
# compile design files in correct order
if [ $SKIP_COM -eq 0 ]; then
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/verbosity_pkg.sv"                                                      -work altera_common_sv_packages                           -cdslib ./cds_libs/altera_common_sv_packages.cds.lib                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/avalon_utilities_pkg.sv"                                               -work altera_common_sv_packages                           -cdslib ./cds_libs/altera_common_sv_packages.cds.lib                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/avalon_mm_pkg.sv"                                                      -work altera_common_sv_packages                           -cdslib ./cds_libs/altera_common_sv_packages.cds.lib                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_avalon_st_adapter_003_error_adapter_0.sv" -work error_adapter_0                                     -cdslib ./cds_libs/error_adapter_0.cds.lib                                    
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_avalon_st_adapter_error_adapter_0.sv"     -work error_adapter_0                                     -cdslib ./cds_libs/error_adapter_0.cds.lib                                    
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_mm_slave_bfm.sv"                                         -work border                                              -cdslib ./cds_libs/border.cds.lib                                             
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_interrupt_sink.sv"                                       -work border                                              -cdslib ./cds_libs/border.cds.lib                                             
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_clock_source.sv"                                         -work border                                              -cdslib ./cds_libs/border.cds.lib                                             
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_reset_source.sv"                                         -work border                                              -cdslib ./cds_libs/border.cds.lib                                             
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_hps_0_hps_io_border_memory.sv"                              -work border                                              -cdslib ./cds_libs/border.cds.lib                                             
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_hps_0_hps_io_border_hps_io.sv"                              -work border                                              -cdslib ./cds_libs/border.cds.lib                                             
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_hps_0_hps_io_border.sv"                                     -work border                                              -cdslib ./cds_libs/border.cds.lib                                             
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_avalon_st_adapter_001_channel_adapter_0.sv"                 -work channel_adapter_0                                   -cdslib ./cds_libs/channel_adapter_0.cds.lib                                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_avalon_st_adapter_channel_adapter_0.sv"                     -work channel_adapter_0                                   -cdslib ./cds_libs/channel_adapter_0.cds.lib                                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_rsp_mux.sv"                               -work rsp_mux                                             -cdslib ./cds_libs/rsp_mux.cds.lib                                            
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -work rsp_mux                                             -cdslib ./cds_libs/rsp_mux.cds.lib                                            
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_cmd_mux.sv"                               -work cmd_mux                                             -cdslib ./cds_libs/cmd_mux.cds.lib                                            
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -work cmd_mux                                             -cdslib ./cds_libs/cmd_mux.cds.lib                                            
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_cmd_demux_001.sv"                         -work cmd_demux_001                                       -cdslib ./cds_libs/cmd_demux_001.cds.lib                                      
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_cmd_demux.sv"                             -work cmd_demux                                           -cdslib ./cds_libs/cmd_demux.cds.lib                                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_router_002.sv"                            -work router_002                                          -cdslib ./cds_libs/router_002.cds.lib                                         
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_router.sv"                                -work router                                              -cdslib ./cds_libs/router.cds.lib                                             
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_axi_slave_ni.sv"                                         -work hps_0_f2h_axi_slave_agent                           -cdslib ./cds_libs/hps_0_f2h_axi_slave_agent.cds.lib                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_burst_uncompressor.sv"                                   -work hps_0_f2h_axi_slave_agent                           -cdslib ./cds_libs/hps_0_f2h_axi_slave_agent.cds.lib                          
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/altera_avalon_sc_fifo.v"                                               -work hps_0_f2h_axi_slave_agent                           -cdslib ./cds_libs/hps_0_f2h_axi_slave_agent.cds.lib                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_address_alignment.sv"                                    -work hps_0_f2h_axi_slave_agent                           -cdslib ./cds_libs/hps_0_f2h_axi_slave_agent.cds.lib                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_master_agent.sv"                                         -work DMA_neurax_write_mm_read_agent                      -cdslib ./cds_libs/DMA_neurax_write_mm_read_agent.cds.lib                     
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_master_translator.sv"                                    -work DMA_neurax_write_mm_read_translator                 -cdslib ./cds_libs/DMA_neurax_write_mm_read_translator.cds.lib                
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_avalon_st_adapter_003.vhd"                -work avalon_st_adapter_003                               -cdslib ./cds_libs/avalon_st_adapter_003.cds.lib                              
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_avalon_st_adapter.vhd"                    -work avalon_st_adapter                                   -cdslib ./cds_libs/avalon_st_adapter.cds.lib                                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_width_adapter.sv"                                        -work DMA_neurax_write_descriptor_slave_rsp_width_adapter -cdslib ./cds_libs/DMA_neurax_write_descriptor_slave_rsp_width_adapter.cds.lib
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_address_alignment.sv"                                    -work DMA_neurax_write_descriptor_slave_rsp_width_adapter -cdslib ./cds_libs/DMA_neurax_write_descriptor_slave_rsp_width_adapter.cds.lib
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_burst_uncompressor.sv"                                   -work DMA_neurax_write_descriptor_slave_rsp_width_adapter -cdslib ./cds_libs/DMA_neurax_write_descriptor_slave_rsp_width_adapter.cds.lib
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_rsp_mux_002.sv"                           -work rsp_mux_002                                         -cdslib ./cds_libs/rsp_mux_002.cds.lib                                        
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -work rsp_mux_002                                         -cdslib ./cds_libs/rsp_mux_002.cds.lib                                        
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_rsp_mux.sv"                               -work rsp_mux                                             -cdslib ./cds_libs/rsp_mux.cds.lib                                            
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -work rsp_mux                                             -cdslib ./cds_libs/rsp_mux.cds.lib                                            
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_rsp_demux_001.sv"                         -work rsp_demux_001                                       -cdslib ./cds_libs/rsp_demux_001.cds.lib                                      
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_rsp_demux.sv"                             -work rsp_demux                                           -cdslib ./cds_libs/rsp_demux.cds.lib                                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_cmd_mux_001.sv"                           -work cmd_mux_001                                         -cdslib ./cds_libs/cmd_mux_001.cds.lib                                        
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -work cmd_mux_001                                         -cdslib ./cds_libs/cmd_mux_001.cds.lib                                        
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_cmd_mux.sv"                               -work cmd_mux                                             -cdslib ./cds_libs/cmd_mux.cds.lib                                            
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -work cmd_mux                                             -cdslib ./cds_libs/cmd_mux.cds.lib                                            
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_cmd_demux_002.sv"                         -work cmd_demux_002                                       -cdslib ./cds_libs/cmd_demux_002.cds.lib                                      
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_cmd_demux.sv"                             -work cmd_demux                                           -cdslib ./cds_libs/cmd_demux.cds.lib                                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_burst_adapter.sv"                                        -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_burst_adapter_uncmpr.sv"                                 -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_burst_adapter_13_1.sv"                                   -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_burst_adapter_new.sv"                                    -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_incr_burst_converter.sv"                                        -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_wrap_burst_converter.sv"                                        -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_default_burst_converter.sv"                                     -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_address_alignment.sv"                                    -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_st_pipeline_stage.sv"                                    -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_st_pipeline_base.v"                                      -work DMA_neurax_read_csr_burst_adapter                   -cdslib ./cds_libs/DMA_neurax_read_csr_burst_adapter.cds.lib                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_traffic_limiter.sv"                                      -work hps_0_h2f_lw_axi_master_wr_limiter                  -cdslib ./cds_libs/hps_0_h2f_lw_axi_master_wr_limiter.cds.lib                 
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_reorder_memory.sv"                                       -work hps_0_h2f_lw_axi_master_wr_limiter                  -cdslib ./cds_libs/hps_0_h2f_lw_axi_master_wr_limiter.cds.lib                 
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_sc_fifo.v"                                               -work hps_0_h2f_lw_axi_master_wr_limiter                  -cdslib ./cds_libs/hps_0_h2f_lw_axi_master_wr_limiter.cds.lib                 
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_st_pipeline_base.v"                                      -work hps_0_h2f_lw_axi_master_wr_limiter                  -cdslib ./cds_libs/hps_0_h2f_lw_axi_master_wr_limiter.cds.lib                 
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_router_007.sv"                            -work router_007                                          -cdslib ./cds_libs/router_007.cds.lib                                         
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_router_005.sv"                            -work router_005                                          -cdslib ./cds_libs/router_005.cds.lib                                         
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_router_004.sv"                            -work router_004                                          -cdslib ./cds_libs/router_004.cds.lib                                         
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_router_002.sv"                            -work router_002                                          -cdslib ./cds_libs/router_002.cds.lib                                         
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_router.sv"                                -work router                                              -cdslib ./cds_libs/router.cds.lib                                             
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/altera_avalon_sc_fifo.v"                                               -work DMA_neurax_read_csr_agent_rsp_fifo                  -cdslib ./cds_libs/DMA_neurax_read_csr_agent_rsp_fifo.cds.lib                 
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_slave_agent.sv"                                          -work DMA_neurax_read_csr_agent                           -cdslib ./cds_libs/DMA_neurax_read_csr_agent.cds.lib                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_burst_uncompressor.sv"                                   -work DMA_neurax_read_csr_agent                           -cdslib ./cds_libs/DMA_neurax_read_csr_agent.cds.lib                          
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_axi_master_ni.sv"                                        -work hps_0_h2f_axi_master_agent                          -cdslib ./cds_libs/hps_0_h2f_axi_master_agent.cds.lib                         
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_address_alignment.sv"                                    -work hps_0_h2f_axi_master_agent                          -cdslib ./cds_libs/hps_0_h2f_axi_master_agent.cds.lib                         
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_merlin_slave_translator.sv"                                     -work DMA_neurax_read_csr_translator                      -cdslib ./cds_libs/DMA_neurax_read_csr_translator.cds.lib                     
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/soc_system_hps_0_hps_io.v"                                             -work hps_io                                              -cdslib ./cds_libs/hps_io.cds.lib                                             
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_mm_slave_bfm.sv"                                         -work fpga_interfaces                                     -cdslib ./cds_libs/fpga_interfaces.cds.lib                                    
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_interrupt_sink.sv"                                       -work fpga_interfaces                                     -cdslib ./cds_libs/fpga_interfaces.cds.lib                                    
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_clock_source.sv"                                         -work fpga_interfaces                                     -cdslib ./cds_libs/fpga_interfaces.cds.lib                                    
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_reset_source.sv"                                         -work fpga_interfaces                                     -cdslib ./cds_libs/fpga_interfaces.cds.lib                                    
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_hps_0_fpga_interfaces.sv"                                   -work fpga_interfaces                                     -cdslib ./cds_libs/fpga_interfaces.cds.lib                                    
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/read_master.v"                                                         -work read_mstr_internal                                  -cdslib ./cds_libs/read_mstr_internal.cds.lib                                 
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/MM_to_ST_Adapter.v"                                                    -work read_mstr_internal                                  -cdslib ./cds_libs/read_mstr_internal.cds.lib                                 
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/read_burst_control.v"                                                  -work read_mstr_internal                                  -cdslib ./cds_libs/read_mstr_internal.cds.lib                                 
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/write_master.v"                                                        -work write_mstr_internal                                 -cdslib ./cds_libs/write_mstr_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/byte_enable_generator.v"                                               -work write_mstr_internal                                 -cdslib ./cds_libs/write_mstr_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/ST_to_MM_Adapter.v"                                                    -work write_mstr_internal                                 -cdslib ./cds_libs/write_mstr_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/write_burst_control.v"                                                 -work write_mstr_internal                                 -cdslib ./cds_libs/write_mstr_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/dispatcher.v"                                                          -work dispatcher_internal                                 -cdslib ./cds_libs/dispatcher_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/descriptor_buffers.v"                                                  -work dispatcher_internal                                 -cdslib ./cds_libs/dispatcher_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/csr_block.v"                                                           -work dispatcher_internal                                 -cdslib ./cds_libs/dispatcher_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/response_block.v"                                                      -work dispatcher_internal                                 -cdslib ./cds_libs/dispatcher_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/fifo_with_byteenables.v"                                               -work dispatcher_internal                                 -cdslib ./cds_libs/dispatcher_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/read_signal_breakout.v"                                                -work dispatcher_internal                                 -cdslib ./cds_libs/dispatcher_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/write_signal_breakout.v"                                               -work dispatcher_internal                                 -cdslib ./cds_libs/dispatcher_internal.cds.lib                                
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/altera_reset_controller.v"                                             -work rst_controller                                      -cdslib ./cds_libs/rst_controller.cds.lib                                     
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/altera_reset_synchronizer.v"                                           -work rst_controller                                      -cdslib ./cds_libs/rst_controller.cds.lib                                     
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/soc_system_avalon_st_adapter_001.vhd"                                  -work avalon_st_adapter_001                               -cdslib ./cds_libs/avalon_st_adapter_001.cds.lib                              
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/soc_system_avalon_st_adapter.vhd"                                      -work avalon_st_adapter                                   -cdslib ./cds_libs/avalon_st_adapter.cds.lib                                  
  xmvlog -sv $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_irq_mapper.sv"                                              -work irq_mapper                                          -cdslib ./cds_libs/irq_mapper.cds.lib                                         
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1.v"                                        -work mm_interconnect_1                                   -cdslib ./cds_libs/mm_interconnect_1.cds.lib                                  
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0.v"                                        -work mm_interconnect_0                                   -cdslib ./cds_libs/mm_interconnect_0.cds.lib                                  
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/soc_system_pll_0.vho"                                                  -work pll_0                                               -cdslib ./cds_libs/pll_0.cds.lib                                              
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/neurax.vhd"                                                            -work neurax_avalon_accelerator_0                         -cdslib ./cds_libs/neurax_avalon_accelerator_0.cds.lib                        
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/FPGA_accelerator.vhd"                                                  -work neurax_avalon_accelerator_0                         -cdslib ./cds_libs/neurax_avalon_accelerator_0.cds.lib                        
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/activation_block.vhd"                                                  -work neurax_avalon_accelerator_0                         -cdslib ./cds_libs/neurax_avalon_accelerator_0.cds.lib                        
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/convolution_block.vhd"                                                 -work neurax_avalon_accelerator_0                         -cdslib ./cds_libs/neurax_avalon_accelerator_0.cds.lib                        
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/neurax_accel_types_pkg.vhd"                                            -work neurax_avalon_accelerator_0                         -cdslib ./cds_libs/neurax_avalon_accelerator_0.cds.lib                        
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/neurax_data_interface.vhd"                                             -work neurax_avalon_accelerator_0                         -cdslib ./cds_libs/neurax_avalon_accelerator_0.cds.lib                        
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/neurax_register_block.vhd"                                             -work neurax_avalon_accelerator_0                         -cdslib ./cds_libs/neurax_avalon_accelerator_0.cds.lib                        
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/neurax_register_pkg.vhd"                                               -work neurax_avalon_accelerator_0                         -cdslib ./cds_libs/neurax_avalon_accelerator_0.cds.lib                        
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/pooling_block.vhd"                                                     -work neurax_avalon_accelerator_0                         -cdslib ./cds_libs/neurax_avalon_accelerator_0.cds.lib                        
  xmvlog $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS     "$QSYS_SIMDIR/submodules/soc_system_hps_0.v"                                                    -work hps_0                                               -cdslib ./cds_libs/hps_0.cds.lib                                              
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/soc_system_DMA_neurax_write.vhd"                                       -work DMA_neurax_write                                    -cdslib ./cds_libs/DMA_neurax_write.cds.lib                                   
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/submodules/soc_system_DMA_neurax_read.vhd"                                        -work DMA_neurax_read                                     -cdslib ./cds_libs/DMA_neurax_read.cds.lib                                    
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/soc_system.vhd"                                                                                                                                                                                                           
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/soc_system_rst_controller.vhd"                                                                                                                                                                                            
  xmvhdl -v93 $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS   "$QSYS_SIMDIR/soc_system_rst_controller_001.vhd"                                                                                                                                                                                        
fi

# ----------------------------------------
# elaborate top level design
if [ $SKIP_ELAB -eq 0 ]; then
  xmelab -update -access +w+r+c -namemap_mixgen +DISABLEGENCHK -relax $ELAB_OPTIONS $USER_DEFINED_ELAB_OPTIONS $TOP_LEVEL_NAME
fi

# ----------------------------------------
# simulate
if [ $SKIP_SIM -eq 0 ]; then
  eval xmsim -licqueue $SIM_OPTIONS $USER_DEFINED_SIM_OPTIONS $TOP_LEVEL_NAME
fi
