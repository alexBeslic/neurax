#!/bin/bash

# NEURAX Image Blur Demo Runner
# Convenience script for running the image blur demo with test images

DEMO_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="$DEMO_DIR/bin"
TEST_IMAGE="$DEMO_DIR/image_blur/test_images/Lenna.bmp"

echo "NEURAX Image Blur Demo Runner"
echo "============================="
echo

# Check if demo is built
if [ ! -f "$BIN_DIR/image_blur_demo" ]; then
    echo "Error: image_blur_demo not found. Please run 'make blur-demo' first."
    exit 1
fi

# Check if test image exists
if [ ! -f "$TEST_IMAGE" ]; then
    echo "Error: Test image not found at $TEST_IMAGE"
    exit 1
fi

# Set output filename
OUTPUT_FILE="${1:-blurred_output.bmp}"

echo "Input image: $TEST_IMAGE"
echo "Output image: $OUTPUT_FILE"
echo

# Set library path and run demo
export LD_LIBRARY_PATH="$DEMO_DIR/../software/lib/lib:$LD_LIBRARY_PATH"

echo "Running image blur demo..."
cd "$BIN_DIR"
./image_blur_demo "$TEST_IMAGE" "$OUTPUT_FILE" "${@:2}"

if [ $? -eq 0 ]; then
    echo
    echo "Success! Output saved to: $BIN_DIR/$OUTPUT_FILE"
    echo "File size: $(du -h "$BIN_DIR/$OUTPUT_FILE" | cut -f1)"
else
    echo
    echo "Demo failed with exit code $?"
fi
