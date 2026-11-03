### Makefile for Altera SoC FPGA Message DMA Driver
# Prerequisites:
# - cd to kernel source directory and run: kernel/linux-socfpga$
# - make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- socfpga_defconfig
# - make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- (modules) -j$(nproc)

# After math
# Temporary Installation (For Testing)
# To load a module immediately without moving the file, use the insmod command. This requires the absolute path to the .ko file. 
#  +2
# Command: sudo insmod module_name.ko
# Verification: Run lsmod | grep module_name to confirm it is active.
# Removal: Use sudo rmmod module_name (do not include the .ko extension). 

# 2. Permanent Installation (Persistent)
# To make the module part of the system so it can be loaded by name or at boot, follow these steps: 
# Copy the file: Move the .ko file to the standard kernel module directory.
# sudo cp your_module.ko /lib/modules/$(uname -r)/kernel/drivers/.
# Update dependencies: Run depmod to update the module dependency database so the system "sees" the new file.
# sudo depmod -a
# Load by name: You can now load it using modprobe instead of the full path.
# sudo modprobe your_module_name (omit the .ko extension).
# Auto-load on boot: Add the module name (one per line) to the /etc/modules file or create a .conf file in /etc/modules-load.d/.
# echo "your_module_name" | sudo tee /etc/modules-load.d/your_module.conf

DRV_NAME		= altera-msgdma

TOOLCHAIN_DIR := $(realpath ../../../../toolchain/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf)
CROSS_COMPILE := $(TOOLCHAIN_DIR)/bin/arm-none-linux-gnueabihf-

KERNEL_SRC		=$(realpath ../../../../sdcard/kernel/linux-socfpga) # TODO : modify path
ARCH			=arm

# ccflags-y		+= -I$(KERNEL_SRC)/include/linux

obj-m			+= $(DRV_NAME).o

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules

install: all
	sudo cp $(DRV_NAME).ko /srv/nfs/de1_rootfs/home/root 

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) clean
