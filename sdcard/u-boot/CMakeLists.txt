cmake_minimum_required(VERSION 3.16)
project(BuildUbootSocfpga C)

include(ExternalProject)

# Set where to put everything
set(UBOOT_BUILD_DIR ${CMAKE_BINARY_DIR}/u-boot-build)
set(UBOOT_SOURCE_DIR ${UBOOT_BUILD_DIR}/u-boot-socfpga)
set(QUARTUS_QTS_DIR /mnt/d/Fax/master/neurax/neurax/hardware/quartus/)
set(BSP_DIR /mnt/d/Fax/master/neurax/neurax/preloader/)

# Cross compiler
set(CROSS_COMPILE arm-linux-gnueabihf-)

ExternalProject_Add(
    uboot
    PREFIX ${UBOOT_BUILD_DIR}
    GIT_REPOSITORY https://github.com/altera-opensource/u-boot-socfpga.git
    GIT_TAG socfpga_v2025.04
    
    SOURCE_DIR ${UBOOT_SOURCE_DIR}
    
    # Configure step: run qts-filter.sh
    CONFIGURE_COMMAND
        bash ${UBOOT_SOURCE_DIR}/arch/arm/mach-socfpga/qts-filter.sh
            cyclone5
            ${QUARTUS_QTS_DIR}
            ${BSP_DIR}
            ${UBOOT_SOURCE_DIR}/board/altera/cyclone5-socdk/qts/
    
    # Build step
    BUILD_COMMAND
        $(MAKE) -C ${UBOOT_SOURCE_DIR} socfpga_cyclone5_defconfig
    COMMAND
        $(MAKE) -C ${UBOOT_SOURCE_DIR} CROSS_COMPILE=${CROSS_COMPILE}
    
    BUILD_IN_SOURCE 1
    
    INSTALL_COMMAND ""
)