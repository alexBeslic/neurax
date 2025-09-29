import numpy as np
import tensorflow as tf

# --------- Parametri ---------
batch = 1
channels = 2
H = 6
W = 6

print(f"Input shape: ({batch}, {H}, {W}, {channels})")

# --------- 1. Kreiranje ulaznog tensora (NHWC) ---------
input_tensor = np.zeros((H, W, channels), dtype=np.float32)

# Kanal 0: vrednosti -3..32 to test negative values for ReLU
for i in range(H):
    for j in range(W):
        input_tensor[i, j, 0] = i * W + j - 3

# Kanal 1: small fractional values
for i in range(H):
    for j in range(W):
        input_tensor[i, j, 1] = (100 + i * W + j) / 10.0 - 10.0

# Dodaj batch dimenziju
input_nhwc = input_tensor[np.newaxis, :, :, :]  # (1, H, W, channels)
input_nhwc.tofile("input.bin")

print("Input tensor kanal 0:")
print(input_tensor[:, :, 0])
print("\nInput tensor kanal 1:")
print(input_tensor[:, :, 1])

# --------- 2. Activation (ReLU) ---------
input_tf = tf.constant(input_nhwc, dtype=tf.float32)
output_tf = tf.nn.relu(input_tf)

output_nhwc = output_tf.numpy()
output_nhwc.tofile("output.bin")

print("\nReLU output:")
print("Kanal 0:")
print(output_nhwc[0, :, :, 0])
print("Kanal 1:")
print(output_nhwc[0, :, :, 1])

print(f"\nFajlovi kreirani:")
print(f"- input_activation.bin: {input_nhwc.shape}")
print(f"- output_ref_activation.bin: {output_nhwc.shape}")
