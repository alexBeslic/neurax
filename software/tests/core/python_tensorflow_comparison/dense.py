import numpy as np
import tensorflow as tf

# --------- Parametri ---------
batch = 2
in_features = 4
out_features = 3

print(f"Input shape: ({batch}, {in_features})")
print(f"Weights shape: ({in_features}, {out_features})")

# --------- 1. Kreiranje ulaznog tensora ---------
input_arr = np.arange(batch * in_features, dtype=np.float32).reshape(batch, in_features)
input_arr.tofile("input.bin")

# --------- 2. Kreiranje tezina i bias-a ---------
weights = np.arange(in_features * out_features, dtype=np.float32).reshape(in_features, out_features)
bias = np.arange(out_features, dtype=np.float32)
weights.tofile("weights.bin")
bias.tofile("bias.bin")

# --------- 3. TensorFlow referentni izlaz (matmul + bias) ---------
input_tf = tf.constant(input_arr, dtype=tf.float32)
weights_tf = tf.constant(weights, dtype=tf.float32)
bias_tf = tf.constant(bias, dtype=tf.float32)

output_tf = tf.matmul(input_tf, weights_tf) + bias_tf
output_np = output_tf.numpy()
output_np.tofile("output_ref.bin")

print("Referentni output:")
print(output_np)
print("Kreirani fajlovi: input.bin, weights.bin, bias.bin, output_ref.bin")
