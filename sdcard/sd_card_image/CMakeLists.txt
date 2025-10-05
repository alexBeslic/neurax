cmake_minimum_required(VERSION 3.10)
project(SDCardImage NONE)

# Output file name
set(OUTPUT_IMAGE sdcard_cv.img)

# Step 1: download script
add_custom_command(
    OUTPUT make_sdimage_p3.py
    COMMAND wget https://releases.rocketboards.org/release/2021.04/gsrd/tools/make_sdimage_p3.py -O make_sdimage_p3.py
    COMMENT "Downloading make_sdimage_p3.py"
)

# Step 2: create sdfs directory
add_custom_command(
    OUTPUT sdfs
    COMMAND ${CMAKE_COMMAND} -E make_directory sdfs
    DEPENDS make_sdimage_p3.py
    COMMENT "Creating sdfs directory"
)

# Step 3–5: copy kernel, dtb, rbf
add_custom_command(
    OUTPUT sdfs/zImage sdfs/socfpga.dtb sdfs/socfpga.rbf
    COMMAND ${CMAKE_COMMAND} -E copy ../kernel/zImage sdfs/zImage
    COMMAND ${CMAKE_COMMAND} -E copy ../../hardware/quartus/qsys/socfpga.dtb sdfs/socfpga.dtb
    COMMAND ${CMAKE_COMMAND} -E copy ../../hardware/quartus/output_files/socfpga.rbf sdfs/socfpga.rbf
    DEPENDS sdfs
    COMMENT "Copying zImage, socfpga.dtb, socfpga.rbf"
)

# Step 6: create extlinux dir + extlinux.conf
add_custom_command(
    OUTPUT sdfs/extlinux/extlinux.conf
    COMMAND ${CMAKE_COMMAND} -E make_directory sdfs/extlinux
    COMMAND ${CMAKE_COMMAND} -E echo "LABEL Linux Default" > sdfs/extlinux/extlinux.conf
    COMMAND ${CMAKE_COMMAND} -E echo "  KERNEL ../zImage" >> sdfs/extlinux/extlinux.conf
    COMMAND ${CMAKE_COMMAND} -E echo "  FDT ../socfpga.dtb" >> sdfs/extlinux/extlinux.conf
    COMMAND ${CMAKE_COMMAND} -E echo "  APPEND root=/dev/mmcblk0p2 rw rootwait earlyprintk console=ttyS0,115200n8" >> sdfs/extlinux/extlinux.conf
    DEPENDS sdfs/zImage sdfs/socfpga.dtb sdfs/socfpga.rbf
    COMMENT "Creating extlinux.conf"
)

# Step 7–8: rootfs
add_custom_command(
    OUTPUT rootfs
    COMMAND ${CMAKE_COMMAND} -E make_directory rootfs
    COMMAND ${CMAKE_COMMAND} -E copy ../root-fs/rootfs.tar rootfs/rootfs.tar
    COMMAND sh -c "tar xf rootfs/rootfs.tar -C rootfs"
    COMMAND rm -f rootfs/rootfs.tar
    DEPENDS sdfs/extlinux/extlinux.conf
    COMMENT "Preparing rootfs"
)

# Step 9: copy u-boot
add_custom_command(
    OUTPUT u-boot-with-spl.sfp
    COMMAND ${CMAKE_COMMAND} -E copy ../u-boot/build/u-boot-build/u-boot-socfpga/u-boot-with-spl.sfp ./u-boot-with-spl.sfp
    DEPENDS rootfs
    COMMENT "Copying u-boot-with-spl.sfp"
)

# Step 10: run python script
add_custom_command(
    OUTPUT ${OUTPUT_IMAGE}
    COMMAND sudo python3 ./make_sdimage_p3.py
        -f
        -P u-boot-with-spl.sfp,num=3,format=raw,size=10M,type=A2
        -P sdfs/*,num=1,format=fat32,size=100M
        -P rootfs/*,num=2,format=ext3,size=300M
        -s 512M
        -n ${OUTPUT_IMAGE}
    DEPENDS u-boot-with-spl.sfp
    COMMENT "Generating SD card image"
)

add_custom_target(clean-all
    COMMAND rm -f sdcard_cv.img
    COMMAND rm -f u-boot-with-spl.sfp
    COMMAND rm -f make_sdimage_p3.py
    COMMAND rm -rf sdfs
    COMMAND rm -rf rootfs
    COMMAND rm -rf CMakeFiles
    COMMAND rm -f CMakeCache.txt
    COMMAND rm -f cmake_install.cmake
    COMMAND rm -f Makefile
    COMMENT "Cleaning all generated files (image, folders, and CMake cache)"
)


add_custom_target(build_sd ALL DEPENDS ${OUTPUT_IMAGE})