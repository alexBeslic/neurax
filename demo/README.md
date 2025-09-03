# NEURAX Demo Applications

This directory contains demonstration applications showcasing the capabilities of the NEURAX neural network accelerator library.

## Available Demos

### 1. Simple CNN Demo (`simple_cnn/`)
- **Purpose**: Demonstrates basic CNN operations (convolution, pooling, activation)
- **Features**: 
  - 32x32x3 RGB image processing
  - 3x3 convolution with 32 filters
  - Max pooling and sigmoid activation
  - Performance benchmarking
- **Usage**: `make run` or `make benchmark`

### 2. Image Blur Demo (`image_blur/`)
- **Purpose**: Real-world image processing using Gaussian blur filters
- **Features**:
  - BMP image loading and saving
  - Configurable Gaussian blur kernels
  - Sample image generation
  - Multiple blur intensity levels
- **Usage**: `make demo-blur` or `make run-blur`

### 3. Real-Time Video Demo (`realtime_video/`)
- **Purpose**: Live video processing with web streaming capabilities
- **Features**:
  - Webcam input via V4L2
  - Real-time filter processing (blur, edge, sharpen)
  - HTTP/MJPEG web streaming
  - Performance monitoring
  - Interactive web interface
- **Usage**: `make run-video` (opens http://localhost:8080/stream)

## Quick Start

### Build All Demos
```bash
make all
```

### Run CNN Demo
```bash
make run           # Basic inference demo
make benchmark     # Performance benchmark
```

### Run Real-Time Video Demo
```bash
make run-video     # Start video processing server
# Open browser: http://localhost:8080/stream
```

### Individual Demo Builds
```bash
make cnn-demo      # Build CNN demo only
make blur-demo     # Build image blur demo only
make video-demo    # Build real-time video demo only
```

## Demo Descriptions

### Simple CNN Demo
```
Input: 32x32x3 synthetic image
├── Convolution: 3x3x3x32 filters, ReLU activation
├── Max Pooling: 2x2 pool, stride 2
└── Sigmoid Activation
Output: 15x15x32 feature maps
```

**Performance Results** (CPU emulation):
- Convolution: ~2.6 ms
- Pooling: ~0.2 ms  
- Activation: ~0.04 ms
- **Total: ~2.9 ms**

### Image Blur Demo
```
Input: BMP image (24/32-bit)
├── Load & Convert to tensors
├── Generate Gaussian kernel
├── Apply convolution per channel
└── Convert & Save result
Output: Blurred BMP image
```

**Performance Results** (256x256 RGB):
- 3x3 kernel: ~7.6 ms
- 7x7 kernel: ~48.6 ms

## Hardware Acceleration

Both demos automatically detect hardware availability:
- **FPGA Available**: Uses hardware acceleration
- **CPU Fallback**: Pure software implementation

Current status: CPU emulation mode (FPGA hardware not detected)

## Build Requirements

- GCC compiler
- NEURAX library (automatically linked)
- Math library support
- Standard C libraries

## File Structure

```
demo/
├── Makefile                    # Build system
├── README.md                   # This file
├── bin/                        # Built executables
│   ├── simple_cnn_demo
│   └── image_blur_demo
├── simple_cnn/                 # CNN demo source
│   └── simple_cnn_demo.c
└── image_blur/                 # Image processing demo
    ├── image_blur_demo.c
    └── README.md               # Detailed blur demo docs
```

## Generated Files

Running demos creates:
- `sample.bmp` - Test pattern image (256x256 RGB)
- `blurred.bmp` - Heavy blur result
- `light_blur.bmp` - Light blur result

## Development

### Debug Build
```bash
make dev           # Build with debug symbols
```

### Clean Build
```bash
make clean         # Remove build artifacts
make all           # Rebuild everything
```

### System Installation
```bash
make install       # Install demos to /usr/local/bin/
```

## Makefile Targets

| Target | Description |
|--------|-------------|
| `all` | Build all demo applications |
| `cnn-demo` | Build CNN demo only |
| `blur-demo` | Build image blur demo only |
| `dev` | Development build with debug info |
| `run` | Run simple CNN demo |
| `benchmark` | Run CNN performance benchmark |
| `run-blur` | Show blur demo usage |
| `demo-blur` | Create sample and run blur demo |
| `install` | Install demos system-wide |
| `clean` | Clean build artifacts |
| `help` | Show available targets |

## Error Handling

All demos include comprehensive error handling:
- Device initialization failures
- Memory allocation errors
- File I/O problems
- Invalid parameters
- Hardware communication issues

## Performance Notes

- Times shown are for CPU emulation mode
- Hardware acceleration would provide significant speedup
- Larger images and kernels require more processing time
- Memory usage scales with tensor dimensions

## Next Steps

To extend the demos:
1. Add more image processing filters (edge detection, sharpening)
2. Implement batch processing capabilities
3. Add support for different image formats
4. Create video processing demos
5. Benchmark against other frameworks
