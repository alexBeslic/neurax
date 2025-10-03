cmake_minimum_required(VERSION 3.16)
project(BuildUbootSocfpga C)

include(ExternalProject)

# Set where to put everything
set(UBOOT_BUILD_DIR ${CMAKE_BINARY_DIR}/u-boot-build)
set(UBOOT_SOURCE_DIR ${UBOOT_BUILD_DIR}/u-boot-socfpga)
set(QUARTUS_HANDOFF_DIR ${CMAKE_SOURCE_DIR}/../../hardware/quartus/hps_isw_handoff/soc_system_hps_0)

# Cross compiler
set(ARCH arm)
set(CROSS_COMPILE arm-none-linux-gnueabihf-)

ExternalProject_Add(
    uboot
    PREFIX ${UBOOT_BUILD_DIR}
    GIT_REPOSITORY https://github.com/altera-opensource/u-boot-socfpga.git
    GIT_TAG socfpga_v2025.04
    
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
)