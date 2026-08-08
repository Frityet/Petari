# AreaObj and RestartCube retail foundation

Date: 2026-08-08 UTC

This checkpoint repairs and recovers the root decompilation needed before the
PC port can honestly expose Gateway's area objects. No PC factory entry is
enabled by this checkpoint.

## Recovered behavior

- `AreaObjContainer::getManager` now performs the retail prefix lookup from
  manager zero using `std::find_if`; the old code read an uninitialized pointer,
  skipped manager zero, and could dereference the end iterator.
- `MR::tryToUpdatePlayerRestartIdInfo` queries the real `RestartCube` area at
  the supplied position and dispatches `updatePlayerRestartIdInfo` when found.
- `MR::getPlayerRestartIdInfo` and `MR::setPlayerRestartIdInfo` follow the retail
  `GameSystem -> GameSequenceDirector -> GameDataTemporaryInGalaxy` ownership
  chain.
- `MR::calcCylinderPos` delegates to the area's real `AreaFormCylinder`.
- `ImageEffectAreaMgr::sort` and `LightAreaHolder::sort` restore ascending
  retail priority ordering by `Obj_arg7` and `Obj_arg1`, respectively.

## RMGK02 evidence

Focused objdiff results:

```text
AreaObjContainer::getManager                  152/152 bytes  100%
MR::tryToUpdatePlayerRestartIdInfo             84/84 bytes  100%
MR::getPlayerRestartIdInfo                     20/20 bytes  100%
MR::setPlayerRestartIdInfo                     20/20 bytes  100%
MR::calcCylinderPos                            16/16 bytes  100%
ImageEffectAreaMgr::sort                      172/172 bytes  100%
LightAreaHolder::sort                         160 bytes      96.375%
```

The remaining `LightAreaHolder::sort` differences are register-allocation-only
argument mismatches; its branches, offsets, comparison, swaps, and behavior
match the retail selection sort.

The complete `AreaObjContainer` unit is now 100%: 1244/1244 code bytes,
1968/1968 data bytes, and 14/14 functions. A full `ninja -j 12` completed and
`sha1sum --check config/RMGK02/build.sha1` reported `build/RMGK02/main.dol: OK`.

## Gateway data check

The user-provided extracted RMGK02 `HeavensDoorGalaxy.arc` contains four common
`RestartCube` rows:

```text
row 0: Obj_arg0=1, Obj_arg1=1,  Obj_arg2=-1, Obj_arg3=-1
row 1: Obj_arg0=2, Obj_arg1=-1, Obj_arg2=-1, Obj_arg3=-1
row 2: Obj_arg0=3, Obj_arg1=2,  Obj_arg2=-1, Obj_arg3=-1
row 3: Obj_arg0=4, Obj_arg1=1,  Obj_arg2=-1, Obj_arg3=-1
```

Thus all four volumes update the restart ID without requiring the on-ground
branch, while three also request real HeavensDoor BGM transitions. A PC port
cannot replace those branches with a fixed value or silently ignore the state.

## Real-or-absent boundary

The unchanged retail container owns 67 managers. RestartCube remains absent
from the PC factory until the generalized AreaObj manager lifecycle, persistent
stage/restart state, real player/demo predicates, and ID-aware audio state all
exist outside `src/Game`. The stage preflight continues to reject Gateway before
constructing any placement actors.
