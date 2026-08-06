# KCL, Binder, and global-gravity parity correction

This checkpoint corrects generalized stage physics against the RMGK02 root
decomp and assembly. It contains no stage-name, route-name, placement-row, or
archive-path policy. The retained acceptance route still begins at the normal
title screen, passes file select and all five picturebook advances, and enters
HeavensDoor scenario 1.

## Original references

- `src/Game/LiveActor/Binder.cpp`: initial-center collision check, 35-unit
  movement subdivision, one projected retry, 1.2-unit skin, and per-axis
  positive/negative reaction extrema.
- `build/RMGK02/asm/Game/Map/KCollision.s`: exact `KCHitSphere` and
  `KCHitArrow` predicates, including signed prism thickness, face-axis
  edge/corner depth, one-sided arrows, endpoint hits, and the 0.01-unit edge
  tolerance.
- `src/Game/Gravity/PointGravity.cpp`: a point field reports a valid zero
  vector throughout its 0.001-unit near-center region.
- `src/Game/Gravity/GravityCreator.cpp`: global plane direction is derived
  from rotation only, independent of placement scale.
- `src/Game/Gravity/PlanetGravityManager.cpp`: the first applicable priority
  wins, equal priorities combine, and registration order is stable.
- `src/Game/Util/GravityUtil.cpp`: negative Range, Distant, Priority, and
  Inverse values retain constructor defaults.

## Corrections

`StageCollisionService` now models a KCL triangle as a one-sided prism rather
than a generic two-sided triangle:

- BVH bounds include the slab extruded by `-normal * thickness`.
- line casts require a front-side start, accept a hit at `t == 1`, and retain
  the physical 0.01-unit edge tolerance.
- sphere depth is `sqrt(radius^2 - lateral^2) - signed_face_distance`; face
  hits reduce to `radius - signed_face_distance`.
- depth must be in the inclusive range `[0, thickness]`; zero-radius point
  queries work for strict face interiors.
- Binder checks the initial center, stops at its first colliding substep,
  aggregates coincident/seam reactions by component extrema, and makes one
  projected retry for remaining movement.
- the third `initBinder` value is consumed as the stored-plane capacity (zero
  selects the original temporary 32-plane capacity), not as a binder type.

`StageGravityService` now ignores negative JMap sentinels, builds plane axes
from rotation-only SRT, preserves stable equal-priority order, and treats an
exact or near point-field center as a valid zero field that suppresses lower
priorities. `ActorMotionCompat` follows `LiveActor::movement`'s `calcGravity`
behavior, retaining the previous gravity when no field applies; it no longer
overwrites gravity from a post-bind floor normal.

The native resource suite covers front/back and endpoint rays, edge tolerance,
thin and extruded prisms, analytical edge depth, radius-zero hits, initial
overlap, duplicate-face extrema, plane capacity, all Inverse sentinel values,
negative parameter sentinels, rotation with zero scale, and point-center
priority suppression.

## Route evidence

The current `gateway_handoff` run passed at frame 10350. Its aggregate
[manifest](manifest.json), checkpoint [manifest](gateway_handoff/manifest.json),
[screenshot](gateway_handoff/gateway_handoff-frame-10350.png),
[application log](gateway_handoff/gateway_handoff-app.log),
[placement report](gateway_handoff/gateway_handoff-placement-report.md), and
[trace validator log](gateway_handoff/gateway_handoff-trace-validator.log) are
kept here. The generated SQLite trace and save directory remain local and are
intentionally excluded from Git.

Expected checkpoint facts:

- route status: passed;
- stage/scenario: `HeavensDoorGalaxy` / 1;
- placement result: 168 created, 2 blocked, 72 intentionally ignored out of
  242 records;
- trace: 573 render packets and 1,864 semantic events;
- screenshot: 640x480 with a 0.98057 non-black ratio.

## Verification

- `xmake build smg-pc`: passed.
- `xmake run smg-pc-aurora-native-tests`: 22/22 passed.
- `xmake run smg-pc-stage-start-camera-tests`: 4/4 passed (the optional
  real-disc HeavensDoor case included).
- `xmake aurora-route-smoke ... gateway_handoff`: passed from title through
  file select, picturebook, and stage handoff.

## Explicit next boundaries

- Moving KCL is still loaded as static geometry. The rotating Gateway parts
  need a generalized `CollisionParts` lifecycle and transform updates; they
  must not become object-name exclusions.
- KCL `.pa` rows need to remain associated with source meshes and contacts.
  Gateway rotating parts contain `DamageElectric` floor codes, so a raw row
  index without source identity cannot reproduce material behavior.
- The stage host does not yet instantiate a playable actor from selected
  `StartInfo`. A compatibility-owned, data-driven stage player is the next
  hard route blocker before chase/demo actors can be exercised.

## Follow-up exactness audit

A second source/assembly pass after the initial checkpoint found and corrected
several edge cases that the first route smoke did not exercise:

- Binder plane capacity is cumulative across its initial sweep and projected
  retry. A retry still detects and stops at a face after storage is full, but
  that unstored face contributes neither a contact plane nor a reaction.
- Host BVH traversal no longer makes limited contact arrays deepest-first.
  Until KCL octree leaf lists are retained, capacity uses deterministic source
  prism order; this is documented as an approximation of original octree-list
  encounter order rather than claimed as exact.
- `initBinder(0, 0, ...)` is now represented explicitly and zero-radius point
  binders reach the strict-interior KCL path. `initBinder` also restores the
  original `onBind` transition.
- Binder centers use the actor's scale-free base-matrix Y basis. Host-only TRS
  scale is removed, including negative scale, and a zero Y scale reconstructs
  the basis from the X/Z columns or actor rotation when all axes collapse.
- wall/floor/roof classification now preserves the exact strict wall threshold
  (`abs(dot) < 0.34202015`), with threshold equality classified by sign.
- transformed KCL now applies affine plane separation to prism thickness and
  preserves the original 0.01 local-unit arrow allowance through
  source-derived barycentric tolerances.
- missing gravity fields return false and write zero rather than echoing
  `LiveActor::mGravity`; a valid zero field still reaches the grounded-normal
  branch of `calcGravityOrZero`.

The host gravity implementation now lives in
`src/compat/GameGravityCompat.cpp`. `pc-port/src/Game/Util/GravityUtil.cpp` and
`.hpp` are byte-identical to the root decomp and excluded from the host target;
the Metrowerks null-host overloads live in a forced compatibility header.

Post-audit verification remained green: 22/22 Aurora native tests, 4/4 stage
camera tests, and the `smg-pc` application link all passed in debug mode.
