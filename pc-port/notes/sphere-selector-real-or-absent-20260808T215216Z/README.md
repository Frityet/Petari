# SphereSelector exact PC closure boundary

## Outcome

The four PC Game translation-unit files for `SphereSelector` and
`SphereSelectorHandle` are byte-identical mirrors of the root decomp. There is
no PC-only branch, debug bypass, or compatibility workaround in those files.

The non-audio runtime closure is implemented in generalized compatibility and
Aurora runtime services. `SphereSelectorHandle` is deliberately **not** exposed
through `NameObjFactory`: its mandatory rotation path reaches retail
`MR::startAtmosphereLevelSE`, while the PC audio backend does not yet implement
the retail
`AudWrap::getAtmosphereSeObject()->startLevelSoundParam` behavior. The provider
throws a precise `logic_error`; it does not return a logical-silent handle or
record a substitute event.

Consequently the strict FileSelect report remains honest: four objects total,
two complete, and two blocked. `FileSelector` remains the first blocker and
`SphereSelectorHandle` is recorded as
`atmosphere_level_sound_playback_runtime_unavailable` with no creator route.

## Exact source boundary

These PC files compare byte-for-byte with their root counterparts:

- `pc-port/src/Game/Map/SphereSelector.hpp`
- `pc-port/src/Game/Map/SphereSelector.cpp`
- `pc-port/src/Game/Map/SphereSelectorHandle.hpp`
- `pc-port/src/Game/Map/SphereSelectorHandle.cpp`

The coordinated source-mirror test also covers the exact
`PlacementStateChecker.{hpp,cpp}` pair added by the stage-start lifecycle wave.

## Real compatibility infrastructure

- SceneObj `0x6F` constructs, initializes, owns, and returns the exact
  `SphereSelector` synchronously through the ordinary scene-object holder.
- Star-pointer Sphere finger/reaction modes use a requester-scoped,
  retail-priority mode table. `endStarPointerMode` pops the requester's latest
  entry, and scene-scope teardown removes all entries owned by a destroyed
  `LiveActor`, preventing stale requester pointers.
- The pointer position-or-edge query reads the real WPAD channel and clamps to
  the Wii logical screen bounds. With no active runtime it fails explicitly.
- Default-game-layout activation is the real inverse of the existing
  deactivation service.
- Camera view-matrix, up/front matrix, interpolation, nerve easing, and LP64
  recovered-`long` compatibility are provided outside Game source.
- Existing exact message-sensor, simple-demo-cast, and scene-scheduler paths
  accept the real handle and retain its `LiveActor` identity.
- Atmosphere level sound playback remains absent and explicit, which is the
  only factory-advertisement blocker found by this closure.

## Retail evidence

The focused test reads `/workspaces/pcport/RMGK01.iso` through the real DVD and
placement resolver. In FileSelect scenario 1 it finds
`SphereSelectorHandle` at row 2 of
`jmp/placement/common/objinfo`, layer `common`, with `Obj_arg0 == 0`.

That exact `JMapInfoIter` initializes the decompiled handle, synchronously
binds it to SceneObj `0x6F`, adds it to the real target group, and registers it
as a simple demo cast. Messages `0xE0` through `0xE5` are exercised through the
real scene message sensor. The test observes the retail deferred-nerve
contract: an accepted message queues the next nerve (`step == -1`) rather than
pretending the transition has already executed.

The exact `playRotateSE()` path is then exercised and is required to stop at
the explicit missing-audio error. This is why successful focused initialization
does not justify a factory creator.

## Verification

See `verification.log` for the final commands and output. At this checkpoint:

- `smg-pc-game` builds successfully.
- The focused real-or-absent suite passes 4/4 with the real RMGK01 row.
- The Game source-mirror suite passes all listed pairs, including the four
  Sphere files and two PlacementStateChecker files.
- `git diff --check` passes.

## Owned paths and shared hunks

See `owned-paths.txt`. Several files were concurrently edited by other waves;
only the named Sphere-specific hunks belong to this closure. In particular,
this work did not modify the protected `SaveIcon` or `TriggerChecker` files and
did not stage or commit anything.
