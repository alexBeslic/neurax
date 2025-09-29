
# (C) 2001-2025 Altera Corporation. All rights reserved.
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

# ACDS 24.1 1077 win32 2025.09.27.19:39:34
# ----------------------------------------
# Auto-generated simulation script rivierapro_setup.tcl
# ----------------------------------------
# This script provides commands to simulate the following IP detected in
# your Quartus project:
#     soc_system
# 
# Altera recommends that you source this Quartus-generated IP simulation
# script from your own customized top-level script, and avoid editing this
# generated script.
# 
# To write a top-level script that compiles Altera simulation libraries and
# the Quartus-generated IP in your project, along with your design and
# testbench files, copy the text from the TOP-LEVEL TEMPLATE section below
# into a new file, e.g. named "aldec.do", and modify the text as directed.
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
# # the simulator.
# #
# set QSYS_SIMDIR <script generation output directory>
# #
# # Source the generated IP simulation script.
# source $QSYS_SIMDIR/aldec/rivierapro_setup.tcl
# #
# # Set any compilation options you require (this is unusual).
# set USER_DEFINED_COMPILE_OPTIONS <compilation options>
# set USER_DEFINED_VHDL_COMPILE_OPTIONS <compilation options for VHDL>
# set USER_DEFINED_VERILOG_COMPILE_OPTIONS <compilation options for Verilog>
# #
# # Call command to compile the Quartus EDA simulation library.
# dev_com
# #
# # Call command to compile the Quartus-generated IP simulation files.
# com
# #
# # Add commands to compile all design files and testbench files, including
# # the top level. (These are all the files required for simulation other
# # than the files compiled by the Quartus-generated IP simulation script)
# #
# vlog -sv2k5 <your compilation options> <design and testbench files>
# #
# # Set the top-level simulation or testbench module/entity name, which is
# # used by the elab command to elaborate the top level.
# #
# set TOP_LEVEL_NAME <simulation top>
# #
# # Set any elaboration options you require.
# set USER_DEFINED_ELAB_OPTIONS <elaboration options>
# #
# # Call command to elaborate your design and testbench.
# elab
# #
# # Run the simulation.
# run
# #
# # Report success to the shell.
# exit -code 0
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

# ----------------------------------------
# Initialize variables
if ![info exists SYSTEM_INSTANCE_NAME] { 
  set SYSTEM_INSTANCE_NAME ""
} elseif { ![ string match "" $SYSTEM_INSTANCE_NAME ] } { 
  set SYSTEM_INSTANCE_NAME "/$SYSTEM_INSTANCE_NAME"
}

if ![info exists TOP_LEVEL_NAME] { 
  set TOP_LEVEL_NAME "soc_system"
}

if ![info exists QSYS_SIMDIR] { 
  set QSYS_SIMDIR "./../"
}

if ![info exists QUARTUS_INSTALL_DIR] { 
  set QUARTUS_INSTALL_DIR "C:/intelfpga_lite/24.1std/quartus/"
}

if ![info exists USER_DEFINED_COMPILE_OPTIONS] { 
  set USER_DEFINED_COMPILE_OPTIONS ""
}
if ![info exists USER_DEFINED_VHDL_COMPILE_OPTIONS] { 
  set USER_DEFINED_VHDL_COMPILE_OPTIONS ""
}
if ![info exists USER_DEFINED_VERILOG_COMPILE_OPTIONS] { 
  set USER_DEFINED_VERILOG_COMPILE_OPTIONS ""
}
if ![info exists USER_DEFINED_ELAB_OPTIONS] { 
  set USER_DEFINED_ELAB_OPTIONS ""
}

# ----------------------------------------
# Initialize simulation properties - DO NOT MODIFY!
set ELAB_OPTIONS ""
set SIM_OPTIONS ""
if ![ string match "*-64 vsim*" [ vsim -version ] ] {
} else {
}

