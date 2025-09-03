#!/bin/bash

# NEURAX Dependencies Installation Script
# Automatically installs required packages for building and running NEURAX

set -e  # Exit on any error

echo "NEURAX Dependencies Installation"
echo "==============================="

# Detect the operating system
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VERSION=$VERSION_ID
else
    echo "Error: Cannot detect operating system"
    exit 1
fi

echo "Detected OS: $OS $VERSION"
echo ""

# Function to install packages on Ubuntu/Debian
install_ubuntu_debian() {
    echo "Installing packages for Ubuntu/Debian..."
    
    # Update package list
    echo "Updating package list..."
    sudo apt-get update
    
    # Core build tools
    echo "Installing core build tools..."
    sudo apt-get install -y \
        build-essential \
        cmake \
        make \
        gcc \
        g++ \
        pkg-config
    
    # NEURAX core dependencies
    echo "Installing NEURAX dependencies..."
    #sudo apt-get install -y \
    
    # Development tools (optional but recommended)
    echo "Installing development tools..."
    sudo apt-get install -y \
        git \
        gdb \
    
    echo "Ubuntu/Debian packages installed successfully!"
}

# Function to check if packages are installed correctly
verify_installation() {
    echo ""
    echo "Verifying installation..."
    
    # Check build tools
    echo -n "Checking gcc... "
    if command -v gcc &> /dev/null; then
        echo "✓ $(gcc --version | head -n1)"
    else
        echo "✗ Not found"
    fi
    
    echo -n "Checking make... "
    if command -v make &> /dev/null; then
        echo "✓ $(make --version | head -n1)"
    else
        echo "✗ Not found"
    fi
    
    # Check libraries using pkg-config
    echo -n "Checking libjpeg... "
    if pkg-config --exists libjpeg 2>/dev/null; then
        echo "✓ $(pkg-config --modversion libjpeg 2>/dev/null || echo "Found")"
    else
        echo "✗ Not found"
    fi
    
    echo -n "Checking libv4l2... "
    if pkg-config --exists libv4l2 2>/dev/null; then
        echo "✓ $(pkg-config --modversion libv4l2 2>/dev/null || echo "Found")"
    else
        echo "✗ Not found"
    fi
    
    # Check video devices
    echo -n "Checking video devices... "
    if ls /dev/video* &> /dev/null; then
        echo "✓ $(ls /dev/video* | wc -l) device(s) found"
    else
        echo "⚠ No video devices found (webcam may not be connected)"
    fi
    
    echo ""
}

# Main installation logic
case $OS in
    ubuntu|debian)
        install_ubuntu_debian
        ;;
    *)
        echo "Error: Unsupported operating system: $OS"
        echo ""
        echo "Manual installation required. Please install:"
        echo "- build-essential or equivalent"
        exit 1
        ;;
esac

# Verify installation
verify_installation

echo "Installation complete!"
echo ""
echo "You can now build NEURAX with:"
echo "  make all"
echo ""
echo "Or test individual components:"
echo "  make software"
echo "  make demo"
echo ""
