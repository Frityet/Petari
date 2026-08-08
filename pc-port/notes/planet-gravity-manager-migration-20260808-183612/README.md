# Exact PlanetGravityManager migration

Date: 2026-08-08 UTC

## Outcome

The PC port now uses the exact decompiled `PlanetGravityManager` for gravity
registration, priority ordering, filtering, vector combination, normalization,
and `GravityInfo` selection. The duplicated `StageGravityService` manager/query
implementation was removed.

Scene ownership stays in the generalized compatibility layer:

- `SceneObjHolderBinding` owns the manager as a `NameObj`.
- SceneObj ID `0x32` constructs `PlanetGravityManager("重力")`, matching the
  retail `SceneObjHolder::newEachObj` mapping.
- `StageHostScene` creates ID `0x32` before placement roots initialize.
- `GameGravityCompat` supplies only host-boundary concerns: explicit absence,
  null and duplicate validation, and safe host-pointer narrowing before it
  delegates to the exact manager.
- Roots are destroyed before the SceneObj binding, so the manager remains alive
  throughout root teardown.

## Source identity

Both checks returned zero:

```text
cmp src/Game/Gravity/PlanetGravityManager.cpp \
    pc-port/src/Game/Gravity/PlanetGravityManager.cpp
cmp include/Game/Gravity/PlanetGravityManager.hpp \
    pc-port/src/Game/Gravity/PlanetGravityManager.hpp
```

SHA-256 for both manager `.cpp` copies:

```text
9cfb3913d2e3951edf5c870ea45dda83f0b325207c524d68e2ed9f9a1aaccd
```

SHA-256 for both manager `.hpp` copies:

```text
b7ce50c1f53263579802374135007900656a781162452d304a588d6b1c61c1e8
```

## Host compiler boundary

Unmodified retail source contains a pointer-to-`u32` cast at
`PlanetGravityManager.cpp:36`. Strict 64-bit GCC rejects that cast. The manager
translation unit therefore receives one file-local `-fpermissive`; no Game
source was changed. `compile_commands.json` inspection found exactly one manager
TU entry and exactly one `-fpermissive` argument in that entry.

The 64-bit host layout is naturally larger than the annotated Wii layout
because `NameObj` and pointers are larger. This is not a live ABI blocker: all
consumers are recompiled C++ and access named members, with no serialized or
precompiled consumer of the Wii offsets. The public host token remains `u32`,
so compatibility code and the exact manager both use the low 32 bits, matching
the previous host behavior and retail API.

## Verification

Builds:

```text
xmake build smg-pc-gravity-real-or-absent-tests
xmake build smg-pc-sceneobj-holder-real-or-absent-tests
xmake build smg-pc-aurora-native-tests
```

Runtime results:

```text
6 gravity real-or-absent tests passed
3 SceneObjHolder real-or-absent tests passed
25 Aurora-native tests passed
```

Coverage includes empty-manager behavior, priority and equal-priority vector
combination, `GravityInfo`, host exclusion on a 64-bit process, type masks,
explicit no-holder and no-manager absence, null registration, duplicate
registration, exact `PointGravityCreator` registration, SceneObj singleton and
cross-scene isolation, placement preflight side effects, follower post-placement
binding, and grounded-normal gravity behavior.

`git diff --check` passed, and a repository search found no remaining
`StageGravityService`, `StageGravityStats`, or `StageGravityLoadStats` reference.

Final integrated verification also passed:

```text
xmake -vD smg-pc
xmake test -g aurora -j 1
100% tests passed, 0 failed out of 33

RMGK01 title probe: exact archives resolved; sequence completed at frame 373
RMGK01 HeavensDoorGalaxy scenario 1 strict probe: expected exit 1
first real missing creator: RestartCube in jmp/placement/common/areaobjinfo
```

## Retail invariants retained

The exact manager has a fixed 128-entry array and does not provide host-only
capacity/statistics/clear APIs. Those service-only behaviors were not retained.
Real retail stage data is expected to satisfy the 128-field invariant.

Gravity creators and fields still rely on the retail scene-heap lifetime model;
the PC port does not yet reclaim every such allocation as a scene heap would.
The removed service also did not own or reclaim those allocations, so this is an
existing compatibility-layer lifetime gap rather than a regression from this
migration.