set Aldec "Riviera"
if { [ string match "*Active-HDL*" [ vsim -version ] ] } {
  set Aldec "Active"
}

if { [ string match "Active" $Aldec ] } {
  scripterconf -tcl
  createdesign "$TOP_LEVEL_NAME"  "."
  opendesign "$TOP_LEVEL_NAME"
}

# ----------------------------------------
# Copy ROM/RAM files to simulation directory
alias file_copy {
  echo "\[exec\] file_copy"
}

# ----------------------------------------
# Create compilation libraries
proc ensure_lib { lib } { if ![file isdirectory $lib] { vlib $lib } }
ensure_lib      ./libraries     
ensure_lib      ./libraries/work
vmap       work ./libraries/work
ensure_lib                       ./libraries/altera_ver           
vmap       altera_ver            ./libraries/altera_ver           
ensure_lib                       ./libraries/lpm_ver              
vmap       lpm_ver               ./libraries/lpm_ver              
ensure_lib                       ./libraries/sgate_ver            
vmap       sgate_ver             ./libraries/sgate_ver            
ensure_lib                       ./libraries/altera_mf_ver        
vmap       altera_mf_ver         ./libraries/altera_mf_ver        
ensure_lib                       ./libraries/altera_lnsim_ver     
vmap       altera_lnsim_ver      ./libraries/altera_lnsim_ver     
ensure_lib                       ./libraries/cyclonev_ver         
vmap       cyclonev_ver          ./libraries/cyclonev_ver         
ensure_lib                       ./libraries/cyclonev_hssi_ver    
vmap       cyclonev_hssi_ver     ./libraries/cyclonev_hssi_ver    
ensure_lib                       ./libraries/cyclonev_pcie_hip_ver
vmap       cyclonev_pcie_hip_ver ./libraries/cyclonev_pcie_hip_ver
ensure_lib                       ./libraries/altera               
vmap       altera                ./libraries/altera               
ensure_lib                       ./libraries/lpm                  
vmap       lpm                   ./libraries/lpm                  
ensure_lib                       ./libraries/sgate                
vmap       sgate                 ./libraries/sgate                
ensure_lib                       ./libraries/altera_mf            
vmap       altera_mf             ./libraries/altera_mf            
ensure_lib                       ./libraries/altera_lnsim         
vmap       altera_lnsim          ./libraries/altera_lnsim         
ensure_lib                       ./libraries/cyclonev             
vmap       cyclonev              ./libraries/cyclonev             
ensure_lib                       ./libraries/cyclonev_hssi        
vmap       cyclonev_hssi         ./libraries/cyclonev_hssi        
ensure_lib                                                           ./libraries/altera_common_sv_packages                                
vmap       altera_common_sv_packages                                 ./libraries/altera_common_sv_packages                                
ensure_lib                                                           ./libraries/error_adapter_0                                          
vmap       error_adapter_0                                           ./libraries/error_adapter_0                                          
ensure_lib                                                           ./libraries/border                                                   
vmap       border                                                    ./libraries/border                                                   
ensure_lib                                                           ./libraries/channel_adapter_0                                        
vmap       channel_adapter_0                                         ./libraries/channel_adapter_0                                        
ensure_lib                                                           ./libraries/rsp_mux                                                  
vmap       rsp_mux                                                   ./libraries/rsp_mux                                                  
ensure_lib                                                           ./libraries/cmd_mux                                                  
vmap       cmd_mux                                                   ./libraries/cmd_mux                                                  
ensure_lib                                                           ./libraries/cmd_demux_001                                            
vmap       cmd_demux_001                                             ./libraries/cmd_demux_001                                            
ensure_lib                                                           ./libraries/cmd_demux                                                
vmap       cmd_demux                                                 ./libraries/cmd_demux                                                
ensure_lib                                                           ./libraries/router_002                                               
vmap       router_002                                                ./libraries/router_002                                               
ensure_lib                                                           ./libraries/router                                                   
vmap       router                                                    ./libraries/router                                                   
ensure_lib                                                           ./libraries/hps_0_f2h_axi_slave_agent                                
vmap       hps_0_f2h_axi_slave_agent                                 ./libraries/hps_0_f2h_axi_slave_agent                                
ensure_lib                                                           ./libraries/DMA_neurax_write_mm_read_agent                           
vmap       DMA_neurax_write_mm_read_agent                            ./libraries/DMA_neurax_write_mm_read_agent                           
ensure_lib                                                           ./libraries/DMA_neurax_write_mm_read_translator                      
vmap       DMA_neurax_write_mm_read_translator                       ./libraries/DMA_neurax_write_mm_read_translator                      
ensure_lib                                                           ./libraries/avalon_st_adapter_003                                    
vmap       avalon_st_adapter_003                                     ./libraries/avalon_st_adapter_003                                    
ensure_lib                                                           ./libraries/avalon_st_adapter                                        
vmap       avalon_st_adapter                                         ./libraries/avalon_st_adapter                                        
ensure_lib                                                           ./libraries/DMA_neurax_write_descriptor_slave_rsp_width_adapter      
vmap       DMA_neurax_write_descriptor_slave_rsp_width_adapter       ./libraries/DMA_neurax_write_descriptor_slave_rsp_width_adapter      
ensure_lib                                                           ./libraries/rsp_demux                                                
vmap       rsp_demux                                                 ./libraries/rsp_demux                                                
ensure_lib                                                           ./libraries/neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
vmap       neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter  ./libraries/neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
ensure_lib                                                           ./libraries/hps_0_h2f_axi_master_wr_limiter                          
vmap       hps_0_h2f_axi_master_wr_limiter                           ./libraries/hps_0_h2f_axi_master_wr_limiter                          
ensure_lib                                                           ./libraries/router_005                                               
vmap       router_005                                                ./libraries/router_005                                               
ensure_lib                                                           ./libraries/neurax_avalon_accelerator_0_avalon_slave_0_agent_rsp_fifo
vmap       neurax_avalon_accelerator_0_avalon_slave_0_agent_rsp_fifo ./libraries/neurax_avalon_accelerator_0_avalon_slave_0_agent_rsp_fifo
ensure_lib                                                           ./libraries/neurax_avalon_accelerator_0_avalon_slave_0_agent         
vmap       neurax_avalon_accelerator_0_avalon_slave_0_agent          ./libraries/neurax_avalon_accelerator_0_avalon_slave_0_agent         
ensure_lib                                                           ./libraries/hps_0_h2f_axi_master_agent                               
vmap       hps_0_h2f_axi_master_agent                                ./libraries/hps_0_h2f_axi_master_agent                               
ensure_lib                                                           ./libraries/neurax_avalon_accelerator_0_avalon_slave_0_translator    
vmap       neurax_avalon_accelerator_0_avalon_slave_0_translator     ./libraries/neurax_avalon_accelerator_0_avalon_slave_0_translator    
ensure_lib                                                           ./libraries/hps_io                                                   
vmap       hps_io                                                    ./libraries/hps_io                                                   
ensure_lib                                                           ./libraries/fpga_interfaces                                          
vmap       fpga_interfaces                                           ./libraries/fpga_interfaces                                          
ensure_lib                                                           ./libraries/read_mstr_internal                                       
vmap       read_mstr_internal                                        ./libraries/read_mstr_internal                                       
ensure_lib                                                           ./libraries/write_mstr_internal                                      
vmap       write_mstr_internal                                       ./libraries/write_mstr_internal                                      
ensure_lib                                                           ./libraries/dispatcher_internal                                      
vmap       dispatcher_internal                                       ./libraries/dispatcher_internal                                      
ensure_lib                                                           ./libraries/rst_controller                                           
vmap       rst_controller                                            ./libraries/rst_controller                                           
ensure_lib                                                           ./libraries/avalon_st_adapter_001                                    
vmap       avalon_st_adapter_001                                     ./libraries/avalon_st_adapter_001                                    
ensure_lib                                                           ./libraries/irq_mapper                                               
vmap       irq_mapper                                                ./libraries/irq_mapper                                               
ensure_lib                                                           ./libraries/mm_interconnect_1                                        
vmap       mm_interconnect_1                                         ./libraries/mm_interconnect_1                                        
ensure_lib                                                           ./libraries/mm_interconnect_0                                        
vmap       mm_interconnect_0                                         ./libraries/mm_interconnect_0                                        
ensure_lib                                                           ./libraries/pll_0                                                    
vmap       pll_0                                                     ./libraries/pll_0                                                    
ensure_lib                                                           ./libraries/neurax_avalon_accelerator_0                              
vmap       neurax_avalon_accelerator_0                               ./libraries/neurax_avalon_accelerator_0                              
ensure_lib                                                           ./libraries/hps_0                                                    
vmap       hps_0                                                     ./libraries/hps_0                                                    
ensure_lib                                                           ./libraries/DMA_neurax_write                                         
vmap       DMA_neurax_write                                          ./libraries/DMA_neurax_write                                         
ensure_lib                                                           ./libraries/DMA_neurax_read                                          
vmap       DMA_neurax_read                                           ./libraries/DMA_neurax_read                                          

