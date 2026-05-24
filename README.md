# Trail Mix

**A conga line of chaos on your Nintendo DS.**

## What is this?

Trail Mix is a roguelike auto-chess shooter for the Nintendo DS. You pilot a lone character through waves of increasingly rude enemies while collecting colorful companions who trail behind you in a wiggly conga line, blasting everything in sight. Buy guys, merge guys, watch guys do cool stuff. It's like herding cats, except the cats have flamethrowers.

## Features

- **36 companion classes** across 6 color factions — Pyromancers, Hexers, Tesla Coils, Plague Doctors, and 32 other weirdos
- **3-merge system** — three of the same companion smush together into a stronger version (auto-chess style)
- **6 color synergies** with 4 tiers each — stack companions of one color for increasingly absurd bonuses
- **15 perks** — free after boss waves, or blow your savings in the shop
- **12 enemy types** in three sizes, plus 4 bosses who definitely don't play fair
- **Interest economy** — unspent gold earns interest, rewarding the fiscally responsible
- **Full English & German localization** — Zerstörung in zwei Sprachen
- **Runs on real DS hardware** — as the homebrew gods intended

## Screenshots

<!-- TODO: Add screenshot of the conga line in action -->

<!-- TODO: Add screenshot of the shop/upgrade screen -->

<!-- TODO: Add screenshot of a boss fight -->

<!-- TODO: Add screenshot of synergy effects -->

## How to Play

| Input | Action |
|-------|--------|
| D-Pad | Move your character |
| Touch Screen | Pick upgrades, buy companions, manage your squad |

Your companions follow you automatically and shoot on their own. Your job is to dodge, position, and make smart choices between waves. Three identical companions merge into a powered-up version — plan your purchases accordingly.

## Building from Source

You'll need [devkitPro](https://devkitpro.org/) with the NDS development tools installed.

```bash
# Install devkitPro and nds-dev packages first, then:
make
```

The build produces a `.nds` ROM you can run on hardware (via flashcart) or in an emulator like [melonDS](https://melonds.kuribo64.net/).

## Credits

Built with devkitARM, libnds, and an unreasonable amount of enthusiasm.

Licensed under MIT. Do whatever you want with it — just don't blame us when you can't stop playing.
