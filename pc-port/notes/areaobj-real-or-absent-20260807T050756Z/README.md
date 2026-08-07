# AreaObj real-or-absent boundary

Updated: 2026-08-07T05:07:56Z

## Outcome

The old `GameAreaCompat.cpp` fallback was removed. It used to report every
area query as `false` and generated evenly spaced Mercator rail positions.
Those answers looked valid despite having no parsed AreaObj ownership.

The PC build now compiles the byte-identical retail `Game/Util/AreaObjUtil.cpp`
translation unit. Its required `AreaObj`, `AreaObjContainer`, and `AreaObjUtil`
headers are also byte-identical to the root decompilation.

The compatibility boundary supplies only explicit unavailability:

- `AreaObjContainer` cannot be created through the scene holder until its
  retail manager set and parsed stage placements are hosted.
- Area, death, dark-matter, and water queries throw `std::logic_error` rather
  than answering `false`.
- Mercator rail division throws without invoking the position callback. It no
  longer substitutes ordinary equally spaced rail coordinates for the retail
  projection routine.

## Why the real container is not installed yet

The retail `AreaObjContainer` builds a fixed manager table containing generic
area managers and specialized camera, water, light, warp, glare, and image
effect managers. A useful instance must then receive real area objects created
from stage placement data. Those source/runtime closures are not yet hosted by
the PC scene. Constructing only the generic names would be another partial
stand-in, so the scene object remains absent.

## Verification

| Check | Result |
| --- | --- |
| `xmake -vD smg-pc` | pass |
| `smg-pc-area-obj-real-or-absent-tests` | 3/3 pass |
| `smg-pc-sceneobj-holder-real-or-absent-tests` | 3/3 pass |
| `smg-pc-game-actor-physics-real-or-absent-tests` | 7/7 pass |
| byte comparison of four imported Game files | exact |
| scan for the removed false/rail-spacing implementation | no matches |

See `exactness.sha256`, `test-output.txt`, and `symbol-evidence.txt` for the
supporting data.