# ----------------------------------------
# Compile device library files
alias dev_com {
  echo "\[exec\] dev_com"
  eval vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_primitives.v"                    -work altera_ver           
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/220model.v"                             -work lpm_ver              
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/sgate.v"                                -work sgate_ver            
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_mf.v"                            -work altera_mf_ver        
  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_lnsim.sv"                        -work altera_lnsim_ver     
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/aldec/cyclonev_atoms_ncrypt.v"          -work cyclonev_ver         
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/aldec/cyclonev_hmi_atoms_ncrypt.v"      -work cyclonev_ver         
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_atoms.v"                       -work cyclonev_ver         
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/aldec/cyclonev_hssi_atoms_ncrypt.v"     -work cyclonev_hssi_ver    
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_hssi_atoms.v"                  -work cyclonev_hssi_ver    
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/aldec/cyclonev_pcie_hip_atoms_ncrypt.v" -work cyclonev_pcie_hip_ver
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_pcie_hip_atoms.v"              -work cyclonev_pcie_hip_ver
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_syn_attributes.vhd"              -work altera               
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_standard_functions.vhd"          -work altera               
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/alt_dspbuilder_package.vhd"             -work altera               
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_europa_support_lib.vhd"          -work altera               
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_primitives_components.vhd"       -work altera               
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_primitives.vhd"                  -work altera               
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/220pack.vhd"                            -work lpm                  
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/220model.vhd"                           -work lpm                  
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/sgate_pack.vhd"                         -work sgate                
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/sgate.vhd"                              -work sgate                
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_mf_components.vhd"               -work altera_mf            
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_mf.vhd"                          -work altera_mf            
  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_lnsim.sv"                        -work altera_lnsim         
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/altera_lnsim_components.vhd"            -work altera_lnsim         
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/aldec/cyclonev_atoms_ncrypt.v"          -work cyclonev             
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_atoms.vhd"                     -work cyclonev             
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_components.vhd"                -work cyclonev             
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_hssi_components.vhd"           -work cyclonev_hssi        
  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QUARTUS_INSTALL_DIR/eda/sim_lib/aldec/cyclonev_hssi_atoms_ncrypt.v"     -work cyclonev_hssi        
  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QUARTUS_INSTALL_DIR/eda/sim_lib/cyclonev_hssi_atoms.vhd"                -work cyclonev_hssi        
}

