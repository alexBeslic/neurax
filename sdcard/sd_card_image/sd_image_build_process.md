# SD Card Image Build Process Documentation

This document describes the build process defined in `CMakeLists.txt` for creating a bootable SD card image for the Neurax FPGA project. It outlines all steps, commands, and what they accomplish.

## Overview

The CMakeLists.txt orchestrates a complete SD card image creation process that:
1. Downloads the SD card image creation Python script
2. Prepares the boot partition (sdfs) with kernel, device tree, FPGA bitstream
3. Creates boot loader configuration (extlinux)
4. Prepares the root file system (rootfs)
5. Copies the U-Boot bootloader binary
6. Generates the final SD card image file

## Output File

```
Final SD Card Image: sdcard_cv.img (512 MB total)
```

## Build Directory Structure

The build process creates the following directory structure:

```
sdfs/                          # Boot partition (FAT32, 100M)
  ├── zImage                   # Linux kernel
  ├── socfpga.dtb             # Device tree blob
  ├── socfpga.rbf             # FPGA bitstream
  ├── u-boot.scr              # U-Boot boot script
  └── extlinux/
      └── extlinux.conf       # Boot loader configuration

rootfs/                        # Root file system (EXT3, 300M)
  └── [extracted rootfs files]

u-boot-with-spl.sfp          # U-Boot bootloader binary (partition 3, 10M)

sdcard_cv.img                 # Final bootable SD card image
```

## Build Stages

### Stage 1: Download SD Card Image Creation Script

**Purpose:** Get the Python script that creates the SD card image from components.

**Command:**
```bash
cd /root/neurax/neurax/sdcard/sd_card_image
wget https://releases.rocketboards.org/release/2021.04/gsrd/tools/make_sdimage_p3.py -O make_sdimage_p3.py
```

**Expected Output:**
- File: `make_sdimage_p3.py` (Python script for creating SD card image)

**What it does:** Downloads Intel's Rocket Boards SD card image creation tool that handles partitioning and image generation.

---

### Stage 2: Create SDFS Directory

**Purpose:** Create the boot partition staging directory.

**Command:**
```bash
mkdir -p sdfs
```

**Expected Output:**
- Directory: `sdfs/`

**Dependencies:** Requires `make_sdimage_p3.py` to exist

---

### Stage 3: Copy Boot Files to SDFS

**Purpose:** Copy kernel, device tree, FPGA bitstream, and boot script to the boot partition directory.

**Command:**
```bash
cd /root/neurax/neurax/sdcard/sd_card_image

# Copy Linux kernel
cp ../kernel/zImage sdfs/zImage

# Copy device tree blob
cp ../../hardware/quartus/qsys/socfpga.dtb sdfs/socfpga.dtb

# Copy FPGA bitstream
cp ../../hardware/quartus/output_files/socfpga.rbf sdfs/socfpga.rbf

# Copy U-Boot boot script
cp ../u-boot/u-boot.scr sdfs/u-boot.scr
```

**Expected Output Files in sdfs/:**
- `zImage` - Linux kernel (from `sdcard/kernel/`)
- `socfpga.dtb` - Device tree (from `hardware/quartus/qsys/`)
- `socfpga.rbf` - FPGA bitstream (from `hardware/quartus/output_files/`)
- `u-boot.scr` - U-Boot boot script (from `sdcard/u-boot/`)

**Dependencies:** Requires `sdfs/` directory to exist

---

### Stage 4: Create Boot Loader Configuration (extlinux)

**Purpose:** Create the extlinux boot configuration that tells U-Boot how to boot Linux.

**Command:**
```bash
cd /root/neurax/neurax/sdcard/sd_card_image

# Create extlinux directory
mkdir -p sdfs/extlinux

# Create extlinux.conf configuration file
cat > sdfs/extlinux/extlinux.conf << 'EOF'
LABEL Linux Default
  KERNEL ../zImage
  FDT ../socfpga.dtb
  APPEND root=/dev/mmcblk0p2 rw rootwait earlyprintk console=ttyS0,115200n8
EOF
```

**Configuration Breakdown:**
- `LABEL Linux Default` - Boot menu label
- `KERNEL ../zImage` - Path to Linux kernel relative to extlinux.conf
- `FDT ../socfpga.dtb` - Flattened Device Tree file path
- `APPEND root=/dev/mmcblk0p2 rw rootwait earlyprintk console=ttyS0,115200n8` - Kernel boot parameters:
  - `root=/dev/mmcblk0p2` - Root filesystem on MMC card partition 2
  - `rw` - Mount root filesystem read-write
  - `rootwait` - Wait for root device to appear
  - `earlyprintk` - Enable early kernel debug messages
  - `console=ttyS0,115200n8` - Use serial port at 115200 baud for console output

