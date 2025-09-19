import numpy as np
import tensorflow as tf

# --------- Parametri ---------
batch = 1
in_channels = 4   # RGBA
out_channels = 4  # RGBA
H = 5
W = 5
kernel_size = 3

# --------- 1. Kreiranje ulaznog tensora (NHWC) ---------
input_tensor = np.zeros((H, W, in_channels), dtype=np.float32)

# R kanal: linearni niz
input_tensor[:, :, 0] = np.arange(H*W, dtype=np.float32).reshape(H, W)
# G kanal: šahovska tabla
for i in range(H):
    for j in range(W):
        input_tensor[i, j, 1] = (i + j) % 2
# B kanal: gradijent po x
for i in range(H):
    for j in range(W):
        input_tensor[i, j, 2] = j
# A kanal: konstantno 1
input_tensor[:, :, 3] = 1.0

input_nhwc = input_tensor[np.newaxis, :, :, :]  # (1,H,W,C)
input_nhwc.tofile("input.bin")

# --------- 2. Laplacian kernel (HWIO) ---------
weights = np.zeros((kernel_size, kernel_size, in_channels, out_channels), dtype=np.float32)

laplacian_kernel = np.array([[0,  1, 0],
                             [1, -4, 1],
                             [0,  1, 0]], dtype=np.float32)

# Popuni kernela: RGB = Laplacian, A = identitet
for out_ch in range(out_channels):
    for in_ch in range(in_channels):
        if in_ch < 3 and out_ch == in_ch:
            # RGB: Laplacian samo za odgovarajući kanal (R->R, G->G, B->B)
            weights[:, :, in_ch, out_ch] = laplacian_kernel
        elif in_ch == 3 and out_ch == 3:
            # A kanal = identitet
            weights[kernel_size//2, kernel_size//2, 3, 3] = 1.0
        else:
            # ostali elementi = 0
            weights[:, :, in_ch, out_ch] = 0.0

weights.tofile("weights.bin")

# --------- 3. TensorFlow korelacija (conv2d) ---------
input_tf = tf.constant(input_nhwc, dtype=tf.float32)
kernel_tf = tf.constant(weights, dtype=tf.float32)

output_tf = tf.nn.conv2d(input_tf, kernel_tf, strides=[1,1,1,1], padding="SAME")
output_ref = output_tf.numpy()
output_ref.tofile("output_ref.bin")

print("Output kanali (RGBA):")
for c in range(out_channels):
    print(f"Kanal {c}:")
    print(output_ref[0, :, :, c])
    print()

print("Binarni fajlovi kreirani:")
print(f" - input.bin: {input_nhwc.shape}")
print(f" - weights.bin: {weights.shape}")
print(f" - output_ref.bin: {output_ref.shape}")
