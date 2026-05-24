#!/usr/bin/env python3
"""
Convert 96x16 sprite sheet PNGs to DS 4bpp tile data (.bin) and palette (.bin).

Each PNG is a 96x16 strip of 6 frames (16x16 each).
Each 16x16 frame = 4 tiles (2x2 of 8x8) in 1D-32 mapping order.
Total: 6 frames * 4 tiles = 24 tiles * 32 bytes = 768 bytes per sprite.

Palette: 16 colors * 2 bytes (RGB555 LE) = 32 bytes.
"""

import os
import struct
from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
SPRITES_IN = os.path.join(PROJECT_DIR, "graphics", "sprites")
SPRITES_OUT = os.path.join(PROJECT_DIR, "arm9", "data", "sprites")

# Sprite ordering: determines classId mapping
# player=0, then red_1..red_6, blue_1..blue_6, green_1..green_6,
# yellow_1..yellow_6, purple_1..purple_6, cyan_1..cyan_6
COLORS = ["red", "blue", "green", "yellow", "purple", "cyan"]
SPRITE_ORDER = ["player"] + [f"{c}_{i}" for c in COLORS for i in range(1, 7)]

# Palette ordering: one palette per color group + player
# 0=red, 1=blue, 2=green, 3=yellow, 4=purple, 5=cyan, 6=player
PALETTE_ORDER = COLORS + ["player"]


def rgb888_to_rgb555(r, g, b):
    """Convert RGB888 to DS RGB555 (u16 LE)."""
    return (r // 8) | ((g // 8) << 5) | ((b // 8) << 10)


def extract_tile_4bpp(img, tile_x, tile_y):
    """Extract one 8x8 tile as 32 bytes of 4bpp packed data.

    Each byte = low nibble (left pixel) | high nibble (right pixel).
    """
    data = bytearray(32)
    for row in range(8):
        for col_pair in range(4):
            px = tile_x * 8 + col_pair * 2
            py = tile_y * 8 + row
            left = img.getpixel((px, py)) & 0x0F
            right = img.getpixel((px + 1, py)) & 0x0F
            data[row * 4 + col_pair] = left | (right << 4)
    return bytes(data)


def convert_sprite(png_path, bin_path):
    """Convert a 96x16 sprite sheet PNG to raw DS tile data."""
    img = Image.open(png_path)
    assert img.size == (96, 16), f"Expected 96x16, got {img.size}"
    assert img.mode == "P", f"Expected palette mode, got {img.mode}"

    tile_data = bytearray()

    # 6 frames, each 16x16
    for frame in range(6):
        frame_x = frame * 16
        # 2x2 tiles in 1D-32 order: top-left, top-right, bottom-left, bottom-right
        for ty in range(2):
            for tx in range(2):
                tile = extract_tile_4bpp(img, (frame_x // 8) + tx, ty)
                tile_data.extend(tile)

    with open(bin_path, "wb") as f:
        f.write(tile_data)

    return len(tile_data)


def convert_palette(png_path, pal_path):
    """Extract 16-color palette from PNG and write as DS RGB555."""
    img = Image.open(png_path)
    raw_pal = img.getpalette()  # flat list of R,G,B values

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
    os.makedirs(SPRITES_OUT, exist_ok=True)

    print(f"Converting sprites from {SPRITES_IN}")
    print(f"Output directory: {SPRITES_OUT}")
    print()

    # Convert all sprite tile data
    for name in SPRITE_ORDER:
        png_path = os.path.join(SPRITES_IN, f"{name}.png")
        bin_path = os.path.join(SPRITES_OUT, f"spr_{name}.bin")

        if not os.path.exists(png_path):
            print(f"  WARNING: {png_path} not found, skipping")
            continue

        size = convert_sprite(png_path, bin_path)
        print(f"  {name}.png -> spr_{name}.bin ({size} bytes)")

    print()

    # Convert palettes (one per color group, using _1.png as representative)
    for color in PALETTE_ORDER:
        if color == "player":
            png_path = os.path.join(SPRITES_IN, "player.png")
        else:
            png_path = os.path.join(SPRITES_IN, f"{color}_1.png")

        pal_path = os.path.join(SPRITES_OUT, f"pal_{color}.bin")

        if not os.path.exists(png_path):
            print(f"  WARNING: {png_path} not found, skipping palette")
            continue

        size = convert_palette(png_path, pal_path)
        print(f"  {color} palette -> pal_{color}.bin ({size} bytes)")

    print()
    print("Done!")


if __name__ == "__main__":
    main()