**Expected Output:**
- File: `sdfs/extlinux/extlinux.conf` (Boot configuration)

**Dependencies:** Requires `sdfs/zImage`, `sdfs/socfpga.dtb`, `sdfs/socfpga.rbf` to exist

---

### Stage 5: Prepare Root File System

**Purpose:** Extract and prepare the root file system that contains all the Linux userspace files.

**Command:**
```bash
cd /root/neurax/neurax/sdcard/sd_card_image

# Create rootfs directory
mkdir -p rootfs

# Copy the root filesystem archive
cp ../root-fs/rootfs.tar rootfs/rootfs.tar

# Extract the archive
tar xf rootfs/rootfs.tar -C rootfs

# Remove the archive (no longer needed)
rm -f rootfs/rootfs.tar
```

**What happens:**
1. Creates `rootfs/` directory
2. Copies `rootfs.tar` from `sdcard/root-fs/` directory
3. Extracts the tar archive into rootfs directory (creates all user space files, libraries, binaries)
4. Deletes the tar file to save space

**Expected Output:**
- Directory: `rootfs/` (containing full Linux root filesystem)

**Dependencies:** Requires `sdfs/extlinux/extlinux.conf` to exist

---

### Stage 6: Copy U-Boot Bootloader

**Purpose:** Copy the compiled U-Boot binary with SPL (Secondary Program Loader) to be placed in partition 3.

**Command:**
```bash
cd /root/neurax/neurax/sdcard/sd_card_image

# Copy U-Boot binary with SPL
cp ../u-boot/build/u-boot-build/u-boot-socfpga/u-boot-with-spl.sfp ./u-boot-with-spl.sfp
```

**Expected Output:**
- File: `u-boot-with-spl.sfp` (U-Boot bootloader binary)

**What it is:** This is the complete bootloader that:
- Initializes the HPS (Hard Processor System)
- Loads the FPGA bitstream
- Loads the Linux kernel from the boot partition
- Executes the kernel

**Dependencies:** Requires `rootfs` directory to be prepared

---

### Stage 7: Generate Final SD Card Image

**Purpose:** Create the complete SD card image with all partitions.

**Command:**
```bash
cd /root/neurax/neurax/sdcard/sd_card_image

sudo python3 ./make_sdimage_p3.py \
    -f \
    -P u-boot-with-spl.sfp,num=3,format=raw,size=10M,type=A2 \
    -P sdfs/*,num=1,format=fat32,size=100M \
    -P rootfs/*,num=2,format=ext3,size=300M \
    -s 512M \
    -n sdcard_cv.img
```

**Command Parameters Breakdown:**

| Parameter | Meaning |
|-----------|---------|
| `-f` | Force creation (overwrite if exists) |
| `-s 512M` | Total SD card image size: 512 MB |
| `-n sdcard_cv.img` | Output filename |
| `-P` | Partition definition (can have multiple) |

**Partition Definitions:**

1. **U-Boot Bootloader (Partition 3)**
   ```
   -P u-boot-with-spl.sfp,num=3,format=raw,size=10M,type=A2
   ```
   - File: `u-boot-with-spl.sfp`
   - Partition number: 3
   - Format: Raw (binary)
   - Size: 10 MB
   - Type: A2 (Intel hex)

2. **Boot Partition (Partition 1)**
   ```
   -P sdfs/*,num=1,format=fat32,size=100M
   ```
   - Files: All contents of `sdfs/` directory
   - Partition number: 1 (first partition)
   - Format: FAT32
   - Size: 100 MB
   - Contains: Kernel, device tree, FPGA bitstream, boot scripts

3. **Root File System (Partition 2)**
   ```
   -P rootfs/*,num=2,format=ext3,size=300M
   ```
   - Files: All contents of `rootfs/` directory
   - Partition number: 2 (second partition)
   - Format: EXT3
   - Size: 300 MB
   - Contains: All Linux userspace files, libraries, binaries

**Boot Sequence:** When the SD card boots:
1. SoC reads U-Boot from partition 3
2. U-Boot loads FPGA bitstream (socfpga.rbf) from partition 1
3. U-Boot loads kernel (zImage) from partition 1
4. U-Boot loads device tree (socfpga.dtb) from partition 1
5. U-Boot executes kernel with extlinux configuration
6. Kernel mounts root filesystem from partition 2 (rootfs)
7. Linux starts normally

**Expected Output:**
- File: `sdcard_cv.img` (Bootable SD card image, 512 MB)

