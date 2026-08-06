# Rosetta RMGK02 reconstruction

Date: 2026-08-06

## Result

Reconstructed the complete root `src/Game/NPC/Rosetta.cpp`. The previous
source contained only `makeArchiveList`; Rosetta can now initialize its NPC
base, sensors, talk callbacks, demo casts, joint tracking, HeavensDoor demo
executor, reactions, and demo lifecycle.

This is the critical actor-side prerequisite for the original Gateway tower
and spin-acquisition sequence. No PC-only Rosetta actor or stage-name branch
was added.

## Provenance and target corrections

- Recovered the full prior implementation from repository commit
  `ae8020773`.
- Recovered later compiler/target refinements from `0378b267f` and the local
  decomp record `decomp-notes/2026-06-01-rosetta-progress.md`.
- Rechecked every function against
  `build/RMGK02/asm/Game/NPC/Rosetta.s`.
- Target-backed corrections beyond the historical source:
  - direct zero stores to the message offset;
  - preserved message-controller local during callback registration;
  - `makeActorAppeared()` at the target virtual-call slot;
  - inherited `NPCActor::setDefaults()` for the shared reaction actions;
  - inline player-vector subtraction and exact `> 0.95f` stare comparison.

## Verification

```text
ninja build/RMGK02/src/Game/NPC/Rosetta.o
ninja: no work to do.  # clean confirmation rerun

objdiff unit: main/Game/NPC/Rosetta
.text    3204 bytes   98.93883%
.ctors      4 bytes  100.0%
.data     584 bytes  100.0%
.sdata2    48 bytes  100.0%
```

Twenty of 25 authored/comparable functions are exact. The five remaining
functions are all high similarity:

| Function | Similarity |
|---|---:|
| `makeArchiveList` | 99.78723% |
| `init` | 97.172935% |
| `control` | 99.69512% |
| `eventFunc` | 99.609375% |
| `exeReaction` | 99.254906% |

`canUpdateStarePos()` is exact. Remaining differences are compiler register
allocation, equivalent string relocations, and equivalent epilogue scheduling;
no known behavior differs.

```text
Rosetta.cpp SHA-256
971d488e3d6a1f3dd463f54a1eb533d323348826271ae88d4fa1bcf7de7ab8db

Rosetta.o SHA-256
a80eaeca978f2102fdc779f82ba93e235487ef013e84ad4b228d405be87ce7ed
```

A full bare `ninja` also encountered pre-existing failures in unrelated
`DemoDirector`, `CameraDirector`, `DemoExecutor`, and `DemoTimeKeeper` units.
The Rosetta object itself builds cleanly under the RMGK02 configuration.
