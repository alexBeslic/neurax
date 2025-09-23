import numpy as np
import tensorflow as tf
from tensorflow.keras import layers, models
import json

# -------------------------------
# 1. Definicija male mreže
# -------------------------------
def make_tiny_cnn(input_shape=(8, 8, 3), num_classes=2):
    model = models.Sequential(name="TinyCNN")
    model.add(layers.Conv2D(4, (3, 3), padding="same", activation="relu", input_shape=input_shape, name="conv1"))
    model.add(layers.MaxPooling2D((2, 2), name="pool1"))
    model.add(layers.Flatten(name="flatten"))
    model.add(layers.Dense(8, activation="relu", name="fc1"))
    model.add(layers.Dense(num_classes, activation="tanh", name="pred"))
    return model

model = make_tiny_cnn()
model.summary()

# -------------------------------
# 2. Dummy input (NHWC)
# -------------------------------
batch = 1
H, W, C = 8, 8, 3
x = np.arange(batch * H * W * C, dtype=np.float32).reshape((batch, H, W, C))
np.save("input.npy", x)
x.tofile("input.bin")

# -------------------------------
# 3. Forward pass
# -------------------------------
y = model(x, training=False).numpy()
np.save("output_ref.npy", y)
y.tofile("output_ref.bin")
print("Input shape:", x.shape)
print("Output shape:", y.shape)

# -------------------------------
# 4. Spremi težine + manifest
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
    if isinstance(layer, layers.Conv2D):
        layer_entry["params"] = {
            "kernel_size": layer.kernel_size,
            "strides": layer.strides,
            "padding": layer.padding,
            "dilation_rate": layer.dilation_rate
        }
    elif isinstance(layer, layers.MaxPooling2D) or isinstance(layer, layers.AveragePooling2D):
        layer_entry["params"] = {
            "pool_size": layer.pool_size,
            "strides": layer.strides,
            "padding": layer.padding
        }
    elif isinstance(layer, layers.Dense):
        layer_entry["params"] = {
            "units": layer.units
        }
    elif isinstance(layer, layers.Flatten):
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
np.savez("tinycnn_weights.npz", **params)

print("Parametri sačuvani u tinycnn_weights.npz")
print("Manifest sa redosledom i tipovima slojeva sačuvan u manifest.json")