**Dependencies:** Requires `u-boot-with-spl.sfp` to exist

---

## Partition Layout on SD Card

```
Offset    Size      Partition  Format   Purpose
────────────────────────────────────────────────────────
0 MB      10 MB     3          RAW      U-Boot Bootloader
10 MB     100 MB    1          FAT32    Boot Files (kernel, DTB, FPGA bitstream)
110 MB    300 MB    2          EXT3     Root Filesystem
410 MB    102 MB    (unused)            Free space
────────────────────────────────────────────────────────
Total: 512 MB
```

---

## Clean Target

**Target Name:** `clean-all`

**Purpose:** Remove all generated files and directories to do a clean rebuild.

**Command:**
```bash
cd /root/neurax/neurax/sdcard/sd_card_image

# Remove generated files
rm -f sdcard_cv.img
rm -f u-boot-with-spl.sfp
rm -f make_sdimage_p3.py

# Remove generated directories
rm -rf sdfs
rm -rf rootfs
rm -rf CMakeFiles

# Remove CMake cache files
rm -f CMakeCache.txt
rm -f cmake_install.cmake
rm -f Makefile
```

**Run clean command:**
```bash
cmake --build build --target clean-all
```

---

## Writing the Image to an SD Card

Once the image is created, you can write it to a physical SD card:

```bash
# List your SD card device
lsblk

# IMPORTANT: Replace /dev/sdX with your actual SD card device!
# BE VERY CAREFUL - this command will overwrite the device!

# Write the image
sudo dd if=sdcard_cv.img of=/dev/sdX bs=4M status=progress
sudo sync

# Verify write successful
sudo eject /dev/sdX
```

**Warnings:**
- Replace `/dev/sdX` with your actual SD card device (check `lsblk` to find it)
- This command will ERASE all data on the SD card
- Use `bs=4M` for faster writing on modern SD cards

---

## Input Files Required

Before building, ensure these files exist:

| File | Location | Purpose |
|------|----------|---------|
| `zImage` | `sdcard/kernel/` | Linux kernel |
| `socfpga.dtb` | `hardware/quartus/qsys/` | Device tree blob |
| `socfpga.rbf` | `hardware/quartus/output_files/` | FPGA bitstream |
| `u-boot.scr` | `sdcard/u-boot/` | U-Boot boot script |
| `rootfs.tar` | `sdcard/root-fs/` | Root filesystem archive |
| `u-boot-with-spl.sfp` | `sdcard/u-boot/build/u-boot-build/u-boot-socfpga/` | U-Boot bootloader binary |

---

## Output Files

After successful build:

| File | Location | Size | Purpose |
|------|----------|------|---------|
| `sdcard_cv.img` | Current directory | 512 MB | Complete bootable SD card image |

---

## Environment Requirements

- **Python 3** - For make_sdimage_p3.py script
- **sudo privileges** - Required to write raw images (security)
- **wget** - For downloading the script
- **tar** - For extracting rootfs
- **bash** - For shell commands
- **dd** - For writing to SD card (Linux standard)

---

## Build Command

Using CMake:

```bash
cd /root/neurax/neurax/sdcard/sd_card_image
mkdir build
cd build
cmake ..
cmake --build . --target build_sd
```

Or to clean everything before rebuilding:

```bash
cmake --build . --target clean-all
cmake --build . --target build_sd
```

---

## Troubleshooting

### Script download fails
- Check internet connection
- Verify Rocket Boards website is accessible
- Try downloading manually: `wget https://releases.rocketboards.org/release/2021.04/gsrd/tools/make_sdimage_p3.py`

### "Permission denied" when running python3 script
- Ensure the script has execute permissions: `chmod +x make_sdimage_p3.py`
- May need to run with `sudo` (script already does this)

### Missing input files (zImage, DTB, RBF)
- Ensure U-Boot and hardware builds completed successfully
- Check file paths in CMakeLists.txt match your actual directory structure
- Run the u-boot CMakeLists.txt first to generate required files

### rootfs.tar not found
- Extract it from root-fs directory if not present
- Check that `sdcard/root-fs/` directory exists and contains rootfs files

### Image write fails with "dd: error writing"
- Ensure SD card is not write-protected
- Verify you have the correct device (`lsblk` to check)
- Try unmounting the SD card first: `sudo umount /dev/sdX*`

### Image too large for SD card
- Image is 512MB, ensure your SD card is at least 1GB
- Adjust partition sizes in CMakeLists.txt if needed:
  - Reduce `-P rootfs/*,num=2,format=ext3,size=300M` to smaller value
  - Reduce `-s 512M` total size accordingly
