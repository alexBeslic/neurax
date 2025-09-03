# NEURAX Software Library Makefile

# Compiler settings
CC ?= gcc
CXX ?= g++
AR = ar
CFLAGS = -Wall -Wextra -O2 -fPIC -std=c99
CXXFLAGS = -Wall -Wextra -O2 -fPIC -std=c++11
LDFLAGS = -shared
ARFLAGS = rcs

# Debug build
ifdef DEBUG
CFLAGS += -g -DNEURAX_DEBUG -O0
CXXFLAGS += -g -DNEURAX_DEBUG -O0
endif

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
LIB_DIR = lib

# Source files
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
CXX_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)

# Object files
C_OBJECTS = $(C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CXX_OBJECTS = $(CXX_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
OBJECTS = $(C_OBJECTS) $(CXX_OBJECTS)

# Library names
STATIC_LIB = $(LIB_DIR)/libneurax.a
SHARED_LIB = $(LIB_DIR)/libneurax.so
VERSION = 1.0.0
SHARED_LIB_VERSION = $(LIB_DIR)/libneurax.so.$(VERSION)

# Include paths
INCLUDES = -I$(INCLUDE_DIR)

# System libraries
LIBS = -lm -lpthread

# Default target
.PHONY: all clean install dev

all: $(STATIC_LIB) $(SHARED_LIB)

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile C++ source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Create static library
$(STATIC_LIB): $(OBJECTS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	@echo "Static library created: $@"

# Create shared library
$(SHARED_LIB): $(OBJECTS) | $(LIB_DIR)
	$(CC) $(LDFLAGS) -Wl,-soname,libneurax.so.1 -o $(SHARED_LIB_VERSION) $(OBJECTS) $(LIBS)
	ln -sf libneurax.so.$(VERSION) $(SHARED_LIB)
	@echo "Shared library created: $@"

# Development build (native compilation for testing)
dev:
	$(MAKE) CC=gcc CXX=g++ DEBUG=1 all

# Install library
install: all
	@echo "Installing NEURAX library..."
	sudo cp $(STATIC_LIB) /usr/local/lib/
	sudo cp $(SHARED_LIB_VERSION) /usr/local/lib/
	sudo ln -sf /usr/local/lib/libneurax.so.$(VERSION) /usr/local/lib/libneurax.so
	sudo cp -r $(INCLUDE_DIR)/* /usr/local/include/
	sudo ldconfig
	@echo "Installation complete"

# Uninstall library
uninstall:
	@echo "Uninstalling NEURAX library..."
	sudo rm -f /usr/local/lib/libneurax.*
	sudo rm -f /usr/local/include/neurax.h
	sudo rm -f /usr/local/include/neurax_private.h
	sudo ldconfig
	@echo "Uninstall complete"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(LIB_DIR)
	@echo "Clean complete"

# Show build information
info:
	@echo "NEURAX Library Build Information:"
	@echo "================================="
	@echo "CC:              $(CC)"
	@echo "CXX:             $(CXX)"
	@echo "CFLAGS:          $(CFLAGS)"
	@echo "CXXFLAGS:        $(CXXFLAGS)"
	@echo "INCLUDES:        $(INCLUDES)"
	@echo "LIBS:            $(LIBS)"
	@echo "Sources:         $(words $(C_SOURCES)) C files, $(words $(CXX_SOURCES)) C++ files"
	@echo "Static library:  $(STATIC_LIB)"
	@echo "Shared library:  $(SHARED_LIB)"

# Show targets
help:
	@echo "NEURAX Library Makefile"
	@echo "======================="
	@echo "Available targets:"
	@echo "  all       - Build static and shared libraries"
	@echo "  dev       - Build for development (native, debug)"
	@echo "  install   - Install libraries to system"
	@echo "  uninstall - Remove libraries from system"
	@echo "  clean     - Remove build artifacts"
	@echo "  info      - Show build configuration"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Build options:"
	@echo "  DEBUG=1   - Enable debug build"
	@echo "  CC=<compiler> - Set C compiler"
	@echo "  CXX=<compiler> - Set C++ compiler"

# Dependencies (simplified)
$(BUILD_DIR)/neurax_core.o: $(INCLUDE_DIR)/neurax.h $(INCLUDE_DIR)/neurax_private.h
$(BUILD_DIR)/neurax_conv2d.o: $(INCLUDE_DIR)/neurax.h $(INCLUDE_DIR)/neurax_private.h
$(BUILD_DIR)/neurax_layers.o: $(INCLUDE_DIR)/neurax.h $(INCLUDE_DIR)/neurax_private.h
$(BUILD_DIR)/neurax_utils.o: $(INCLUDE_DIR)/neurax.h $(INCLUDE_DIR)/neurax_private.h
$(BUILD_DIR)/neurax_perf.o: $(INCLUDE_DIR)/neurax_private.h
