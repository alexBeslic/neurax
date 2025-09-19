import numpy as np
import tensorflow as tf

# --------- Parametri ---------
batch = 1
in_channels = 1
out_channels = 1
H = 5
W = 5
kernel_size = 3

# --------- 1. Kreiranje ulaznog tensora (NHWC) ---------
# jednostavna test slika: 0,1,2,...,24
input_tensor = np.arange(H*W, dtype=np.float32).reshape(H, W)
input_nhwc = input_tensor[np.newaxis, :, :, np.newaxis]  # (1, H, W, 1)
input_nhwc.tofile("input.bin")

# --------- 2. Kreiranje blur kernela (HWIO) ---------
kernel = np.ones((kernel_size, kernel_size), dtype=np.float32) / (kernel_size * kernel_size)
weights_hwio = kernel[:, :, np.newaxis, np.newaxis]  # (3, 3, 1, 1)
weights_hwio.tofile("weights.bin")

# --------- 3. TensorFlow korelacija (conv2d) ---------
input_tf = tf.constant(input_nhwc, dtype=tf.float32)
kernel_tf = tf.constant(weights_hwio, dtype=tf.float32)

# strides: [1, stride_y, stride_x, 1], padding="SAME" isto kao scipy 'same'
output_tf = tf.nn.conv2d(input_tf, kernel_tf, strides=[1, 1, 1, 1], padding="SAME")

output_nhwc = output_tf.numpy()
output_nhwc.tofile("output_ref.bin")

print("Referentni output (NHWC):")
print(output_nhwc)

print("Binarni fajlovi kreirani:")
print(" - input.bin:", input_nhwc.shape)
print(" - weights.bin:", weights_hwio.shape)
print(" - output_ref.bin:", output_nhwc.shape)