# ----------------------------------------
# Compile the design files in correct order
alias com {
  echo "\[exec\] com"
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/verbosity_pkg.sv"                                                                                   -work altera_common_sv_packages                                
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/avalon_utilities_pkg.sv"                                                                            -work altera_common_sv_packages                                
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/avalon_mm_pkg.sv"                                                                                   -work altera_common_sv_packages                                
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_avalon_st_adapter_003_error_adapter_0.sv" -l altera_common_sv_packages -work error_adapter_0                                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_avalon_st_adapter_error_adapter_0.sv"     -l altera_common_sv_packages -work error_adapter_0                                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_mm_slave_bfm.sv"                                         -l altera_common_sv_packages -work border                                                   
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_interrupt_sink.sv"                                       -l altera_common_sv_packages -work border                                                   
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_clock_source.sv"                                         -l altera_common_sv_packages -work border                                                   
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_reset_source.sv"                                         -l altera_common_sv_packages -work border                                                   
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_hps_0_hps_io_border_memory.sv"                              -l altera_common_sv_packages -work border                                                   
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_hps_0_hps_io_border_hps_io.sv"                              -l altera_common_sv_packages -work border                                                   
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_hps_0_hps_io_border.sv"                                     -l altera_common_sv_packages -work border                                                   
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_avalon_st_adapter_001_channel_adapter_0.sv"                 -l altera_common_sv_packages -work channel_adapter_0                                        
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_avalon_st_adapter_channel_adapter_0.sv"                     -l altera_common_sv_packages -work channel_adapter_0                                        
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_rsp_mux.sv"                               -l altera_common_sv_packages -work rsp_mux                                                  
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -l altera_common_sv_packages -work rsp_mux                                                  
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_cmd_mux.sv"                               -l altera_common_sv_packages -work cmd_mux                                                  
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -l altera_common_sv_packages -work cmd_mux                                                  
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_cmd_demux_001.sv"                         -l altera_common_sv_packages -work cmd_demux_001                                            
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_cmd_demux.sv"                             -l altera_common_sv_packages -work cmd_demux                                                
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_router_002.sv"                            -l altera_common_sv_packages -work router_002                                               
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1_router.sv"                                -l altera_common_sv_packages -work router                                                   
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_axi_slave_ni.sv"                                         -l altera_common_sv_packages -work hps_0_f2h_axi_slave_agent                                
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_burst_uncompressor.sv"                                   -l altera_common_sv_packages -work hps_0_f2h_axi_slave_agent                                
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_sc_fifo.v"                                                                            -work hps_0_f2h_axi_slave_agent                                
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_address_alignment.sv"                                    -l altera_common_sv_packages -work hps_0_f2h_axi_slave_agent                                
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_master_agent.sv"                                         -l altera_common_sv_packages -work DMA_neurax_write_mm_read_agent                           
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_master_translator.sv"                                    -l altera_common_sv_packages -work DMA_neurax_write_mm_read_translator                      
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_avalon_st_adapter_003.vhd"                                             -work avalon_st_adapter_003                                    
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_avalon_st_adapter.vhd"                                                 -work avalon_st_adapter                                        
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_width_adapter.sv"                                        -l altera_common_sv_packages -work DMA_neurax_write_descriptor_slave_rsp_width_adapter      
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_address_alignment.sv"                                    -l altera_common_sv_packages -work DMA_neurax_write_descriptor_slave_rsp_width_adapter      
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_burst_uncompressor.sv"                                   -l altera_common_sv_packages -work DMA_neurax_write_descriptor_slave_rsp_width_adapter      
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_rsp_mux.sv"                               -l altera_common_sv_packages -work rsp_mux                                                  
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -l altera_common_sv_packages -work rsp_mux                                                  
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_rsp_demux.sv"                             -l altera_common_sv_packages -work rsp_demux                                                
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_cmd_mux.sv"                               -l altera_common_sv_packages -work cmd_mux                                                  
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_arbitrator.sv"                                           -l altera_common_sv_packages -work cmd_mux                                                  
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_cmd_demux.sv"                             -l altera_common_sv_packages -work cmd_demux                                                
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_burst_adapter.sv"                                        -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_burst_adapter_uncmpr.sv"                                 -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_burst_adapter_13_1.sv"                                   -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_burst_adapter_new.sv"                                    -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_incr_burst_converter.sv"                                        -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_wrap_burst_converter.sv"                                        -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_default_burst_converter.sv"                                     -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_address_alignment.sv"                                    -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_st_pipeline_stage.sv"                                    -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_st_pipeline_base.v"                                      -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter 
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_traffic_limiter.sv"                                      -l altera_common_sv_packages -work hps_0_h2f_axi_master_wr_limiter                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_reorder_memory.sv"                                       -l altera_common_sv_packages -work hps_0_h2f_axi_master_wr_limiter                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_sc_fifo.v"                                               -l altera_common_sv_packages -work hps_0_h2f_axi_master_wr_limiter                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_st_pipeline_base.v"                                      -l altera_common_sv_packages -work hps_0_h2f_axi_master_wr_limiter                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_router_005.sv"                            -l altera_common_sv_packages -work router_005                                               
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_router_002.sv"                            -l altera_common_sv_packages -work router_002                                               
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0_router.sv"                                -l altera_common_sv_packages -work router                                                   
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_avalon_sc_fifo.v"                                                                            -work neurax_avalon_accelerator_0_avalon_slave_0_agent_rsp_fifo
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_slave_agent.sv"                                          -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_agent         
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_burst_uncompressor.sv"                                   -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_agent         
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_axi_master_ni.sv"                                        -l altera_common_sv_packages -work hps_0_h2f_axi_master_agent                               
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_address_alignment.sv"                                    -l altera_common_sv_packages -work hps_0_h2f_axi_master_agent                               
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_merlin_slave_translator.sv"                                     -l altera_common_sv_packages -work neurax_avalon_accelerator_0_avalon_slave_0_translator    
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_hps_0_hps_io.v"                                                                          -work hps_io                                                   
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_mm_slave_bfm.sv"                                         -l altera_common_sv_packages -work fpga_interfaces                                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_interrupt_sink.sv"                                       -l altera_common_sv_packages -work fpga_interfaces                                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_clock_source.sv"                                         -l altera_common_sv_packages -work fpga_interfaces                                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/altera_avalon_reset_source.sv"                                         -l altera_common_sv_packages -work fpga_interfaces                                          
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_hps_0_fpga_interfaces.sv"                                   -l altera_common_sv_packages -work fpga_interfaces                                          
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/read_master.v"                                                                                      -work read_mstr_internal                                       
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/MM_to_ST_Adapter.v"                                                                                 -work read_mstr_internal                                       
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/read_burst_control.v"                                                                               -work read_mstr_internal                                       
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/write_master.v"                                                                                     -work write_mstr_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/byte_enable_generator.v"                                                                            -work write_mstr_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/ST_to_MM_Adapter.v"                                                                                 -work write_mstr_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/write_burst_control.v"                                                                              -work write_mstr_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/dispatcher.v"                                                                                       -work dispatcher_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/descriptor_buffers.v"                                                                               -work dispatcher_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/csr_block.v"                                                                                        -work dispatcher_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/response_block.v"                                                                                   -work dispatcher_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/fifo_with_byteenables.v"                                                                            -work dispatcher_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/read_signal_breakout.v"                                                                             -work dispatcher_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/write_signal_breakout.v"                                                                            -work dispatcher_internal                                      
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_reset_controller.v"                                                                          -work rst_controller                                           
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/altera_reset_synchronizer.v"                                                                        -work rst_controller                                           
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/soc_system_avalon_st_adapter_001.vhd"                                                               -work avalon_st_adapter_001                                    
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/soc_system_avalon_st_adapter.vhd"                                                                   -work avalon_st_adapter                                        
  eval  vlog  $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS      "$QSYS_SIMDIR/submodules/soc_system_irq_mapper.sv"                                              -l altera_common_sv_packages -work irq_mapper                                               
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_1.v"                                                                     -work mm_interconnect_1                                        
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_mm_interconnect_0.v"                                                                     -work mm_interconnect_0                                        
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/soc_system_pll_0.vho"                                                                               -work pll_0                                                    
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/neurax.vhd"                                                                                         -work neurax_avalon_accelerator_0                              
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/FPGA_accelerator.vhd"                                                                               -work neurax_avalon_accelerator_0                              
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/activation_block.vhd"                                                                               -work neurax_avalon_accelerator_0                              
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/convolution_block.vhd"                                                                              -work neurax_avalon_accelerator_0                              
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/neurax_accel_types_pkg.vhd"                                                                         -work neurax_avalon_accelerator_0                              
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/neurax_data_interface.vhd"                                                                          -work neurax_avalon_accelerator_0                              
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/neurax_register_block.vhd"                                                                          -work neurax_avalon_accelerator_0                              
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/neurax_register_pkg.vhd"                                                                            -work neurax_avalon_accelerator_0                              
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/pooling_block.vhd"                                                                                  -work neurax_avalon_accelerator_0                              
  eval  vlog -v2k5 $USER_DEFINED_VERILOG_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS "$QSYS_SIMDIR/submodules/soc_system_hps_0.v"                                                                                 -work hps_0                                                    
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/soc_system_DMA_neurax_write.vhd"                                                                    -work DMA_neurax_write                                         
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/submodules/soc_system_DMA_neurax_read.vhd"                                                                     -work DMA_neurax_read                                          
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/soc_system.vhd"                                                                                                                                                               
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/soc_system_rst_controller.vhd"                                                                                                                                                
  eval  vcom $USER_DEFINED_VHDL_COMPILE_OPTIONS $USER_DEFINED_COMPILE_OPTIONS          "$QSYS_SIMDIR/soc_system_rst_controller_001.vhd"                                                                                                                                            
}

