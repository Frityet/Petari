# MarioMoveSphere and MarioParts RMGK02 recovery

Timestamp: `2026-08-09T02:36:51Z`

This tranche reconstructs the two small root Player foundation units `Game/Player/MarioMoveSphere.cpp` and `Game/Player/MarioParts.cpp`. It is exact-source work only: neither unit is copied into or activated by `pc-port`, and no compatibility, stage, factory, or debug behavior was added.

## Recovered behavior

- `MarioMove::MarioMove` preserves the retail `MarioModule` ownership relationship and stores only its `MarioActor` owner.
- `MarioMove::initAfter` snapshots Mario's head, front, and side vectors into both vector banks and clears the two trailing scalar values.
- Both `MarioParts` constructors initialize the retail draw-buffer selection, fog display-list maker, differed display-list buffer, optional light controller, and optional fixed-position binding.
- `MarioParts::init` restores the retail effect/sound/clipping and appearance sequence.
- The generated `MarioParts` destructor and vtable reproduce the retail object.

## ABI evidence

RMGK02 constructor code stores the actor pointer at `0x04`, installs the five-entry `MarioMove` vtable, and never initializes a `MarioState` status/link tail. `MarioMove` therefore derives from `MarioModule`, not `MarioState`. Its six vectors occupy `0x08..0x4F`, followed by floats at `0x50` and `0x54`, for a total size of `0x58`.

The existing `MarioParts` declaration already matches the retail ABI: its only derived member is the effect resource name pointer at `0x9C`, and its size is `0xA0`. No `MarioParts.hpp` change was required.

## Objdiff fidelity

| Unit | Retail `.text` | Functions | `.text` match |
| --- | ---: | ---: | ---: |
| `MarioMoveSphere` | `0xB0` / 176 bytes | 2 | 100% |
| `MarioParts` | `0x230` / 560 bytes | 4 | 100% |

`MarioMoveSphere` also matches `.data` and `.sdata2` at 100%. The `MarioParts` vtable and zero constant match at 100%. The remaining target-only `MarioParts` `.data` bytes are the `lbl_805C8FB0` 288-byte Shift-JIS animation/sound string pool attributed by the retail split; no recovered function in this unit references it, so the source does not synthesize an unused duplicate of the neighboring Mario actor literals.

## Integrity

- Both focused RMGK02 source objects compile successfully with the configured `GC/3.0a3` compiler.
- `git diff --check` is clean for the frozen paths.
- A full `build/RMGK02/main.dol` rebuild succeeds.
- `build/tools/dtk shasum -c config/RMGK02/build.sha1` reports `OK`.
- The rebuilt and original DOL SHA-256 values are identical: `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`.

## Frozen review paths

- `include/Game/Player/MarioMove.hpp`
- `src/Game/Player/MarioMoveSphere.cpp`
- `src/Game/Player/MarioParts.cpp`

See `verification.log` for command-level evidence. This note directory is intentionally ignored and is evidence only.
