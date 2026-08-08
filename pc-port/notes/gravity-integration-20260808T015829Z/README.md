# Exact retail gravity integration

Date: 2026-08-08T01:58:29Z

Disc used locally: user-provided `RMGK01.iso` (not included)

## Scope

- Synced the recovered retail gravity family, `BaseMatrixFollowTargetHolder`,
  `JMapLinkInfo`, and the required JSystem `JMath.hpp` wrapper into the PC tree.
- Kept all 36 synced Game/JSystem files byte-identical to their root sources,
  including the exact `PlanetGravityManager` source and header.
- Put host-only matrix, model-presence, JMap-matrix, pointer-width, factory,
  lifetime, and SceneObj orchestration in compatibility/scene code. The exact
  manager owns registration, sorting, filtering, and gravity queries.
- Removed `PlanetGravityCompat.cpp` and all placement-derived synthetic gravity.
  Exact `GravityCreator` instances now own construction and call
  `MR::registerGravity` into the active scene manager.
- Removed the duplicate `StageGravityService`; SceneObj ID `0x32` now owns the
  exact retail `PlanetGravityManager` for the scene lifetime.
- Added a creation-order SceneObj `initAfterPlacement` pass so the exact
  BaseMatrix follower holder resolves links after actors initialize.
- Added a strict preflight gate before either explicit-root or placement-root
  construction. A blocked stage constructs no placement actors and registers no
  gravity.

## Real-or-absent boundary

`HeavensDoorGalaxy` scenario 1 is still unavailable because 216 of its 242
actor-bearing/helper rows do not yet have their retail creators. This is now a
generic missing-creator boundary; exact `GlobalPointGravity`/other gravity
factory entries are no longer the blocker. The strict rerun stops immediately
after stage environment setup and before any `stage_host_constructed` event,
actor initialization, or gravity registration.

The first remaining blocker is:

```text
RestartCube in jmp/placement/common/areaobjinfo
```

## Verification summary

- Full debug executable build/link: pass.
- Full Aurora test group: 33/33 targets pass, zero failures.
- Focused gravity real-or-absent suite: 6/6 pass.
- All ten global gravity names map to their exact retail creator functions.
- Follower binding and `host * inverse(placement)` transform: pass, including
  the in-place inverse alias path.
- `LiveActor::getBaseMtx`: null without a model, non-null with a host model.
- Duplicated retail scratch `DUMMY()` emitters: both debug objects expose a
  translation-unit-local `t DUMMY()` symbol.
- Title real-disc probe: pass at frame 373.
- Strict HeavensDoor real-disc probe: expected unavailable boundary, exit 1.
- `git diff --check`: pass.

See `verification.log`, `title-probe.log`, and `heavens-door-strict.log` for
concise command evidence.
