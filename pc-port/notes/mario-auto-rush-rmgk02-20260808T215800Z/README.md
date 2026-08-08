# Mario auto-rush RMGK02 reconstruction

Date: 2026-08-08 UTC

## Outcome

The retail MarioActor auto-rush message chain needed by FileSelector's title
handoff is reconstructed in the root decomp:

- `MarioActor::attackSensor`
- `MarioActor::tryStandardRush`
- `MarioActor::checkPriorRushTarget`
- `MarioActor::getNearestJumpTarget`
- `MarioActor::tryStartRush`
- the supporting `MarioActor::setPlayerMode(u16, bool)` ABI correction

Every actual `MarioActorRushMsg` gameplay function is a 100% match, as is
`attackSensor`. The ABI-corrected `setPlayerMode` is 99.60% fuzzy. The remaining
translation-unit percentage comes from unrelated missing sensor functions and
static relocation/data pairing, not the auto-rush chain.

Retail ownership remains unchanged: Mario's collision/sensor update sends
`ACTMES_AUTORUSH_BEGIN` through the prior-binder sensor to FileSelector. No PC
compatibility message injection or alternate title host was added.

## Runtime boundary

The PC tree still has no complete Mario/MarioActor player closure, MarioHolder,
or exact player collision/update pipeline. These root files are therefore not
partially exposed through a PC factory. The reconstruction is a decomp and
future exact-source foundation, not a claim that auto-rush currently runs on
PC.

## Verification

See `verification.log`.
