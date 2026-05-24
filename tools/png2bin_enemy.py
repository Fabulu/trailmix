#!/usr/bin/env python3
"""
Convert enemy sprite PNGs to DS-compatible LINEAR 4bpp binary data.

Unlike companion sprites (tiled for OAM hardware), enemy sprites use
linear pixel data because they are software-blitted to the bitmap
framebuffer.

Each enemy PNG is a 2-frame horizontal strip: [frame1 | frame2]
Sizes:
  8px  -> 16x8  PNG (small "_s")
  16px -> 32x16 PNG (medium "_m")
  24px -> 48x24 PNG (large "_l")
  32px -> 64x32 PNG (boss)

Output format:
  .bin     - Linear 4bpp pixel data. Two pixels per byte:
             low nibble = left pixel, high nibble = right pixel.
             Frame 1 then frame 2, each row-by-row top to bottom.
  .pal.bin - 16-color palette, each color u16 RGB555 little-endian.
             32 bytes total. Index 0 = transparent.
"""

import os
import struct
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
ENEMIES_IN = os.path.join(PROJECT_DIR, "graphics", "enemies")
ENEMIES_OUT = os.path.join(PROJECT_DIR, "arm9", "data", "enemies")

# Regular enemy types (each has _s, _m, _l variants)
REGULAR_ENEMIES = [
    "grunt", "charger", "brute", "spitter", "sniper", "splitter",
    "shield", "artillery", "nightmare", "ghost", "drone", "bomber",
    "medic", "anchor", "trapper", "hexer", "hive",
]

# Boss enemy types (single 32px size, 2 frames)
BOSS_ENEMIES = [
    "boss_sentinel", "boss_dreadnought", "boss_leviathan", "boss_nightmare",
]

# Large bosses (64px, variable frame count)
LARGE_BOSS_ENEMIES = [
    ("boss_apothecary", 64, 8),  # (name, frame_size, frame_count)
]

# Size suffix -> (frame_width, frame_height, png_width, png_height)
SIZE_MAP = {
    "_s": (8, 8, 16, 8),
    "_m": (16, 16, 32, 16),
    "_l": (24, 24, 48, 24),
}

BOSS_SIZE = (32, 32, 64, 32)


def rgb888_to_rgb555(r, g, b):
    """Convert RGB888 to DS RGB555 (u16 LE)."""
    return (r // 8) | ((g // 8) << 5) | ((b // 8) << 10)


def extract_linear_frame_4bpp(img, frame_x, frame_w, frame_h):
    """Extract one frame as linear 4bpp packed data.

    Each byte = low nibble (left pixel) | high nibble (right pixel).
    Row by row, top to bottom.
    """
    data = bytearray(frame_w * frame_h // 2)
    idx = 0
    for y in range(frame_h):
        for x in range(0, frame_w, 2):
            left = img.getpixel((frame_x + x, y)) & 0x0F
            right = img.getpixel((frame_x + x + 1, y)) & 0x0F
            data[idx] = left | (right << 4)
            idx += 1
    return bytes(data)


def convert_enemy(png_path, bin_path, frame_w, frame_h, num_frames=2):
    """Convert an N-frame enemy sprite strip to linear 4bpp data."""
    img = Image.open(png_path)
    expected_w = frame_w * num_frames
    expected_h = frame_h
    assert img.mode == "P", f"Expected palette mode ('P'), got '{img.mode}' in {png_path}"
    assert img.size == (expected_w, expected_h), (
        f"Expected {expected_w}x{expected_h}, got {img.size[0]}x{img.size[1]} in {png_path}"
    )

    pixel_data = bytearray()
    for f in range(num_frames):
        pixel_data.extend(extract_linear_frame_4bpp(img, f * frame_w, frame_w, frame_h))

    with open(bin_path, "wb") as fout:
        fout.write(pixel_data)

    return len(pixel_data)


def convert_palette(png_path, pal_path):
    """Extract 16-color palette from PNG and write as DS RGB555."""
    img = Image.open(png_path)
    raw_pal = img.getpalette()  # flat list of R, G, B values

    pal_data = bytearray()
    num_colors = min(16, len(raw_pal) // 3)
    for i in range(16):
        if i < num_colors:
            r = raw_pal[i * 3]
            g = raw_pal[i * 3 + 1]
            b = raw_pal[i * 3 + 2]
            rgb555 = rgb888_to_rgb555(r, g, b)
        else:
            rgb555 = 0
        pal_data.extend(struct.pack("<H", rgb555))

    with open(pal_path, "wb") as f:
        f.write(pal_data)

    return len(pal_data)


def main():
    os.makedirs(ENEMIES_OUT, exist_ok=True)

    print(f"Converting enemy sprites from {ENEMIES_IN}")
    print(f"Output directory: {ENEMIES_OUT}")
    print()

    converted = 0
    skipped = 0

    # Regular enemies: 2 frames each, 3 sizes
    for enemy in REGULAR_ENEMIES:
        for suffix, (fw, fh, pw, ph) in SIZE_MAP.items():
            name = f"{enemy}{suffix}"
            png_path = os.path.join(ENEMIES_IN, f"{name}.png")
            bin_path = os.path.join(ENEMIES_OUT, f"e_{name}.bin")
            pal_path = os.path.join(ENEMIES_OUT, f"e_{name}_pal.bin")
            if not os.path.exists(png_path):
                print(f"  WARNING: {name}.png not found, skipping")
                skipped += 1
                continue
            size = convert_enemy(png_path, bin_path, fw, fh, 2)
            pal_size = convert_palette(png_path, pal_path)
            print(f"  {name}.png -> e_{name}.bin ({size}B) + pal ({pal_size}B)")
            converted += 1

    # Standard bosses: 32px, 2 frames
    for boss in BOSS_ENEMIES:
        png_path = os.path.join(ENEMIES_IN, f"{boss}.png")
        bin_path = os.path.join(ENEMIES_OUT, f"e_{boss}.bin")
        pal_path = os.path.join(ENEMIES_OUT, f"e_{boss}_pal.bin")
        if not os.path.exists(png_path):
            print(f"  WARNING: {boss}.png not found, skipping")
            skipped += 1
            continue
        size = convert_enemy(png_path, bin_path, 32, 32, 2)
        pal_size = convert_palette(png_path, pal_path)
        print(f"  {boss}.png -> e_{boss}.bin ({size}B) + pal ({pal_size}B)")
        converted += 1

    # Large bosses: variable size and frame count
    for boss_name, frame_px, num_frames in LARGE_BOSS_ENEMIES:
        png_path = os.path.join(ENEMIES_IN, f"{boss_name}.png")
        bin_path = os.path.join(ENEMIES_OUT, f"e_{boss_name}.bin")
        pal_path = os.path.join(ENEMIES_OUT, f"e_{boss_name}_pal.bin")
        if not os.path.exists(png_path):
            print(f"  WARNING: {boss_name}.png not found, skipping")
            skipped += 1
            continue
        size = convert_enemy(png_path, bin_path, frame_px, frame_px, num_frames)
        pal_size = convert_palette(png_path, pal_path)
        print(f"  {boss_name}.png -> e_{boss_name}.bin ({size}B, {num_frames}f) + pal ({pal_size}B)")
        converted += 1

    print()
    print(f"Done! Converted {converted}, skipped {skipped}.")


if __name__ == "__main__":
    main()
