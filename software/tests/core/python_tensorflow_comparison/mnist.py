import numpy as np
import tensorflow as tf
from tensorflow.keras.models import load_model
import json

# -------------------------------
# 1. Učitaj pretreniranu MNIST mrežu
# -------------------------------
# Pretpostavimo da postoji fajl 'mnist_cnn.h5' sa sačuvanim težinama
model = load_model("best_model.h5", compile=False)
model.summary()

# -------------------------------
# 2. Dummy ulazni primjer (prvi uzorak iz test skupa)
# -------------------------------
mnist = tf.keras.datasets.mnist
(_, _), (x_test, y_test) = mnist.load_data()
print("Expected label:", y_test[0])  # Prvi testni uzorak
x_input = x_test[0:1].astype(np.float32) / 255.0  # normalizacija
x_input = np.expand_dims(x_input, axis=-1)        # NHWC: (1, 28, 28, 1)

np.save("input.npy", x_input)
x_input.tofile("input.bin")

# -------------------------------
# 3. Forward pass
# -------------------------------
y_output = model(x_input, training=False).numpy()
print(y_output)
np.save("output_ref.npy", y_output)
y_output.tofile("output_ref.bin")
print("Input shape:", x_input.shape)
print("Output shape:", y_output.shape)

# -------------------------------
# 4. Sačuvaj težine + manifest
# -------------------------------
params = {}
manifest = []

for layer in model.layers:
    layer_entry = {
        "name": layer.name,
        "type": layer.__class__.__name__,
        "weights": None,
        "bias": None,
        "activation": getattr(layer, "activation", None).__name__ if hasattr(layer, "activation") else None,
        "params": {}
    }

    # Specifični parametri po tipu sloja
    if isinstance(layer, tf.keras.layers.Conv2D):
        layer_entry["params"] = {
            "kernel_size": layer.kernel_size,
            "strides": layer.strides,
            "padding": layer.padding,
            "dilation_rate": layer.dilation_rate
        }
    elif isinstance(layer, tf.keras.layers.MaxPooling2D) or isinstance(layer, tf.keras.layers.AveragePooling2D):
        layer_entry["params"] = {
            "pool_size": layer.pool_size,
            "strides": layer.strides,
            "padding": layer.padding
        }
    elif isinstance(layer, tf.keras.layers.Dense):
        layer_entry["params"] = {
            "units": layer.units
        }
    elif isinstance(layer, tf.keras.layers.Flatten):
        layer_entry["params"] = {}

    # Težine i bias
    weights = layer.get_weights()
    if weights:
        if len(weights) == 1:
            W = weights[0]
            params[layer.name + "_W"] = W
            layer_entry["weights"] = layer.name + "_W"
            np.save(f"{layer.name}_W.npy", W)
            W.tofile(f"{layer.name}_W.bin")
        elif len(weights) == 2:
            W, b = weights
            params[layer.name + "_W"] = W
            params[layer.name + "_b"] = b
            layer_entry["weights"] = layer.name + "_W"
            layer_entry["bias"] = layer.name + "_b"
            np.save(f"{layer.name}_W.npy", W)
            np.save(f"{layer.name}_b.npy", b)
            W.tofile(f"{layer.name}_W.bin")
            b.tofile(f"{layer.name}_b.bin")

    manifest.append(layer_entry)

# Sačuvaj manifest
with open("manifest.json", "w") as f:
    json.dump(manifest, f, indent=2)

# Sve težine u npz
np.savez("mnist_cnn_weights.npz", **params)

print("Parametri sačuvani u mnist_cnn_weights.npz")
print("Manifest sa redosledom i tipovima slojeva sačuvan u manifest.json")
