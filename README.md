# Trail Mix

A roguelike auto-chess shooter for the Nintendo DS. You collect weird little guys who follow you around and shoot things.

## What is this?

You run around a top-down arena dodging enemies. Between waves you hit the shop and buy companions - 36 different classes across 6 colors, from Pyromancers to Plague Doctors to whatever a "Circuit Hacker" is supposed to be. They trail behind you in a line and handle the shooting while you handle the not-dying. Buy three of the same guy and they smoosh into a stronger version.

The whole thing runs on a Nintendo DS. Two screens, 256x192 each, and somehow it all fits.

## Features

**36 companion classes** across 6 color factions (Red, Blue, Green, Yellow, Purple, Cyan), each with 3 upgrade tiers. Every class has a unique shot pattern - Gunners fire tracer rounds, Tesla Coils chain lightning between targets, Plague Doctors lob poison clouds, Overclockers fire faster and faster until they overheat and burst. Three of the same merge into the next tier.

**30 curated waves** with 5 randomly selected variants per wave, so every run plays different. Difficulty ramps from small grunt swarms to massive formations of shielded snipers, teleporting ghosts, healing medics, and mine-laying trappers. 17 enemy types in 3 sizes plus 5 bosses, each with real AI - the Sentinel orbits and fires spiral bullets, the Dreadnought charges and slams, the Leviathan summons adds while spitting at you.

**Wave 30 final boss** - The Apothecary, a 3-phase fight with phase transitions, minion spawns, poison zones, and a meltdown mode. Beat it for fireworks and the option to keep going in endless mode.

**30 perks** from the practical to the weird. Bullet Hell fires in all 4 directions. Shortcut lets you merge with only 2 copies instead of 3. Double or Nothing flips a coin on your wave gold. Wildcard lets any color merge with any color. Loan Shark gives you 80g now and bills you 10g per wave for 10 waves.

**6 color synergies** with 4 tiers each (24 effects). Stack 2/3/4/5 companions of one color for escalating bonuses - Red burns, Blue freezes, Green heals, Yellow prints money, Purple hexes, Cyan electrifies.

**Economy with interest** - unspent gold earns 25% interest per wave. Save up or spend it all, your call.

**Full English and German localization.** Every string, every button, every description.

**Runs on real DS hardware.** Tested on R4 flashcart.

## Controls

| Input | What it does |
|-------|-------------|
| D-Pad | Move |
| Touch | Move (virtual stick) |
| A/B/X/Y/R | Dash (invincible + damages enemies) |
| L | Slow walk (hold for precision) |

**In the shop:**

| Input | What it does |
|-------|-------------|
| D-Pad | Navigate cards / perk / reroll / start |
| A | Buy / confirm / start wave |
| B | Deselect |
| L | Lock selected card |
| R | Cycle owned companions (to sell) |
| Touch | Everything above, plus heal button |

## Building

Needs [devkitPro](https://devkitpro.org/) with the NDS toolchain. Then:

```
make
```

Out comes `trailmix.nds`. Run it on hardware with a flashcart or in [melonDS](https://melonds.kuribo64.net/).

Sprites are pre-converted in `arm9/data/`. To regenerate from source PNGs:

```
python3 tools/png2bin.py
python3 tools/png2bin_enemy.py
```

## Screenshots

<!-- TODO: gameplay -->

<!-- TODO: shop screen -->

<!-- TODO: boss fight -->

## Credits

All sound effects from [Kenney.nl](https://kenney.nl) (CC0).
Music from [OpenGameArt.org](https://opengameart.org) (CC0).
Built with devkitARM and libnds.

## License

MIT
