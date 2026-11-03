# U-Boot Build Process Documentation

This document describes the build process defined in `CMakeLists.txt` for the Neurax FPGA project. It outlines all steps, commands, and what they accomplish.

## Overview

The CMakeLists.txt orchestrates a complete FPGA boot chain build process that:
1. Generates an RBF file (FPGA bitstream) from a SOF file
2. Generates a device tree blob (DTB) from hardware description
3. Generates a U-Boot boot script from a user-defined script
4. Builds U-Boot bootloader from source code

## Build Directory Structure

```
U-Boot Build Directory: ${CMAKE_BINARY_DIR}/u-boot-build
U-Boot Source Directory: ${CMAKE_BINARY_DIR}/u-boot-build/u-boot-socfpga
Boot Script Output Directory: ${CMAKE_BINARY_DIR}/boot-scripts
```

## Configuration Variables

```cmake
ARCH = arm
CROSS_COMPILE = arm-none-linux-gnueabihf-
INTEL_FPGA_ROOT = $HOME/intelFPGA/20.1
EMBEDDED_SHELL = $HOME/intelFPGA/20.1/embedded/embedded_command_shell.sh

# Input directories
UBOOT_BUILD_DIR = ${CMAKE_BINARY_DIR}/u-boot-build
UBOOT_SOURCE_DIR = ${CMAKE_BINARY_DIR}/u-boot-build/u-boot-socfpga
QUARTUS_HANDOFF_DIR = ../../hardware/quartus/hps_isw_handoff/soc_system_hps_0
QUARTUS_OUTPUT_DIR = ../../hardware/quartus/output_files
QUARTUS_QSYS_DIR = ../../hardware/quartus/qsys
BOOT_SCRIPT_SOURCE = ./uboot.script
BOOT_SCRIPT_OUTPUT_DIR = ${CMAKE_BINARY_DIR}/boot-scripts
```

## Build Stages

### Stage 1: Generate RBF File from SOF

**Target Name:** `generate_rbf`

**Purpose:** Converts a Quartus SOF (SRAM Object File) to RBF (Raw Binary Format) for FPGA programming.

**Command:**
```bash
# First, source the Intel FPGA embedded shell
source ~/intelFPGA/20.1/embedded/embedded_command_shell.sh

# Then run the conversion
cd ../../hardware/quartus/output_files
quartus_cpf -c de1-soc-top.sof socfpga.rbf
```

**Expected Output:**
- File: `socfpga.rbf` (FPGA bitstream file in binary format)

---

### Stage 2: Generate DTB (Device Tree Blob)

**Target Name:** `generate_dtb`

**Purpose:** Creates a device tree binary from hardware description files (.sopcinfo and .xml board files). The device tree describes the hardware to the Linux kernel.

**Dependencies:** Runs after `generate_rbf`

**Command:**
```bash
# Source the Intel FPGA embedded shell
source ~/intelFPGA/20.1/embedded/embedded_command_shell.sh

# Generate the DTB file
cd ../../hardware/quartus/qsys
sopc2dts --input soc_system.sopcinfo \
         --output socfpga.dtb \
         --type dtb \
         --board hps_common_board_info.xml \
         --board soc_system_board_info.xml \
         --board neurax_board_info.xml \
         --bridge-removal all \
         --clocks
```

**Command Breakdown:**
- `--input soc_system.sopcinfo`: Input hardware description from Quartus Qsys
- `--output socfpga.dtb`: Output device tree blob filename
- `--type dtb`: Output format is binary device tree
- `--board`: Include multiple board information XML files
- `--bridge-removal all`: Remove all redundant bridge entries from device tree
- `--clocks`: Include clock information in device tree

**Expected Output:**
- File: `socfpga.dtb` (Device tree binary)

---

### Stage 3: Generate U-Boot Boot Script

**Target Name:** `generate_boot_script`

**Purpose:** Converts a human-readable U-Boot script into a binary format that U-Boot can execute during boot.

**Dependencies:** Requires `uboot.script` to exist

**Command:**
```bash
# Create output directory if it doesn't exist
mkdir -p ${CMAKE_BINARY_DIR}/boot-scripts

# Generate the boot script binary
cd ${CMAKE_BINARY_DIR}/boot-scripts
mkimage -A arm \
        -O linux \
        -T script \
        -C none \
        -a 0 \
        -e 0 \
        -n "Neurax U-boot script" \
        -d ./uboot.script \
        ./u-boot.scr
```

