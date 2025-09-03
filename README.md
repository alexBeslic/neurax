# NEURAX - Neural Network Accelerator for DE1-SoC Platform

## Quick Start

### Automatic Setup (Recommended)
```bash
# One-command setup (installs dependencies and builds everything)
./setup.sh

# Or step by step:
make install-deps  # Install system dependencies
make all           # Build everything
```

### Manual Dependency Installation
```bash
# Ubuntu/Debian
sudo apt-get install build-essential
```

## Overview

NEURAX is an advanced neural network accelerator designed for the DE1-SoC platform that combines an ARM Cortex-A9 processor (HPS) with a Cyclone V FPGA. The system enables efficient neural network inference execution through hardware acceleration of key operations.

## Architecture

### Hardware Components
- **FPGA Accelerator**: Configurable accelerator with three main blocks
  - 2D Convolution block
  - Activation block (ReLU, Tanh, Sigmoid)
  - Pooling block (Max/Average)
- **DMA Controller**: For efficient data transfer
- **Register Interface**: Avalon-MM/AXI-Lite for configuration

### Software Components
- **NEURAX Library**: C/C++ API for using the accelerator
- **Drivers**: Linux kernel modules for FPGA communication
- **Utility Libraries**: Image preprocessing, postprocessing
- **Demo Applications**: Usage examples for various NN tasks

## Project Structure

```
neurax/
├── hardware/           # FPGA design files
│   ├── fpga/          # Verilog/VHDL implementation
│   └── qsys/          # QSys system integration
├── software/          # Software components
│   ├── lib/           # NEURAX library
│   ├── drivers/       # Kernel drivers
│   └── utils/         # Utility libraries
├── demo/              # Demo applications
├── tests/             # Test suite
└── docs/              # Documentation

```

## Key Features

- **Configurability**: All blocks can be independently configured
- **Flexibility**: Support for 8-bit and 16-bit data
- **Standard Interfaces**: Avalon-ST, AXI-Stream for data
- **Optimization**: Parallelization and pipeline processing
- **Scalability**: Modular design for easy expansion

## FPGA Accelerator Specifications

### Convolution Block
- Configurable kernel dimensions (up to 11x11)
- Support for different stride values
- Optimized for 2D convolution

### Activation Block
- ReLU (Rectified Linear Unit)
- Tanh (Hyperbolic tangent)
- Sigmoid
- Runtime function selection

### Pooling Block
- Max pooling
- Average pooling
- Configurable window dimensions

## Installation and Usage

### Prerequisites
- Intel Quartus Prime (for FPGA synthesis)
- Altera SoC EDS (for software development)
- Linux kernel headers
- OpenCV (for image processing)

### Build Process
```bash
# Clone repository
git clone <repository-url>
cd neurax

# Install dependencies
make install-deps

# Build software
make software

# Build demo applications
make demo
```

## Documentation

Detailed documentation can be found in the `docs/` directory:
- System architecture
- API reference
- Usage tutorial
- Hardware specifications

## License

[Add license information]

## Contact

[Add contact information]
