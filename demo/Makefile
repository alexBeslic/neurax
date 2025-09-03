# NEURAX Demo Applications Makefile

# Compiler settings
CC ?= gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -L../software/lib/lib -static -lneurax -lm

# Debug build
ifdef DEBUG
CFLAGS += -g -DNEURAX_DEBUG -O0
endif

# Directories
BLUR_SRC_DIR = image_blur
BUILD_DIR = build
BIN_DIR = bin

# Include paths
INCLUDES = -I../software/lib/include

# Source files
BLUR_SOURCES = $(BLUR_SRC_DIR)/image_blur_demo.c

# Output binaries
BLUR_TARGET = $(BIN_DIR)/image_blur_demo

# Default target
.PHONY: all clean install blur-demo

all: $(BLUR_TARGET)

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build image blur demo
$(BLUR_TARGET): $(BLUR_SOURCES) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(BLUR_SOURCES) -o $@ $(LDFLAGS)
	@echo "Image blur demo application built: $@"

# Build specific demos
blur-demo: $(BLUR_TARGET)

# Development build
dev:
	$(MAKE) CC=gcc DEBUG=1 all

# Install demo applications
install: all
	@echo "Installing demo applications..."
	sudo cp $(BLUR_TARGET) /usr/local/bin/
	@echo "Installation complete"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)
	@echo "Clean complete"

# Create sample image and run blur
demo-blur: $(BLUR_TARGET)
	@echo "Creating sample image and running blur demo..."
	@echo "Using convenience script for easier testing..."
	./run_blur_demo.sh sample_blur.bmp --kernel-size 7 --sigma 2.0

# Show help
help:
	@echo "NEURAX Demo Makefile"
	@echo "===================="
	@echo "Available targets:"
	@echo "  all       - Build all demo applications"
	@echo "  blur-demo - Build image blur demo only"
	@echo "  dev       - Build for development"
	@echo "  install   - Install demos to system"
	@echo "  clean     - Remove build artifacts"
	@echo "  demo-blur - Run blur demo with test image and custom parameters"
	@echo "  help      - Show this help"
