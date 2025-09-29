import subprocess
import sys
import os

# -------------------
# Python generatori i odgovarajući parametri za C++ validator
# Format: generator_script : (batch, H, W, in_channels, out_channels, kernel_size)
# -------------------
GENERATORS = {
    "conv2d.py"             : ("core_test_conv2d",  (1, 5, 5, 1, 1, 3)),           # single-channel test
    "conv2d_multichannel.py": ("core_test_conv2d",  (1, 5, 5, 3, 2, 3)),           # multi-channel test
    "pooling.py"            : ("core_test_pool",    (1, 6, 6, 2, 0, 0)),           # pooling test (uses pool_size/stride inside C++ args)
    "activation.py"         : ("core_test_activation", (1, 6, 6, 2, 0, 0))            # activation test (ReLU)
}

# C++ validators base path (relative to this script)
CPP_VALIDATOR_BASE = "../../../../build/software/tests/core/python_tensorflow_comparison"

def run_cmd(cmd, desc=""):
    """Pokreće komandu i prijavljuje stdout/stderr. Prekida na grešku."""
    print(f"\n>>> Pokrećem: {desc if desc else ' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(result.stdout)
        if result.stderr.strip():
            print("stderr:", result.stderr)
    except subprocess.CalledProcessError as e:
        print("❌ Greška prilikom izvršavanja:", desc)
        print("stdout:", e.stdout)
        print("stderr:", e.stderr)
        sys.exit(1)

def main():
    print("Pokrećem harness test za sve generator skripte i C++ validator...\n")

    for generator, cfg in GENERATORS.items():
        validator_name, params = cfg
        print(f"\n=== Test sa generatorom: {generator} ===")

        # 1. Pokreni generator (fiksni parametri unutar skripte)
        run_cmd(
            ["python3", generator],
            desc=f"Generisanje fajlova sa {generator}"
        )

        # Build validator path
        validator_path = os.path.join(CPP_VALIDATOR_BASE, validator_name)

        # Prepare args depending on test type
        if validator_name == "core_test_conv2d":
            batch, H, W, in_ch, out_ch, kernel_size = params
            args = [validator_path, str(batch), str(H), str(W), str(in_ch), str(out_ch), str(kernel_size)]
        elif validator_name == "core_test_pool":
            # For pool test, existing C++ expects: batch H W in_channels pool_size stride
            batch, H, W, in_ch, _, _ = params
            pool_size = 2
            stride = 2
            args = [validator_path, str(batch), str(H), str(W), str(in_ch), str(pool_size), str(stride)]
        elif validator_name == "core_test_activation":
            # Activation test expects: batch H W in_channels
            batch, H, W, in_ch, _, _ = params
            args = [validator_path, str(batch), str(H), str(W), str(in_ch)]
        else:
            # Fallback - call with minimal args
            args = [validator_path]

        # 2. Pokreni C++ validator sa pripadajućim parametrima
        run_cmd(args, desc=f"Validacija C++ programom nakon {generator}")

    print("\n✅ Svi testovi završeni uspješno!")

if __name__ == "__main__":
    # Očekivani direktorijum (relativno ili apsolutno)
    EXPECTED_DIR = "software/tests/core/python_tensorflow_comparison"

    # Trenutni radni direktorijum
    cwd = os.getcwd()
    if EXPECTED_DIR not in cwd:
        print(f"❌ Pokrenite ovaj skript iz direktorijuma koji sadrži '{EXPECTED_DIR}' u putanji.")
        sys.exit(1)
    main()
