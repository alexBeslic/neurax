cmake_minimum_required(VERSION 3.16)
project(BuildUbootSocfpga C)

include(ExternalProject)

# Set where to put everything
set(UBOOT_BUILD_DIR ${CMAKE_BINARY_DIR}/u-boot-build)
set(UBOOT_SOURCE_DIR ${UBOOT_BUILD_DIR}/u-boot-socfpga)
set(QUARTUS_HANDOFF_DIR ${CMAKE_SOURCE_DIR}/../../hardware/quartus/hps_isw_handoff/soc_system_hps_0)
set(QUARTUS_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/../../hardware/quartus/output_files)
set(QUARTUS_QSYS_DIR ${CMAKE_SOURCE_DIR}/../../hardware/quartus/qsys)
set(BOOT_SCRIPT_SOURCE ${CMAKE_SOURCE_DIR}/boot.script)
set(BOOT_SCRIPT_OUTPUT_DIR ${CMAKE_BINARY_DIR}/boot-scripts)

# Cross compiler
set(ARCH arm)
set(CROSS_COMPILE arm-none-linux-gnueabihf-)

# Intel FPGA paths
set(INTEL_FPGA_ROOT $ENV{HOME}/intelFPGA/20.1)
set( ${INTEL_FPGA_ROOT}/embedded/embedded_command_shell.sh)

# Create boot script output directory
file(MAKE_DIRECTORY ${BOOT_SCRIPT_OUTPUT_DIR})

# Generate RBF file from SOF
add_custom_target(
    generate_rbf
    COMMENT "Generating RBF file from SOF..."
    COMMAND ${EMBEDDED_SHELL}
            quartus_cpf -c de1-soc-top.sof socfpga.rbf
    WORKING_DIRECTORY ${QUARTUS_OUTPUT_DIR}
)

# Generate DTB file from SOPCINFO
add_custom_target(
    generate_dtb
    COMMENT "Generating device tree blob..."
    COMMAND ${EMBEDDED_SHELL}
            sopc2dts --input soc_system.sopcinfo
                     --output socfpga.dtb
                     --type dtb
                     --board soc_system_board_info.xml
                     --board hps_common_board_info.xml
                     --bridge-removal all
                     --clocks
    WORKING_DIRECTORY ${QUARTUS_QSYS_DIR}
    DEPENDS generate_rbf
)


# Custom target to generate U-Boot boot script from existing boot.script
add_custom_target(
    generate_boot_script
    COMMENT "Generating U-Boot boot script (u-boot.scr) from ${BOOT_SCRIPT_SOURCE}..."
    COMMAND mkimage -A arm -O linux -T script -C none -a 0 -e 0 
            -n "Neurax U-boot script" 
            -d ${BOOT_SCRIPT_SOURCE} 
            ${BOOT_SCRIPT_OUTPUT_DIR}/u-boot.scr
    WORKING_DIRECTORY ${BOOT_SCRIPT_OUTPUT_DIR}
    DEPENDS ${BOOT_SCRIPT_SOURCE}
    BYPRODUCTS ${BOOT_SCRIPT_OUTPUT_DIR}/u-boot.scr
)

ExternalProject_Add(
    uboot
    PREFIX ${UBOOT_BUILD_DIR}
    GIT_REPOSITORY https://github.com/altera-opensource/u-boot-socfpga.git
    GIT_TAG socfpga_v2025.04
    GIT_SHALLOW 1
    
    SOURCE_DIR ${UBOOT_SOURCE_DIR}
    
    CONFIGURE_COMMAND
        python3 ${UBOOT_SOURCE_DIR}/arch/arm/mach-socfpga/cv_bsp_generator/cv_bsp_generator.py
            -i ${QUARTUS_HANDOFF_DIR}
            -o ${UBOOT_SOURCE_DIR}/board/altera/cyclone5-socdk/qts/
    
    # Build step
    BUILD_COMMAND
        $(MAKE) -C ${UBOOT_SOURCE_DIR} socfpga_cyclone5_defconfig
    COMMAND
        $(MAKE) -C ${UBOOT_SOURCE_DIR} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}

    BUILD_IN_SOURCE 1
    
    INSTALL_COMMAND ""
    
    DEPENDS generate_dtb
)

# Add a custom target to build everything in order
add_custom_target(
    build_all
    DEPENDS uboot generate_boot_script
    COMMENT "Building complete FPGA boot flow: RBF -> DTB -> U-Boot -> Boot Script"
)

# Display output locations
add_custom_command(
    TARGET build_all POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "===== Build Complete ====="
    COMMAND ${CMAKE_COMMAND} -E echo "U-Boot binaries: ${UBOOT_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "Boot script: ${BOOT_SCRIPT_OUTPUT_DIR}/u-boot.scr"
    COMMAND ${CMAKE_COMMAND} -E echo "RBF file: ${QUARTUS_OUTPUT_DIR}/socfpga.rbf"
    COMMAND ${CMAKE_COMMAND} -E echo "DTB file: ${QUARTUS_QSYS_DIR}/socfpga.dtb"
)