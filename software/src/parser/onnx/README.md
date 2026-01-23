# ONNX Protocol Buffer Files

This directory should contain the official ONNX protocol buffer definition files.

## Required Files

1. **onnx.proto** - Core ONNX definitions
2. **onnx-ml.proto** - ONNX ML extension definitions (recommended for full support)

## How to Obtain

Download the latest .proto files from the official ONNX repository:

```bash
# From the src/parser/onnx directory:
curl -O https://raw.githubusercontent.com/onnx/onnx/main/onnx/onnx.proto
curl -O https://raw.githubusercontent.com/onnx/onnx/main/onnx/onnx-ml.proto
```

Or manually download from:
- https://github.com/onnx/onnx/blob/main/onnx/onnx.proto
- https://github.com/onnx/onnx/blob/main/onnx/onnx-ml.proto

## Version Compatibility

The NEURAX ONNX parser is designed to work with ONNX opset versions 11-21.
Make sure to download compatible .proto files.

## Build Process

Once the .proto files are in place, CMake will automatically generate the C++
bindings (onnx.pb.h, onnx.pb.cc, onnx-ml.pb.h, onnx-ml.pb.cc) in the
`build/generated` directory during the build process.

## Cross-Compilation Notes

For DE1-SoC ARM targets:
- The parser uses static protobuf linking
- No runtime protobuf installation is required on the target
- Generated code is endian-safe for ARM

## Minimal .proto for Testing

If you only need basic functionality and don't want to download the full files,
you can create minimal stubs. However, for production use, always use the
official ONNX .proto files.
