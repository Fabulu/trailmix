# Pixel Art Sprite Brief -- DS Pill Game

## Context

Top-down auto-chess roguelike shooter for Nintendo DS. 256x192 resolution, 4bpp sprites. Everything is geometric, simple, high-contrast. Characters are pill/capsule-shaped.

## Deliverables

**25 sprite sheets total:** 1 player pill + 24 companion pills (4 colors x 6 classes).

All sprites are 16x16 pixels. Each sheet is 64x16: four 16x16 frames laid out horizontally in this order: idle-1, idle-2, upgraded-idle-1, upgraded-idle-2.

## Player Pill

White/silver capsule, vertically oriented (taller than wide). 1px dark outline (#1a1a2e). Body gradient from #e8e8f0 (highlight) to #b0b0c0 (shadow). Add a tiny face: two 1px dot eyes in black. The player pill should look slightly more "leader-like" than companions -- give it a small white cross/plus on its forehead (1px lines, 3px tall). Idle animation: frame 1 is neutral, frame 2 shifts the body 1px up (subtle bob). No upgraded variant needed -- frames 3-4 can duplicate frames 1-2.

Filename: `pill_player.png`

## Companion Pills (24 total)

Each companion is a capsule/oval in its color family with a 1px dark outline. The capsule body fills roughly 10x14 of the 16x16 tile (centered, 3px padding on sides, 1px top/bottom). Two 1px dot eyes in a darker shade of the pill's color. Each class has a TINY 4x4 to 5x5 icon overlaid on the lower half of the capsule to distinguish it.

**Idle animation:** Frame 1 neutral, frame 2 body shifts 1px up.
**Upgraded variant:** Same sprite but add a 1px bright glow outline (white or pale tint) around the capsule and a single sparkle pixel in the top-right corner.

### Color Palettes (16 colors each, index 0 = transparent)

**RED palette:**
0: transparent, 1: #1a0a0a (outline), 2: #ff3344 (body), 3: #cc1122 (body shadow), 4: #ff6677 (body highlight), 5: #ffaaaa (specular), 6: #880011 (dark accent), 7: #ffffff (eyes/sparkle), 8: #ffdd44 (icon accent), 9: #222222 (icon outline), 10: #ff8899 (glow), 11-15: reserved

**BLUE palette:**
0: transparent, 1: #0a0a1a, 2: #3366ff, 3: #1144cc, 4: #6699ff, 5: #aaccff, 6: #002288, 7: #ffffff, 8: #44ddff, 9: #222222, 10: #88bbff, 11-15: reserved

**GREEN palette:**
0: transparent, 1: #0a1a0a, 2: #33cc44, 3: #119922, 4: #66ee77, 5: #aaffaa, 6: #006611, 7: #ffffff, 8: #ddff44, 9: #222222, 10: #88ff99, 11-15: reserved

**YELLOW palette:**
0: transparent, 1: #1a1a0a, 2: #ffcc22, 3: #cc9900, 4: #ffdd66, 5: #ffeeaa, 6: #886600, 7: #ffffff, 8: #ff8844, 9: #222222, 10: #ffee88, 11-15: reserved

**PLAYER palette:**
0: transparent, 1: #1a1a2e, 2: #e8e8f0, 3: #b0b0c0, 4: #d0d0dd, 5: #ffffff, 6: #707088, 7: #222222, 8: #44ccff, 9-15: reserved

### Class Icons (overlaid on lower capsule body)

| # | Class | Icon (4x4 approx) |
|---|-------|--------------------|
| R1 | Gunner | Tiny bullet/arrow pointing right |
| R2 | Pyromaniac | Flame (3 pixels tall) |
| R3 | Shotgunner | Three-dot spread |
| R4 | Berserker | Jagged fang/zigzag |
| R5 | Detonator | Circle with fuse (bomb) |
| R6 | Executioner | Tiny scythe blade |
| B1 | Warden | Small shield (3x4 trapezoid) |
| B2 | Channeler | Lightning bolt |
| B3 | Frost Archer | Snowflake (4px cross+diagonals) |
| B4 | Orbiter | Two dots circling |
| B5 | Chronomancer | Hourglass |
| B6 | Nullifier | X mark |
| G1 | Thornshot | Thorn/spike |
| G2 | Seedling | Sprout with two leaves |
| G3 | Apothecary | Plus/cross (medical) |
| G4 | Vinecaller | Curling vine tendril |
| G5 | Sporewitch | Mushroom cap |
| G6 | Lifeshaper | Heart |
| Y1 | Gambler | Dice face |
| Y2 | Scavenger | Coin (circle with line) |
| Y3 | Jester | Jester hat point |
| Y4 | Mimic | Question mark |
| Y5 | Alchemist | Flask/beaker |
| Y6 | Trickster | Star (4-point) |

## Style Rules

- **Binding of Isaac items** at half the size: bold, readable, geometric.
- 1px dark border on ALL capsule bodies, no exceptions.
- High contrast: icon must read clearly against the pill body color.
- No anti-aliasing. No dithering. Clean pixel art only.
- Shadows go bottom-right. Highlights go top-left.

## File Output

Directory: `graphics/sprites/`

Naming: `pill_player.png`, `pill_red_1.png` through `pill_red_6.png`, `pill_blue_1.png` through `pill_blue_6.png`, `pill_green_1.png` through `pill_green_6.png`, `pill_yellow_1.png` through `pill_yellow_6.png`.

Each PNG is 64x16 (4 frames horizontal). Indexed color, 4bpp. Transparent background.

---

## Enemy Sprite Animation Spec

All enemies use **2 frames (32x16 PNG for 16x16 sprites)**. No enemy needs more than 2 frames in its sheet. Special states are handled entirely in code — not through extra frames.

### Standard 2-frame idle

| Enemy | Frame 1 | Frame 2 | Loop feel |
|-------|---------|---------|-----------|
| Skitter | legs spread | legs tucked | scurry cycle |
| Driftling | body neutral | body arched | undulating swim |
| Orbiter | ring at 0° | ring at 45° | continuous rotation |
| Golem | upright | leaning 1px left | lumbering sway |

### 2-frame with attack state (ranged)

These enemies reuse their 2 frames as idle and shoot states. The engine selects which frame is displayed based on attack phase — no extra art needed.

| Enemy | Frame 1 (idle) | Frame 2 (attack) |
|-------|---------------|-----------------|
| Spewer | mouth closed | mouth open |
| Needler | dim sensor | sensor glowing |
| Mortar | upright | barrel tilted / recoil |

### Special states (no extra frames)

| Enemy | State | Method |
|-------|-------|--------|
| Lancer | charging | increase idle toggle speed in code |
| Cinder | about to explode | swap to a brighter palette variant |
| Cleft | death split | particle effect — two half-sprites fly apart |
| Pavise | facing direction | horizontal flip of a single right-facing sprite |
| Fang | mirror symmetry | right half drawn; left half is a horizontal flip |

### File output

Enemy sheets follow the same format as companion pills: 32x16 PNG (2 frames), indexed color, 4bpp, transparent background.

Directory: `graphics/sprites/enemies/`

Naming: `enemy_skitter.png`, `enemy_driftling.png`, `enemy_orbiter.png`, `enemy_golem.png`, `enemy_spewer.png`, `enemy_needler.png`, `enemy_mortar.png`, `enemy_lancer.png`, `enemy_cinder.png`, `enemy_cleft.png`, `enemy_pavise.png`, `enemy_fang.png`.
