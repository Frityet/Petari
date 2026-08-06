# Placement-backed stage collision and gravity

This checkpoint replaces the PC host's previous empty map-collision boundary
and actor-gravity fallback with generalized services populated from normal
stage placement and archive data. The implementation is under `src/scene` and
`src/compat`; there are no stage, zone, route, placement-row, or object-path
exceptions.

## Collision

- `StageCollisionService` reads the original big-endian KCL header, positions,
  normals, and prisms, including the format's stored prism pointer adjustment.
- It reconstructs prism vertices using the same normal/cross-product equations
  as `KCollisionServer`, applies each placement's full TRS, and builds one BVH
  for line and sphere queries.
- The existing `MR::getFirstPoly*OnLineToMap` and `isExistMapCollision`
  compatibility boundary now delegates to the active stage service.
- `ActorMotionCompat` restores the original `LiveActor::movement` gravity and
  binder phases outside `Game/`: free actors integrate velocity, while actors
  with binders resolve a sphere against KCL and update ground, wall, roof, and
  contact normals.

The native synthetic-resource test proves a one-prism KCL reconstructs into
the expected triangle, preserves its surface attribute, answers a line cast,
returns a sphere contact, and prevents binder motion through the surface.

## Gravity

- `StageGravityService` consumes ordinary placement records for
  `GlobalPointGravity`, `GlobalPlaneGravity`, `GlobalPlaneGravityInBox`, and
  `GlobalPlaneGravityInCylinder`.
- Placement SRT, range, distant cutoff, priority, inverse flag, gravity type,
  base distance, distance axis, cylinder dimensions, and same-priority vector
  combination are data driven.
- The original `MR::calcGravityVector*` boundary queries the active service;
  actor-local and grounded-normal fallbacks remain available when no field
  applies.

The native test covers point direction, rotated parallel direction, active
service lifecycle, and the original MR query boundary.

## Real-disc route evidence

The real RMGK01 `gateway_handoff` route passed the retained title, file select,
five-page picturebook, and HeavensDoor transition at frame 10350. The aggregate
[manifest](gateway/manifest.json) and checkpoint
[manifest](gateway/gateway_handoff/manifest.json) preserve the route result.

The [application log](gateway/gateway_handoff/gateway_handoff-app.log) records:

- FileSelect: one KCL mesh / two triangles and one gravity field.
- HeavensDoor scenario 1: 242 placement records, 134 collision archive probes,
  61 accepted KCL meshes, 22,416 triangles, and zero rejected triangles.
- HeavensDoor: seven supported gravity fields and zero unsupported fields.
- Camera 78 / `s:004e` / `CAM_TYPE_XZ_PARA`, resolved from the selected
  StartInfo and its zone archive.

The captured [PNG](gateway/gateway_handoff/gateway_handoff-frame-10350.png),
[placement report](gateway/gateway_handoff/gateway_handoff-placement-report.md),
validator log, application log, and local SQLite trace remain in this evidence
directory. The large SQLite database is intentionally kept out of Git.

## Verification

- `xmake build smg-pc`: passed.
- `xmake build smg-pc-aurora-native-tests`: passed.
- `xmake run smg-pc-aurora-native-tests`: 22/22 passed.
- `xmake build smg-pc-stage-start-camera-tests`: passed.
- Focused stage-camera suite: 4/4 passed, including the optional real-disc
  HeavensDoor resource test.
- Root decomp is configured for RMGK02; incremental `ninja` is clean.
- Source-closeness audit: 271 `Game/` files checked, including 63 exact-source
  and eight compile-only files; compatibility/runtime code is inventoried
  separately.

Remaining physics work is intentionally explicit: moving collision ownership,
complete collision-attribute/material behavior, and the full original
`PlanetGravityManager` object surface are not claimed by this checkpoint.
