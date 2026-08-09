# Mario constructor-seeded state recovery

Timestamp: `2026-08-09T02:10:50Z`

Scope was deliberately limited to the four root decompilation units directly seeded by the Mario constructor and the ABI-proven header corrections they require. No PC mirror, factory, compat route, or build-list activation was added.

## Recovered units

| Unit | Original `.text` | Final `.text` match |
| --- | ---: | ---: |
| `MarioClimb.cpp` | `0x2BC` / 6 functions | 99.942856% |
| `MarioFrontStep.cpp` | `0x5E4` / 7 functions | 99.84881% |
| `MarioFaint.cpp` | `0x6C8` / 8 functions | 98.951614% |
| `MarioBlown.cpp` | `0x780` / 6 functions | 99.74792% |

The implementations were recovered from the current RMGK02 relocatable objects and validated function-by-function with `objdiff-cli`. `origin/mario` and `origin/player-phase-01` through `origin/player-phase-06` were inspected first, but none of those refs contains the four source `.cpp` files; their markdown reports still describe these functions as missing.

## ABI/header evidence

- `MarioClimb` owns a `u16` timer at `0x12` and overrides `start`, `close`, and `update`.
- `MarioFrontStep` has no derived data and overrides `start`, `close`, `update`, and `postureCtrl`.
- `MarioFaint` owns `u16` fields at `0x12`, `0x14`, and `0x16`, a vector at `0x18`, and byte flags at `0x24` and `0x25`; it also exposes `setVec`.
- `MarioBlown` owns `u16` fields at `0x12` and `0x14`, a vector at `0x18`, and byte flags at `0x24` and `0x25`.
- RMGK02 callers and implementations prove boolean returns for `Mario::doFrontStep`, `Mario::blown`, and `Mario::damagePolygonCheck`.
- RMGK02 `MarioDamageFreeze.o` defines `Mario::setJumpVec(const TVec3f&)`, and both `MarioBlown` call sites resolve to that symbol rather than `MarioModule::setJumpVec`.

## Verification summary

- All four focused RMGK02 objects compile successfully.
- Function-level matches are recorded in `verification.log`; most functions are exact or above 99.7%. The lowest remaining function is `MarioFaint::start` at 96.94915%.
- `git diff --check` is clean for the frozen path set.
- A full `build/RMGK02/main.dol` rebuild succeeds.
- Rebuilt and original DOL SHA-256 values are identical: `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`.

## Frozen review paths

- `include/Game/Player/Mario.hpp`
- `include/Game/Player/MarioBlown.hpp`
- `include/Game/Player/MarioClimb.hpp`
- `include/Game/Player/MarioFaint.hpp`
- `include/Game/Player/MarioFrontStep.hpp`
- `src/Game/Player/MarioBlown.cpp`
- `src/Game/Player/MarioClimb.cpp`
- `src/Game/Player/MarioFaint.cpp`
- `src/Game/Player/MarioFrontStep.cpp`

This note directory is intentionally ignored and is evidence only.
