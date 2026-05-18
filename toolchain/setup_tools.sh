#!/bin/bash

set -e  # Exit on any error

# This script is used to setup the tools for the project. It will install the necessary tools and dependencies for the project.

TOOLCHAIN_FILE="gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz"
TOOLCHAIN_DIR="gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf"
TOOLCHAIN_URL="https://developer.arm.com/-/media/Files/downloads/gnu-a/10.2-2020.11/binrel/$TOOLCHAIN_FILE"

echo "Starting toolchain setup..."

# Fetch the toolchain for arm-none-linux-gnueabihf
echo "Downloading ARM toolchain..."
if ! wget "$TOOLCHAIN_URL"; then
    echo "Error: Failed to download toolchain from $TOOLCHAIN_URL"
    exit 1
fi

# Extract the toolchain
echo "Extracting toolchain..."
if ! tar xf "$TOOLCHAIN_FILE"; then
    echo "Error: Failed to extract toolchain"
    exit 1
fi

# Verify extraction
if [ ! -d "$TOOLCHAIN_DIR" ]; then
    echo "Error: Toolchain directory not found after extraction"
    exit 1
fi

# Remove the downloaded tarball
echo "Cleaning up tarball..."
rm "$TOOLCHAIN_FILE"

# Install the necessary dependencies for building the project
echo "Installing build dependencies..."
if ! sudo apt-get update && sudo apt-get install -y build-essential cmake libgmp3-dev libmpc-dev libncurses-dev libc6-dev; then
    echo "Error: Failed to install dependencies"
    exit 1
fi

echo "Toolchain setup completed successfully!"