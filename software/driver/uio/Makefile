# Makefile — Neurax UIO driver + userspace test
#
# Targets:
#   all        build kernel module AND userspace test
#   module     build neurax_uio.ko (cross-compiled for ARM)
#   userspace  build neurax_uio_test (cross-compiled for ARM Linux)
#   install    copy outputs to NFS root for deployment
#   clean
#
# Requires the kernel source tree to have been configured with:
#   make ARCH=arm CROSS_COMPILE=... socfpga_defconfig
#   make ARCH=arm CROSS_COMPILE=... (modules_prepare)

TOOLCHAIN_DIR := $(realpath ../../../toolchain/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf)
CROSS_COMPILE := $(TOOLCHAIN_DIR)/bin/arm-none-linux-gnueabihf-
KERNEL_SRC    := $(realpath ../../../sdcard/kernel/linux-socfpga)
ARCH          := arm

# Kernel module
obj-m += neurax_uio.o

# NFS install path (update to match your setup)
NFS_ROOT := .

.PHONY: all module userspace install clean

all: module userspace

module:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		-C $(KERNEL_SRC) M=$(PWD) modules

userspace: neurax_uio_test

neurax_uio_test: neurax_uio_test_2.c msgdma_uio.h
	$(CROSS_COMPILE)gcc -O2 -Wall -Wextra -o $@ neurax_uio_test_2.c 

UIO_KO := $(KERNEL_SRC)/drivers/uio/uio.ko

install: module userspace
	cp $(UIO_KO)        $(NFS_ROOT)/
	cp neurax_uio.ko    $(NFS_ROOT)/
	cp neurax_uio_test  $(NFS_ROOT)/

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		-C $(KERNEL_SRC) M=$(PWD) clean
	rm -f neurax_uio_test
