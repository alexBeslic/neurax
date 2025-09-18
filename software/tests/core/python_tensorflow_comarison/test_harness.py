import subprocess
import sys
import os

# -------------------
# Python generatori i odgovarajući parametri za C++ validator
# Format: generator_script : (batch, H, W, in_channels, out_channels, kernel_size)
# -------------------
GENERATORS = {
    "conv2d.py"             : (1, 5, 5, 1, 1, 3),           # single-channel test
    "conv2d_multichannel.py": (1, 5, 5, 3, 2, 3)             # multi-channel test
}

# C++ validator (full path)
CPP_CONV2D_VALIDATOR = "../../../../build/software/tests/core/python_tensorflow_comarison/core_test_conv2d"

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

    for generator, params in GENERATORS.items():
        batch, H, W, in_ch, out_ch, kernel_size = params
        print(f"\n=== Test sa generatorom: {generator} ===")
        print(f"Validator parametri: batch={batch}, H={H}, W={W}, in_channels={in_ch}, out_channels={out_ch}, kernel_size={kernel_size}")

        # 1. Pokreni generator (fiksni parametri unutar skripte)
        run_cmd(
            ["python3", generator],
            desc=f"Generisanje fajlova sa {generator}"
        )

        # 2. Pokreni C++ validator sa pripadajućim parametrima
        run_cmd(
            [CPP_CONV2D_VALIDATOR,
             str(batch), str(H), str(W),
             str(in_ch), str(out_ch),
             str(kernel_size)],
            desc=f"Validacija C++ programom nakon {generator}"
        )

    print("\n✅ Svi testovi završeni uspješno!")

if __name__ == "__main__":
    # Očekivani direktorijum (relativno ili apsolutno)
    EXPECTED_DIR = "software/tests/core/python_tensorflow_comarison"

    # Trenutni radni direktorijum
    cwd = os.getcwd()
    if EXPECTED_DIR not in cwd:
        print(f"❌ Pokrenite ovaj skript iz direktorijuma koji sadrži '{EXPECTED_DIR}' u putanji.")
        sys.exit(1)
    main()
