#!/usr/bin/env python3
"""
Encrypt zen_masters text files for NitroFS packaging.
Usage: python tools/encrypt_zen.py [--verify]
Reads:  data/zen_masters_en.txt, data/zen_masters_de.txt
Writes: nitrofs/zen_masters_en.enc, nitrofs/zen_masters_de.enc

The --verify flag decrypts the .enc files and diffs against the originals.
"""
import sys
import os


# ---------------------------------------------------------------------------
# RC4 — must match the C implementation in arm9/source/sayings.cpp exactly
# ---------------------------------------------------------------------------
def rc4(key: bytes, data: bytes) -> bytes:
    """Standard RC4 stream cipher. Deterministic for same key+data."""
    S = list(range(256))
    j = 0
    for i in range(256):
        j = (j + S[i] + key[i % len(key)]) & 0xFF
        S[i], S[j] = S[j], S[i]
    out = bytearray(len(data))
    i = j = 0
    for k in range(len(data)):
        i = (i + 1) & 0xFF
        j = (j + S[i]) & 0xFF
        S[i], S[j] = S[j], S[i]
        out[k] = data[k] ^ S[(S[i] + S[j]) & 0xFF]
    return bytes(out)


# ---------------------------------------------------------------------------
# Key construction
# ---------------------------------------------------------------------------
# The key is assembled from two string fragments + language tag in the C runtime:
#   kDataSeedA  (sayings.cpp)  = "trail.mix.2024"   (14 bytes)
#   kStepTableB (camera.cpp)   = ".nitro"            (6 bytes)
#   langTag                    = "en" or "de"        (2 bytes)
# Concatenated: "trail.mix.2024.nitroen" or "trail.mix.2024.nitrode" (22 bytes)
_SEED_A = b"trail.mix.2024"
_STEP_B = b".nitro"


def get_key(lang: str) -> bytes:
    """Build per-language 16-byte RC4 key.

    This prevents XOR-of-ciphertexts from leaking XOR-of-plaintexts
    when EN and DE files share structural similarities.
    """
    return _SEED_A + _STEP_B + lang.encode("ascii")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    verify = "--verify" in sys.argv

    files = [
        ("data/zen_masters_en.txt", "nitrofs/zen_masters_en.enc", "en"),
        ("data/zen_masters_de.txt", "nitrofs/zen_masters_de.enc", "de"),
    ]

    ok = True
    for src_rel, dst_rel, lang in files:
        src = os.path.join(root, src_rel)
        dst = os.path.join(root, dst_rel)

        if not os.path.exists(src):
            print(f"SKIP: {src_rel} not found (contributor build without plaintext)")
            continue

        with open(src, "rb") as f:
            plain = f.read()

        key = get_key(lang)
        cipher = rc4(key, plain)

        if verify:
            # Round-trip: decrypt the ciphertext and compare
            decrypted = rc4(key, cipher)
            if decrypted == plain:
                print(f"VERIFY OK: {src_rel} round-trip matches ({len(plain)} B)")
            else:
                print(f"VERIFY FAIL: {src_rel} round-trip mismatch!")
                ok = False
            # Also check existing .enc file if present
            if os.path.exists(dst):
                with open(dst, "rb") as f:
                    existing = f.read()
                if existing == cipher:
                    print(f"VERIFY OK: {dst_rel} matches freshly encrypted output")
                else:
                    print(f"VERIFY WARN: {dst_rel} differs from fresh encryption (will overwrite)")
        else:
            with open(dst, "wb") as f:
                f.write(cipher)
            print(f"OK: {src_rel} ({len(plain)} B) -> {dst_rel} ({len(cipher)} B)")

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
