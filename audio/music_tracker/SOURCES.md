# Trail Mix Tracker Music Sources

Selected formats in this folder are tracker modules only: `XM` and `IT`.

Licenses used:
- CC0 / public domain only

Validation notes:
- OpenMPT and MilkyTracker were not installed in this environment.
- Each selected module was at least decoded successfully with FFmpeg's `libopenmpt` backend as a tracker playback sanity check.

## File Map

| Final file | Original source file | Source URL | License |
| --- | --- | --- | --- |
| `menu.xm` | `napping on a cloud.xm` | https://opengameart.org/content/napping-on-a-cloud | CC0 |
| `battle.xm` | `my street.xm` | https://opengameart.org/content/my-street | CC0 |
| `shop.it` | `k_jose_-_exploring.it` | https://modarchive.org/index.php?query=183052&request=view_by_moduleid | CC0 |
| `boss.xm` | `devhell1.xm` | https://opengameart.org/content/2-creepy-background-songs | CC0 |

## Size / Duration Check

| File | Size bytes | Decoded duration |
| --- | ---: | ---: |
| `menu.xm` | 23223 | 248.808s |
| `battle.xm` | 35484 | 213.438s |
| `shop.it` | 10842 | 105.688s |
| `boss.xm` | 8765 | 46.800s |

Notes:
- All four selected modules are under the requested `80 KB` size cap.
- `boss.xm` fits the requested `30-60s` duration band.
- The other three were the best low-footprint CC0/public-domain tracker matches found, but they exceed the requested loop-length band.
