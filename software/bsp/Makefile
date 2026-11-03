# Neurax BSP - Makefile for ARM cross-compilation (DE1-SoC)
# Usage:
#   make            - Build BSP library and test program
#   make test       - Build test program only
#   make lib        - Build BSP library only
#   make clean      - Remove build artifacts
#   make deploy     - SCP binaries to DE1-SoC (set BOARD_IP)

# =========================================================================
# Toolchain
# =========================================================================
TOOLCHAIN_DIR := $(realpath ../../toolchain/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf)
CROSS_COMPILE := $(TOOLCHAIN_DIR)/bin/arm-none-linux-gnueabihf-

CC      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
STRIP   := $(CROSS_COMPILE)strip

# =========================================================================
# Build flags
# =========================================================================
CFLAGS  := -Wall -Wextra -O2 -march=armv7-a -mfloat-abi=hard -mfpu=neon
CFLAGS  += -I.
LDFLAGS :=

# Debug build: make DEBUG=1
ifdef DEBUG
  CFLAGS  += -g -O0 -DDEBUG
else
  CFLAGS  += -DNDEBUG
endif

# =========================================================================
# Output
# =========================================================================
BUILD_DIR := build
LIB_NAME  := libneurax_bsp.a
TEST_BIN  := neurax_test

# =========================================================================
# Sources
# =========================================================================
BSP_SRC   := neurax_bsp.c
BSP_OBJ   := $(BUILD_DIR)/neurax_bsp.o

TEST_SRC  := neurax_test.c
TEST_OBJ  := $(BUILD_DIR)/neurax_test.o

# =========================================================================
# Targets
# =========================================================================
.PHONY: all lib test clean deploy

all: lib test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# BSP library
lib: $(BUILD_DIR)/$(LIB_NAME)

$(BUILD_DIR)/$(LIB_NAME): $(BSP_OBJ) | $(BUILD_DIR)
	$(AR) rcs $@ $^
	@echo "Built: $@"

$(BSP_OBJ): $(BSP_SRC) neurax_bsp.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Test program
test: $(BUILD_DIR)/$(TEST_BIN)

$(BUILD_DIR)/$(TEST_BIN): $(TEST_OBJ) $(BUILD_DIR)/$(LIB_NAME) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -L$(BUILD_DIR) -lneurax_bsp -o $@
	@echo "Built: $@"
	@echo "  (stripped copy):"
	cp $@ $@.stripped
	$(STRIP) $@.stripped

$(TEST_OBJ): $(TEST_SRC) neurax_bsp.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# =========================================================================
# Deploy to board via SCP
# =========================================================================
BOARD_IP   ?= 192.168.23.30
BOARD_USER ?= root
BOARD_DIR  ?= /root

deploy: $(BUILD_DIR)/$(TEST_BIN)
	scp $(BUILD_DIR)/$(TEST_BIN).stripped $(BOARD_USER)@$(BOARD_IP):$(BOARD_DIR)/$(TEST_BIN)
	@echo "Deployed to $(BOARD_USER)@$(BOARD_IP):$(BOARD_DIR)/$(TEST_BIN)"

# =========================================================================
# Clean
# =========================================================================
clean:
	rm -rf $(BUILD_DIR)
