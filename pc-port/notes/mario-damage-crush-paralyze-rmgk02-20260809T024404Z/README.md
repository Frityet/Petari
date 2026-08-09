# MarioDamageCrush and MarioDamageParalyze RMGK02 recovery

Timestamp: `2026-08-09T02:44:04Z`

This tranche reconstructs the root `Game/Player/MarioDamageCrush.cpp` and `Game/Player/MarioDamageParalyze.cpp` units plus their dedicated state layouts. It adds no PC activation, factory route, compatibility special case, or fallback.

## Recovered behavior

### Crush

- `Mario::requestCrush` sets the retail pending-crush input flag.
- `Mario::tryCrush` enforces the `_1F` movement-state and invincibility guards, restores shadow position for the retail airborne-or-not-grounded condition, resets damage/player-mode state, stops jump/walk, and enters `MarioCrush`.
- `MarioCrush` applies the squash scale, vibration, sound, life damage, game-over request, timed recovery animation, rush/A-button escape, jump restart, and close-time scale/status restoration.

### Paralyze

- `Mario::doParalyze` enforces the retail movement, invincibility, active-status, fire-damage, after-damage timer, and general damage guards before entering `MarioParalyze`.
- `MarioParalyze` applies electric damage animation/sound/effect/vibration, life damage, timed grounded/airborne recovery, airborne velocity, game-over routing, effect cleanup, and the close-time backward recovery jump.

## ABI evidence

- RMGK02 `Mario::tryCrush()` returns `bool` on every path. The stale shared declaration was corrected from `void` to `bool` after the MarioWalk owner froze and released `Mario.hpp`.
- `MarioCrush` overrides `start`, `close`, and `update`, with `u16` members at `0x12` and `0x14`; its size is `0x18`.
- `MarioParalyze` overrides `start`, `close`, and `update`, with `u16` members at `0x12`, `0x14`, and `0x16`, plus a boolean at `0x18`; its size is `0x1C`.
- Retail bit extraction proves the initial guard in both entry methods is movement flag `_1F`, not `jumping`. The later crush positioning condition is `jumping || !_1`.
- The target-owned `lbl_805C64D8` Faint animation string table is defined here so the already recovered `MarioFaint.cpp` external reference remains linkable when these source objects are eventually activated together.

## Objdiff fidelity

| Unit | Retail `.text` | Functions | `.text` fuzzy match |
| --- | ---: | ---: | ---: |
| `MarioDamageCrush` | `0x3E8` / 1,000 bytes | 7 | 99.96% |
| `MarioDamageParalyze` | `0x4DC` / 1,244 bytes | 6 | 99.877815% |

All Crush methods except `start` are 100%; `start` is 99.8% and differs only in pooled-string relocation identity. `doParalyze`, the constructor, the static initializer, and the vtable are 100%. The remaining Paralyze methods range from 99.6% to 99.93976%; their residual differences are pooled-string or scalar-constant relocation identities, with matching instructions and behavior.

Both `.ctors` and `.sdata2` sections are 100%. The unit-level `.data` percentages include retail split padding, weak Nerve vtables, and neighboring string-pool attribution and therefore are not used as behavioral fidelity measures.

## Verification

- Both focused RMGK02 source objects compile successfully with `GC/3.0a3`; only the repository's pre-existing `MarioActor` nontrivial-union warnings are emitted.
- `git diff --check` is clean for the frozen path set.
- A full `build/RMGK02/main.dol` rebuild succeeds after both the `tryCrush` correction and the concurrent independent `taskOnRotation` correction.
- `build/tools/dtk shasum -c config/RMGK02/build.sha1` reports `OK`.
- Rebuilt and original DOL SHA-256 values are identical: `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`.

## Frozen review paths

- `include/Game/Player/Mario.hpp` (`tryCrush` return type only; `taskOnRotation` belongs to the concurrent MarioSpin lane)
- `include/Game/Player/MarioCrush.hpp`
- `include/Game/Player/MarioParalyze.hpp`
- `src/Game/Player/MarioDamageCrush.cpp`
- `src/Game/Player/MarioDamageParalyze.cpp`

See `verification.log` for command-level evidence. This note directory is intentionally ignored and is evidence only.