# ----------------------------------------
# Elaborate top level design
alias elab {
  echo "\[exec\] elab"
  eval vsim +access +r -t ps $ELAB_OPTIONS -L work -L altera_common_sv_packages -L error_adapter_0 -L border -L channel_adapter_0 -L rsp_mux -L cmd_mux -L cmd_demux_001 -L cmd_demux -L router_002 -L router -L hps_0_f2h_axi_slave_agent -L DMA_neurax_write_mm_read_agent -L DMA_neurax_write_mm_read_translator -L avalon_st_adapter_003 -L avalon_st_adapter -L DMA_neurax_write_descriptor_slave_rsp_width_adapter -L rsp_demux -L neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter -L hps_0_h2f_axi_master_wr_limiter -L router_005 -L neurax_avalon_accelerator_0_avalon_slave_0_agent_rsp_fifo -L neurax_avalon_accelerator_0_avalon_slave_0_agent -L hps_0_h2f_axi_master_agent -L neurax_avalon_accelerator_0_avalon_slave_0_translator -L hps_io -L fpga_interfaces -L read_mstr_internal -L write_mstr_internal -L dispatcher_internal -L rst_controller -L avalon_st_adapter_001 -L irq_mapper -L mm_interconnect_1 -L mm_interconnect_0 -L pll_0 -L neurax_avalon_accelerator_0 -L hps_0 -L DMA_neurax_write -L DMA_neurax_read -L altera_ver -L lpm_ver -L sgate_ver -L altera_mf_ver -L altera_lnsim_ver -L cyclonev_ver -L cyclonev_hssi_ver -L cyclonev_pcie_hip_ver -L altera -L lpm -L sgate -L altera_mf -L altera_lnsim -L cyclonev -L cyclonev_hssi $TOP_LEVEL_NAME
}