**Command Breakdown:**
- `-A arm`: Architecture is ARM
- `-O linux`: Operating system is Linux
- `-T script`: Image type is script
- `-C none`: No compression
- `-a 0`: Load address (not used for scripts)
- `-e 0`: Entry point (not used for scripts)
- `-n "Neurax U-boot script"`: Image name/description
- `-d ./uboot.script`: Input script file
- `-o ./u-boot.scr`: Output binary script file

**Expected Output:**
- File: `u-boot.scr` (Binary U-Boot script)

---

### Stage 4: Build U-Boot from Source

**Target Name:** `uboot`

**Purpose:** Downloads U-Boot source from GitHub and compiles it for the Cyclone V SoC FPGA.

**Dependencies:** Runs after `generate_dtb`

**Source Repository:**
```
Repository: https://github.com/altera-opensource/u-boot-socfpga.git
Tag/Branch: socfpga_v2025.07
```

**4a. BSP Generator Step**

**Purpose:** Generates board-specific code files from Quartus Handoff directory.

**Command:**
```bash
cd ${UBOOT_SOURCE_DIR}
python3 arch/arm/mach-socfpga/cv_bsp_generator/cv_bsp_generator.py \
    -i ../../hardware/quartus/hps_isw_handoff/soc_system_hps_0 \
    -o board/altera/cyclone5-socdk/qts/
```

**Parameters:**
- `-i`: Input directory with Quartus handoff files (HPS configuration)
- `-o`: Output directory for generated board support package files

**4b. Configure U-Boot**

**Command:**
```bash
cd ${UBOOT_SOURCE_DIR}
make socfpga_cyclone5_defconfig
```

**Purpose:** Sets U-Boot configuration for Cyclone V development kit

**4c. Compile U-Boot**

**Command:**
```bash
cd ${UBOOT_SOURCE_DIR}
export ARCH=arm
export CROSS_COMPILE=arm-none-linux-gnueabihf-
make
```

**Environment Variables:**
- `ARCH=arm`: Target architecture
- `CROSS_COMPILE=arm-none-linux-gnueabihf-`: Cross compiler prefix for ARM bare-metal toolchain

**Expected Output:**
- `u-boot.img` - Main U-Boot binary
- Other boot-related files in the build directory

---

## Complete Build Target

**Target Name:** `build_all`

**Purpose:** Builds everything in correct order with all dependencies.

**Execution Order:**
1. `generate_rbf` - Creates FPGA bitstream
2. `generate_dtb` - Creates device tree (depends on RBF)
3. `uboot` - Builds U-Boot (depends on DTB)
4. `generate_boot_script` - Creates boot script (depends on U-Boot)

**Build Command:**
```bash
cmake -B build
cmake --build build --target build_all
```

---

## Output Files

After successful build, the following files will be generated:

| File | Location | Purpose |
|------|----------|---------|
| `socfpga.rbf` | `hardware/quartus/output_files/` | FPGA bitstream (binary format) |
| `socfpga.dtb` | `hardware/quartus/qsys/` | Device tree blob for Linux kernel |
| `u-boot.img` | `build/u-boot-build/u-boot-socfpga/` | U-Boot bootloader binary |
| `u-boot.scr` | `build/boot-scripts/` | U-Boot boot script binary |

---

## Required Tools

- **CMake** (3.16+) - Build system
- **Intel Quartus** (20.1+) - FPGA design tools
- **Python 3** - BSP generator script
- **ARM GCC Toolchain** (`arm-none-linux-gnueabihf-`) - Cross compiler
- **mkimage** - U-Boot image tool (usually comes with U-Boot tools package)
- **Git** - For cloning U-Boot source
- **make** - Build tool for U-Boot compilation

---

## Troubleshooting

### RBF Generation fails
- Ensure `de1-soc-top.sof` exists in Quartus output directory
- Verify Intel FPGA installation path in `INTEL_FPGA_ROOT`
- Run: `source $INTEL_FPGA_ROOT/embedded/embedded_command_shell.sh` manually

### DTB Generation fails
- Check that `.sopcinfo` and `.xml` board files exist in `qsys/` directory
- Verify all three board XML files are present

### U-Boot Build fails
- Ensure cross compiler is installed and in PATH
- Check that BSP generator python script exists
- Verify Quartus handoff directory exists and contains HPS configuration files

### mkimage command not found
- Install U-Boot tools: `sudo apt-get install u-boot-tools`
