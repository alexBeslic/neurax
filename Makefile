# NEURAX Makefile
# Build system for NEURAX neural network accelerator

# Directories
SOFTWARE_DIR = software
DEMO_DIR = demo

# Cross-compilation tools for ARM (optional)
CROSS_COMPILE ?= 
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++

# Build targets
.PHONY: all clean software demo help install install-deps

all: software demo

help:
	@echo "NEURAX Build System"
	@echo "=================="
	@echo "Available commands:"
	@echo "  make install-deps - Install required system dependencies"
	@echo "  make all          - Build complete system"
	@echo "  make software     - Build software libraries"
	@echo "  make demo         - Build demo applications"
	@echo "  make clean        - Clean build artifacts"
	@echo "  make install      - Install to target system"

# Software build
software:
	@echo "Building software libraries..."
	$(MAKE) -C $(SOFTWARE_DIR) CC=$(CC) CXX=$(CXX)

# Demo applications
demo: software
	@echo "Building demo applications..."
	$(MAKE) -C $(DEMO_DIR) CC=$(CC) CXX=$(CXX)

# Installation
install: all
	@echo "Installing NEURAX system..."
	# Install libraries
	cp $(SOFTWARE_DIR)/lib/*.so /usr/local/lib/
	cp $(SOFTWARE_DIR)/lib/*.a /usr/local/lib/
	# Install headers
	cp -r $(SOFTWARE_DIR)/include/* /usr/local/include/
	# Install demo applications
	cp $(DEMO_DIR)/bin/* /usr/local/bin/
	# Update library cache
	ldconfig

# Install system dependencies
install-deps:
	@echo "Installing system dependencies..."
	./install-deps.sh

# Clean all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	$(MAKE) -C $(SOFTWARE_DIR) clean
	$(MAKE) -C $(DEMO_DIR) clean

# Quick development targets
dev-software:
	@echo "Quick software build for development..."
	$(MAKE) -C $(SOFTWARE_DIR) dev CC=$(CC) CXX=$(CXX)
