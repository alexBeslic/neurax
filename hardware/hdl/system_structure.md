# Neurax HDL Architecture Documentation

Complete hierarchy of VHD modules with brief descriptions, inputs/outputs, and main usage.

---

## Layer 1: Top-Level Integration

### `de1_soc_top.vhd`
**Purpose:** Board-level top entity for DE1-SoC FPGA platform  
**Inputs:** CLOCK_50 (50 MHz clock), board I/O signals  
**Outputs:** Board-level signals (LEDs, displays, HPS interface)  
**Main Usage:** Entry point for FPGA design on DE1-SoC board; instantiates Neurax accelerator

---

### `neurax.vhd`
**Purpose:** Top-level accelerator wrapper connecting HPS to FPGA logic  
**Inputs:**  
- Avalon MM Slave (register configuration from HPS)  
- Avalon ST Sink (data stream from HPS)  

**Outputs:**  
- Avalon ST Source (results stream to HPS)  

**Main Usage:** 
- Bridges HPS Avalon interfaces to dual-port RAM  
- Coordinates register block, data interface, and accelerator core  
- Manages data flow: input → processing → output

---

## Layer 2: Control & Configuration

### `neurax_register_pkg.vhd`
**Purpose:** Package defining all register addresses, bit fields, and constants  
**Defines:** Register map (16 registers × 32-bit), pooling operations, activation types  
**Main Usage:** 
- Centralized register definitions  
- Used by register block for read/write operations  
- Constants for bit positions and operation modes

---

### `neurax_register_block.vhd`
**Purpose:** Avalon MM slave implementing register interface  
**Inputs:** Avalon MM slave bus (address, write data, read requests)  
**Outputs:** 
- Configuration signals (kernel size, stride, padding, etc.)  
- Status signals (done, busy, input ready, output ready)  
- Control signals (enable, start operation)  

**Main Usage:**
- Decodes register reads/writes from HPS  
- Stores/retrieves configuration parameters  
- Reports operation status back to HPS  
- Handles 16 registers at addresses 0x0–0xF

---

## Layer 3: Data Path

### `neurax_data_interface.vhd`
**Purpose:** Dual-port RAM wrapper (Altera altsyncram) for shared data access  
**Inputs:**  
- Port A (HPS): Avalon ST Sink (data stream), Avalon ST Source (result stream)  
- Port B (Accelerator): RAM read/write address, data in/out, write enable  

**Outputs:**  
- Port A: Data back to HPS  
- Port B: Data to/from accelerator  

**Main Usage:**
- Single shared 23KB RAM buffer  
- Port A: HPS streams input/weights/bias in; reads results out  
- Port B: Accelerator reads/writes via independent addresses  
- Enables simultaneous HPS and accelerator transactions

---

## Layer 4: Accelerator Core

### `FPGA_accelerator.vhd`
**Purpose:** Top-level accelerator with operation selector  
**Inputs:**  
- Configuration (kernel size, padding, stride, pool type, activation type)  
- Batch size, tensor size, alpha parameter  
- Control (start, enable)  
- RAM Port B interface  

**Outputs:**  
- Status (done, busy, operation type)  
- RAM Port B interface (address, data, write enable)  
- Debug info (cycles, status code)  

**Main Usage:**
- Routes data to active processing block (convolution, pooling, activation)  
- Manages operation sequencing  
- Multiplexes RAM access from multiple blocks  
- Reports completion status

---

## Layer 5: Processing Blocks

### `neurax_accel_types_pkg.vhd`
**Purpose:** Global package defining data types and constants  
**Defines:**  
- DATA_WIDTH (16-bit), fixed-point format (Q8.8)  
- max dimensions, channel counts, kernel sizes  
- Custom types: `conv_config_t`, `pooling_config_t`, `activation_type_t`  
- Array types for multi-dimensional data  

**Main Usage:** Imported by all processing blocks for consistent data representation

---

### `convolution_block.vhd`
**Purpose:** 2D convolution engine with padding, stride, and bias support  
**Inputs:**  
- Config (kernel size, stride, padding, input/output channels)  
- Input data stream + address port  
- Weight/bias data streams  

**Outputs:**  
- Output data stream + address port  
- Status (ready, done)  

**Main Usage:**
- Performs feature extraction via convolution  
- Reads input/weights/bias from RAM  
- Writes results back to RAM  
- Supports batched processing

---

