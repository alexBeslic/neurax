TOOLCHAIN_DIR ?= $(realpath ../../toolchain/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf)
CROSS_COMPILE ?= $(TOOLCHAIN_DIR)/bin/arm-none-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc
CFLAGS ?= -O2 -Wall -Wextra
UIO_DIR := ../driver/uio
LIBRARY := libneurax_fpga.so

.PHONY: all clean

all: $(LIBRARY)

$(LIBRARY): neurax_fpga.c $(UIO_DIR)/neurax_uio_test_conv.c $(UIO_DIR)/msgdma_uio.h $(UIO_DIR)/neurax_regs.h
	$(CC) $(CFLAGS) -fPIC -shared -o $@ neurax_fpga.c -lm

clean:
	rm -f $(LIBRARY)
