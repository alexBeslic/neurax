import numpy as np
import tensorflow as tf

# --------- Parametri ---------
batch = 1
channels = 2
H = 6
W = 6
pool_size = 2
stride = 2

print(f"Input shape: ({batch}, {H}, {W}, {channels})")
print(f"Output shape: ({batch}, {H//stride}, {W//stride}, {channels})")

# --------- 1. Kreiranje ulaznog tensora (NHWC) ---------
input_tensor = np.zeros((H, W, channels), dtype=np.float32)

# Kanal 0: jednostavan niz 0,1,2,...,35
for i in range(H):
    for j in range(W):
        input_tensor[i, j, 0] = i * W + j

# Kanal 1: konstantne vrednosti za lakše testiranje
for i in range(H):
    for j in range(W):
        input_tensor[i, j, 1] = 100 + i * W + j

# Dodaj batch dimenziju
input_nhwc = input_tensor[np.newaxis, :, :, :]  # (1, H, W, channels)
input_nhwc.tofile("input.bin")

print("Input tensor kanal 0:")
print(input_tensor[:, :, 0])
print("\nInput tensor kanal 1:")
print(input_tensor[:, :, 1])

# --------- 2. Max pooling ---------
input_tf = tf.constant(input_nhwc, dtype=tf.float32)
output_tf = tf.nn.max_pool2d(
    input_tf,
    ksize=[1, pool_size, pool_size, 1],
    strides=[1, stride, stride, 1],
    padding="VALID"
)

output_nhwc = output_tf.numpy()
output_nhwc.tofile("output.bin")

print("\nMax pooling output:")
print("Kanal 0:")
print(output_nhwc[0, :, :, 0])
print("Kanal 1:")
print(output_nhwc[0, :, :, 1])

# --------- 3. Manual verification ---------
print("\nManual verification za prvi 2x2 blok:")
print("Input blok [0:2, 0:2] kanal 0:")
print(input_tensor[0:2, 0:2, 0])
print(f"Expected max: {np.max(input_tensor[0:2, 0:2, 0])}")
print("\nInput blok [0:2, 0:2] kanal 1:")
print(input_tensor[0:2, 0:2, 1])
print(f"Expected max: {np.max(input_tensor[0:2, 0:2, 1])}")

print(f"\nFajlovi kreirani:")
print(f"- input_pooling.bin: {input_nhwc.shape}")
print(f"- output_ref_max.bin: {output_nhwc.shape}")