#!/bin/bash

# NEURAX Quick Setup Script
# One-command setup for NEURAX development environment

echo "NEURAX Quick Setup"
echo "=================="
echo ""

# Install dependencies
echo "Step 1: Installing system dependencies..."
if ./install-deps.sh; then
    echo "✓ Dependencies installed successfully"
else
    echo "✗ Failed to install dependencies"
    exit 1
fi

echo ""

# Build the project
echo "Step 2: Building NEURAX..."
if make clean && make all; then
    echo "✓ Build completed successfully"
else
    echo "✗ Build failed"
    exit 1
fi

echo ""
echo "Setup complete! 🎉"
echo ""
echo "Available demo applications:"
ls -la demo/bin/ 2>/dev/null || echo "No demo applications found"
echo ""
echo "Quick test commands:"
echo "  ./demo/bin/image_blur_demo input.bmp output.bmp"
echo ""
echo "For more information, see README.md"
