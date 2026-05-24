# Trail Mix

A roguelike auto-chess shooter for the Nintendo DS. You collect weird little guys who follow you around and shoot things.

## What is this?

You run around a top-down arena dodging enemies. Between waves you hit the shop and buy companions - 36 different classes across 6 colors, from Pyromancers to Plague Doctors to whatever a "Circuit Hacker" is supposed to be. They trail behind you in a line and handle the shooting while you handle the not-dying. Buy three of the same guy and they smoosh into a stronger version.

The whole thing runs on a Nintendo DS. Two screens, 256x192 each, and somehow it all fits.

## Features

- 36 companion classes, 6 color factions, 3 upgrade tiers per class
- Color synergies that scale up the more of one color you stack (2/3/4/5 thresholds)
- 15 perks - free pick after bosses, or pay through the nose in the shop
- 12 enemy types in small/medium/large, plus 4 bosses that will ruin your run
- Interest on unspent gold, because even in a fantasy shooter you can't escape economics
- English and German

## Screenshots

<!-- TODO: gameplay -->

<!-- TODO: shop screen -->

<!-- TODO: boss fight -->

<!-- TODO: synergies going off -->

## Controls

| Input | What it does |
|-------|-------------|
| D-Pad | Move |
| R | Dash (you're invincible during it, abuse this) |
| Touch | Buy, sell, pick perks, manage your guys |

## Building

Needs [devkitPro](https://devkitpro.org/) with the NDS toolchain. Then:

```
make
```

Out comes `trailmix.nds`. Run it on hardware with a flashcart or in [melonDS](https://melonds.kuribo64.net/).

## License

MIT
