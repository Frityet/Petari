# HitSensorInfo RMGK02 recovery

Timestamp: `2026-08-09T03:30:33Z`

## Outcome

`HitSensorInfo::update()` is reconstructed from the RMGK02 retail assembly.
The complete `main/Game/LiveActor/HitSensorInfo` translation unit is now 100%
matched: 864/864 `.text` bytes and 3/3 functions.

This is a generalized LiveActor collision prerequisite. It does not register a
Mario factory, add a PC-only sensor path, or introduce object/stage-specific
behavior.

## Restored behavior

- Actor-owned callback sensors dispatch `LiveActor::updateHitSensor`.
- Matrix-follow sensors transform their local offset through the complete
  3x4 follow matrix, including translation.
- Position-follow sensors use their explicit position when present, otherwise
  the owning actor position.
- Actor-base-matrix sensors rotate the local offset through the actor matrix.
- Actors without a base matrix add the local offset directly.
- The resulting world position is written to the exact `HitSensor` field.

## Evidence

- Retail assembly:
  `build/RMGK02/asm/Game/LiveActor/HitSensorInfo.s`
- Focused object:
  `build/RMGK02/src/Game/LiveActor/HitSensorInfo.o`
- Objdiff unit:
  `main/Game/LiveActor/HitSensorInfo`
- `HitSensorInfo::update()`: 604/604 bytes, 100%
- Translation unit `.text`: 864/864 bytes, 100%
- Functions: 3/3, 100%
- Data: 100%

The structurally equivalent SMG2 implementation in SMGCommunity/Garigari was
used only as a source-shape cross-check. RMGK02 assembly and the local objdiff
target determined all operations and code generation.

## Integrity

- Focused MWCC compilation passes.
- Focused objdiff reports 100% for `update()`.
- The regenerated project report reports 100% for the whole unit.
- The canonical RMGK02 DOL SHA-1 manifest passes.
- Rebuilt DOL SHA-256:
  `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`
- Scoped `git diff --check` passes.
