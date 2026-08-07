# Generalized PC `LodCtrl` compatibility

## Purpose

Gateway NPCs use the ordinary `NPCActorCaps::mLodCtrl` path. In particular,
`RunawayTico::exeGuide1()` calls `mLodCtrl->invalidate()` and later
`mLodCtrl->validate()` without a null check. The PC `NPCActor` implementation
previously advertised the capability in `setDefault()` but never created the
controller, so importing the regular actor source would dereference null during
the guide/transformation sequence.

This change implements that engine prerequisite generically. It contains no
actor name, stage name, route, or effect-name special cases.

## Original-source evidence

- `include/Game/LiveActor/LodCtrl.hpp` is copied byte-for-byte to
  `pc-port/src/Game/LiveActor/LodCtrl.hpp` (`cmp` exit status 0).
- `src/Game/NPC/NPCActor.cpp:418-420` creates the controller whenever
  `mLodCtrl` is enabled.
- `src/Game/NPC/NPCActor.cpp:478-538` forwards appearance, death, and each
  control update to the controller.
- `src/Game/Util/LiveActorUtil.cpp:2615-2625` defines the NPC factory defaults:
  NPC draw/movement classes, animation synchronization, light initialization,
  shadow-host decoupling, and camera-Z distance selection.
- `build/RMGK02/asm/Game/LiveActor/LodCtrl.s:211-331` supplies the still
  undecompiled original `LodCtrl::update()` decision tree. The compatibility
  implementation preserves its priority order: hidden view override, forced
  high/middle/low view state, distance thresholds, and transform following.
- `src/Game/NPC/RunawayTico.cpp:222` and `:234` are the concrete Gateway
  dependency points.

## Implementation

- Controller methods live in `pc-port/src/compat/LodCtrlCompat.cpp`; no
  compatibility logic was added to `pc-port/src/Game`.
- The controller discovers `Middle` and `Low` models from the host actor's
  registered model resource and the ordinary `/ObjectData/<name>Middle.arc`
  and `/ObjectData/<name>Low.arc` paths. Missing optional archives retain the
  original high-model-only behavior.
- As in the regular source, replacement `ModelObj` instances retain the host
  base-matrix pointer and also receive the host translation, rotation, and
  scale during LOD updates.
- Model switching follows the original two-update handoff. The replacement is
  appeared and synchronized first; the prior model is hidden or killed on the
  following update.
- Null checks make direct controller calls safe on the host while retaining the
  original field layout and public API.
- Shadow visible-sync ownership has no host representation yet. The original
  `_1A` state transition is retained; only the unavailable shadow-controller
  callback is omitted.
- Material/joint animation names are propagated through the existing host model
  API. PC clipping calls are applied uniformly to the host and every discovered
  LOD model.

## Verification

See `test-results.txt`. The focused test covers capability allocation, disabled
capability behavior, NPC appear/dead forwarding, camera-Z factory selection,
the original two-update middle/high handoff, transform following, and the
view-control hidden override.

Baseline commit while testing: `1a562d904e28c55a8450b21cfbbbf0c9b07d37ad`
on branch `pcp-aurora`.