### `pooling_block.vhd`
**Purpose:** Pooling engine supporting MAX, AVERAGE, MIN, SUM operations  
**Inputs:**  
- Config (pool size, stride, type, channels)  
- Batch size, input data stream  

**Outputs:**  
- Downsampled output data stream  
- Status, debug counters (position, cycles, window count)  

**Main Usage:**
- Reduces spatial dimensions via pooling  
- Processes multiple channels in parallel  
- Supports 4-channel parallel processing

---

### `activation_block.vhd`
**Purpose:** Activation function processor (ReLU, Sigmoid, Tanh, LeakyReLU, ELU, Linear)  
**Inputs:**  
- Activation type, tensor size  
- Alpha parameter (for LeakyReLU, ELU)  
- Input data stream  

**Outputs:**  
- Activated output data stream  
- Status, debug info (element count, processing cycles)  

**Main Usage:**
- Applies non-linear transformations  
- Uses LUT-based acceleration for sigmoid/tanh  
- Supports 4-unit parallel processing  
- 3-stage pipelined architecture

---

## Data Flow Diagram

```
HPS (Hard Processor System)
  ├─ Register Bus ──→ neurax_register_block ──→ FPGA_accelerator
  └─ Stream Bus ──→ neurax_data_interface (Dual-Port RAM)
                       ├─ Port A: HPS streams input/weights/bias, reads output
                       └─ Port B: FPGA_accelerator reads/writes data
                                    ↓
                          FPGA_accelerator selector
                          ├─ convolution_block
                          ├─ pooling_block
                          └─ activation_block
                                    ↓
                          Results written back to RAM
                                    ↓
                          HPS reads results via Stream Bus
```

---

## File Dependencies

```
de1_soc_top.vhd
  └─ neurax.vhd
      ├─ neurax_register_block.vhd
      │   ├─ neurax_register_pkg.vhd
      │   └─ accel_types.all
      ├─ neurax_data_interface.vhd
      │   └─ altera_mf.altsyncram
      └─ FPGA_accelerator.vhd
          ├─ accel_types.all
          ├─ convolution_block.vhd
          │   └─ accel_types.all
          ├─ pooling_block.vhd
          │   └─ accel_types.all
          └─ activation_block.vhd
              └─ accel_types.all

neurax_accel_types_pkg.vhd (package, imported by all blocks)
neurax_register_pkg.vhd (package, imported by register block)
```

---

## Quick Reference: Register Addresses

| Address | Register | Purpose |
|---------|----------|---------|
| 0x0 | CMD | Command & control (enable, start, operation select) |
| 0x1 | STATUS | Status flags (busy, done, input/output ready) |
| 0x2 | CONFIG | General configuration (reserved) |
| 0x3 | CONV_CONFIG_0 | Convolution config (stride, padding, groups, kernel) |
| 0x4 | CONV_CONFIG_1 | Convolution config (input/output channels) |
| 0x5 | POOL_CONFIG | Pooling config (size, stride, type, channels) |
| 0x6 | ACTIVATION_CONFIG | Activation type & tensor size |
| 0x7 | ACTIVATION_ALPHA | Alpha parameter (LeakyReLU, ELU) |
| 0x8 | BATCH_SIZE | Batch size |
| 0x9–0xC | TEMP_0–3 | Temporary storage |
| 0xD | DEBUG_CYCLES | Cycle counter (read-only) |
| 0xE | DEBUG_STATUS | Debug status code (read-only) |
| 0xF | READ_ONLY | Magic number 0xCAB00D1E (read-only) |

---

## Key Generics

- **DATA_WIDTH:** 16 bits (fixed-point Q8.8)  
- **FRAC_WIDTH:** 8 bits (fractional part)  
- **MAX_BATCH_SIZE:** 4  
- **MAX_KERNEL_SIZE:** 5  
- **MAX_POOL_SIZE:** 8  
- **MAX_CHANNELS:** 16  
- **MAX_TENSOR_SIZE:** 4096  
- **RAM_SIZE:** 23000 × 32-bit words (≈92 KB)

---

## Typical Operation Sequence

1. **HPS** writes configuration to registers (0x0–0x8)  
2. **HPS** streams input/weight/bias data via Avalon ST Sink  
3. **HPS** writes start command to CMD register  
4. **Accelerator** processes via selected block (conv/pool/activation)  
5. **Accelerator** signals DONE in STATUS register  
6. **HPS** reads results via Avalon ST Source  
7. **HPS** reads debug info from DEBUG_CYCLES, DEBUG_STATUS
