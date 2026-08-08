# Exact CubeCamera AreaObj closure

This change adds `CubeCamera.{hpp,cpp}` to `pc-port/src/Game/AreaObj` as a
byte-identical mirror of the decompiled source. The Game source remains free of
host policy and fallback behavior.

In the CubeCamera validation snapshot, the generalized AreaObj compatibility
registry exposed the four retail factory names in original order:

- `CubeCameraBox` (`AreaForm::Type_Cube1`)
- `CubeCameraCylinder` (`AreaForm::Type_Cylinder`)
- `CubeCameraSphere` (`AreaForm::Type_Sphere`)
- `CubeCameraBowl` (`AreaForm::Type_Bowl`)

All four descriptors share one deduplicated `CubeCameraMgr` descriptor at
retail manager-table order 4 and capacity `0xA0`. The registry's optional
manager-finalize callback is general: after all scene-owned managers receive
their virtual `initAfterPlacement`, callbacks run in manager creation order and
are guarded against repeat dispatch. CubeCamera uses that boundary to call its
exact `CubeCameraMgr::initAfterLoad`, which performs the retail priority sort.

No camera-selection system, `CameraManGame`, or synthetic camera behavior is
claimed. This closure is limited to exact placement construction, switch and
category state, manager ownership, sorting, and volume lookup.

## Real-disc result

The focused test constructed all 16 `CubeCamera*` rows from RMGK01
`HeavensDoorGalaxy` scenario 1 through their exact JMap init path:

- 2 box rows
- 8 cylinder rows
- 6 sphere rows
- 0 bowl rows
- 3 `SW_APPEAR=1015` rows
- 1 `SW_A=1125` row
- 1 `SW_A=1127` row
- sorted priorities: `-1,0,0,0,1,2,5,8,8,8,9,10,10,11,12,13`

The CubeCamera-only strict preflight frontier was 242 total, 45 complete, 189
blocked, and 8 intentionally ignored. The first unsupported row remained
`RestartCube`; the 16 CubeCamera rows account exactly for the increase from the
prior 29 complete. A later concurrent MessageArea descriptor layer began after
this 9/9 snapshot and requires the final integrated rerun.

See `verification.log` for the commands and outcomes.