# ----------------------------------------
# Elaborate the top level design with -dbg -O2 option
alias elab_debug {
  echo "\[exec\] elab_debug"
  eval vsim -dbg -O2 +access +r -t ps $ELAB_OPTIONS -L work -L altera_common_sv_packages -L error_adapter_0 -L border -L channel_adapter_0 -L rsp_mux -L cmd_mux -L cmd_demux_001 -L cmd_demux -L router_002 -L router -L hps_0_f2h_axi_slave_agent -L DMA_neurax_write_mm_read_agent -L DMA_neurax_write_mm_read_translator -L avalon_st_adapter_003 -L avalon_st_adapter -L DMA_neurax_write_descriptor_slave_rsp_width_adapter -L rsp_demux -L neurax_avalon_accelerator_0_avalon_slave_0_burst_adapter -L hps_0_h2f_axi_master_wr_limiter -L router_005 -L neurax_avalon_accelerator_0_avalon_slave_0_agent_rsp_fifo -L neurax_avalon_accelerator_0_avalon_slave_0_agent -L hps_0_h2f_axi_master_agent -L neurax_avalon_accelerator_0_avalon_slave_0_translator -L hps_io -L fpga_interfaces -L read_mstr_internal -L write_mstr_internal -L dispatcher_internal -L rst_controller -L avalon_st_adapter_001 -L irq_mapper -L mm_interconnect_1 -L mm_interconnect_0 -L pll_0 -L neurax_avalon_accelerator_0 -L hps_0 -L DMA_neurax_write -L DMA_neurax_read -L altera_ver -L lpm_ver -L sgate_ver -L altera_mf_ver -L altera_lnsim_ver -L cyclonev_ver -L cyclonev_hssi_ver -L cyclonev_pcie_hip_ver -L altera -L lpm -L sgate -L altera_mf -L altera_lnsim -L cyclonev -L cyclonev_hssi $TOP_LEVEL_NAME
}

