import numpy as np
import tensorflow as tf

# --------- Parametri ---------
batch = 1
in_channels = 3   # RGB
out_channels = 2  # dva izlazna kanala
H = 5
W = 5
kernel_size = 3

print(f"Test parametri:")
print(f"Input: {batch}x{H}x{W}x{in_channels} (NHWC)")
print(f"Weights: {kernel_size}x{kernel_size}x{in_channels}x{out_channels} (HWIO)")
print(f"Output: {batch}x{H}x{W}x{out_channels} (NHWC)")
print("-" * 50)

# --------- 1. Input tensor (NHWC) ---------
input_tensor = np.zeros((H, W, in_channels), dtype=np.float32)

# Kanal 0: linearni niz
input_tensor[:, :, 0] = np.arange(H*W, dtype=np.float32).reshape(H, W)

# Kanal 1: šahovska tabla
for i in range(H):
    for j in range(W):
        input_tensor[i, j, 1] = (i + j) % 2

# Kanal 2: gradijent po x
for i in range(H):
    for j in range(W):
        input_tensor[i, j, 2] = j

input_nhwc = input_tensor[np.newaxis, :, :, :]  # (1,H,W,C)
input_nhwc.tofile("input.bin")

print("Input tensor po kanalima:")
for c in range(in_channels):
    print(f"Kanal {c}:")
    print(input_tensor[:, :, c])
    print()

# --------- 2. Kernel (HWIO) ---------
weights = np.zeros((kernel_size, kernel_size, in_channels, out_channels), dtype=np.float32)

# Output kanal 0: blur
blur_kernel = np.ones((kernel_size, kernel_size), dtype=np.float32) / (kernel_size * kernel_size)
for in_ch in range(in_channels):
    weights[:, :, in_ch, 0] = blur_kernel

# Output kanal 1: edge detection (Sobel-y)
edge_kernel = np.array([[-1, -2, -1],
                        [ 0,  0,  0],
                        [ 1,  2,  1]], dtype=np.float32)
for in_ch in range(in_channels):
    weights[:, :, in_ch, 1] = edge_kernel * 0.1

weights.tofile("weights.bin")

# --------- 3. TensorFlow korelacija (conv2d) ---------
input_tf = tf.constant(input_nhwc)
kernel_tf = tf.constant(weights)

output_tf = tf.nn.conv2d(input_tf, kernel_tf, strides=[1,1,1,1], padding="SAME")
output_ref = output_tf.numpy()

output_ref.tofile("output_ref.bin")

print("Output po kanalima:")
for out_ch in range(out_channels):
    print(f"Output kanal {out_ch}:")
    print(output_ref[0, :, :, out_ch])
    print()

print("Binarni fajlovi kreirani:")
print(f" - input.bin: {input_nhwc.shape}")
print(f" - weights.bin: {weights.shape}")
print(f" - output_ref.bin: {output_ref.shape}")