# ----------------------------------------
# Compile all the design files and elaborate the top level design
alias ld "
  dev_com
  com
  elab
"

# ----------------------------------------
# Compile all the design files and elaborate the top level design with -dbg -O2
alias ld_debug "
  dev_com
  com
  elab_debug
"

# ----------------------------------------
# Print out user commmand line aliases
alias h {
  echo "List Of Command Line Aliases"
  echo
  echo "file_copy                                         -- Copy ROM/RAM files to simulation directory"
  echo
  echo "dev_com                                           -- Compile device library files"
  echo
  echo "com                                               -- Compile the design files in correct order"
  echo
  echo "elab                                              -- Elaborate top level design"
  echo
  echo "elab_debug                                        -- Elaborate the top level design with -dbg -O2 option"
  echo
  echo "ld                                                -- Compile all the design files and elaborate the top level design"
  echo
  echo "ld_debug                                          -- Compile all the design files and elaborate the top level design with -dbg -O2"
  echo
  echo 
  echo
  echo "List Of Variables"
  echo
  echo "TOP_LEVEL_NAME                                    -- Top level module name."
  echo "                                                     For most designs, this should be overridden"
  echo "                                                     to enable the elab/elab_debug aliases."
  echo
  echo "SYSTEM_INSTANCE_NAME                              -- Instantiated system module name inside top level module."
  echo
  echo "QSYS_SIMDIR                                       -- Platform Designer base simulation directory."
  echo
  echo "QUARTUS_INSTALL_DIR                               -- Quartus installation directory."
  echo
  echo "USER_DEFINED_COMPILE_OPTIONS                      -- User-defined compile options, added to com/dev_com aliases."
  echo
  echo "USER_DEFINED_ELAB_OPTIONS                         -- User-defined elaboration options, added to elab/elab_debug aliases."
  echo
  echo "USER_DEFINED_VHDL_COMPILE_OPTIONS                 -- User-defined vhdl compile options, added to com/dev_com aliases."
  echo
  echo "USER_DEFINED_VERILOG_COMPILE_OPTIONS              -- User-defined verilog compile options, added to com/dev_com aliases."
}
file_copy
h
