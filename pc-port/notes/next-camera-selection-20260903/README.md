# Next original camera selection closure

Read-only source audit, 2026-09-03. No production source was changed or built
for this audit. The current checkpoint executes original `CameraParallel`
and `CameraHeightArrange`; this document does not claim that the full
`CameraManGame` selector or director is active.

## Selection already exists in original source

`src/Game/Camera/CameraManGame.cpp:177` selects start-position/zoom first,
then the target's current category. `updateNormal` at line 560 does this:

1. Set the CubeCamera category to Normal and use the target's retained area.
   `setCubeChunk` at line 627 creates `c:%04x` from the area's zone and Arg0.
2. Otherwise query the target's ground triangle. A valid triangle with
   `MR::getCameraID(triangle) < 255` selects
   `g:<host actor name>:<camera ID>:0` in that triangle's placement zone.
3. Otherwise reapply the current chunk, or select the null/default camera
   when no chunk has been selected.

`setChunk`, reset decisions, translation of parameters, and safe-pose
handling are also present in the root source. There is no reason to invent
another selector. Preserve the original target-update/selection ordering:
`CameraDirector::movement` updates its target holder before its camera man;
the player target caches an area during that update.

## Concrete shared collision blocker

`pc-port/src/compat/HitInfoCompat.cpp:70` currently returns
`StageCollisionSurface::source_name` from `Triangle::getHostName`.
`getHostPlacementZoneID` at line 75 returns `-1` unconditionally.

The registration source in `pc-port/src/compat/CollisionPartsCompat.cpp:188`
is a diagnostic resource identity, `<resolved archive>:/<KCL entry>`. It is
passed to `StageCollisionService::register_kcl`, then exposed by
`StageCollisionSurface`. That surface has attributes and its sensor, but no
placement zone. Therefore its name and zone cannot presently create the
original ground-camera key, even though `GameMapCollisionCompat.cpp:276`
already reads the real PA `camera_id` field.

The original contracts are explicit:

- `src/Game/Map/HitInfo.cpp:54`: both triangle getters delegate to the part.
- `src/Game/Map/CollisionParts.cpp:225`: host name comes from
  `mHitSensor->mHost->mName`, with null checks.
- `src/Game/Map/CollisionParts.cpp:239`: placement zone comes from
  `mZone->mZoneID`.
- `src/Game/Map/CollisionParts.cpp:35`: the zone is established from
  `MR::getCurrentPlacementZoneId()` when the part is initialized.

Minimal general repair: retain owner identity and placement zone in every
native collision registration, separately from its resource identity.
`PlacementZoneNameScope` and `SceneSystemUtilCompat.cpp:75` already provide
the real current placement zone during construction. Expose the retained
provenance through triangle getters; keep owner withdrawal and source
rebuild lifetimes coherent. This supports every original triangle consumer,
not only cameras. Tests should use two actor names and two zones sharing the
same KCL, and check teardown/disabled-source behavior.

## Original player target dependencies

`src/Game/Camera/CameraTargetObj.cpp` already contains the complete
`CameraTargetPlayer` bodies. Its `movement` at line 102 always populates the
area and triangle, including when the selected controller is XZ parallel.

The current `PublishedCameraTarget` in
`pc-port/src/camera/OriginalGameCamera.cpp:43` only publishes vectors,
jumping, fast rise, and fast drop. It inherits null area/triangle and false
specialized predicates from the base target. `StageCameraTargetState` has
no area, triangle, water, or special-camera state. It is insufficient for
original category and ground selection.

The source-backed closure is:

- `MR::getCameraCube` -> `MarioAccess::getCameraCubeCode` ->
  `Mario::getCameraCubeCode` (`src/Game/Player/MarioCollision.cpp:1909`).
  The last body queries the real CubeCamera manager at Mario's position,
  or position plus gravity times 100 while rising or swimming at the
  surface. Its dependencies are `isSwimming`, `isRising`, gravity, and
  the real swim state. Do not replace this with a generic position query.
- `MR::getPlayerGroundingPolygon` ->
  `MarioAccess::getGroundingPolygon` (`MarioAccess.cpp:130`). This respects
  swimming, grounded state, and a bound actor's Binder before choosing
  Mario's actual ground triangle. Returning `mGroundPolygon` unconditionally
  would lose those semantics.
- The target also uses actual bind/base-matrix, bee/gravity, shadow,
  movement timer, and last-movement queries. Its virtual predicates forward
  to original `MarioActor` camera accessors and water/Foo state. Most bodies
  already exist; excluded large player translation units can supply exact
  accessor subsets through the established shared compatibility pattern.
- `MarioAccess::getPlayerActor` currently expects `MR::getMarioHolder`.
  A generic typed active-player provider must use the real attached actor;
  do not construct a partial holder or manufacture flags.

`pc-port/src/Game/AreaObj/CubeCamera.cpp` is already the original source.
`AreaObjRuntime` supplies the four retail forms, shared `CubeCameraMgr`,
priority sorting, category filtering, and switches. This part needs
integration, not a new area algorithm.

## Catalog and director are a separate complete boundary

An original `CameraManGame` cannot simply replace the current base
`CameraMan` object with two more methods:

- `CameraHolder` eagerly constructs all 45 original camera types and their
  translators (`src/Game/Camera/CameraHolder.cpp:172`). All listed types
  have root calculation implementations, directly or by inheritance, but
  the PC catalog currently imports only Parallel. Their full dependency
  closure has not been compiled in this audit.
- `CameraManGame` directly calls its real director for default FOV and
  interpolation. `CameraLocalUtil::setUsedTarget` writes the director's
  target. The current scoped target adapter does not supply these services.
- `CameraDirector::setInterpolation` updates a real
  `CameraViewInterpolator` and can request `CameraCover`.
  The real director constructor additionally creates target/rail/register
  holders, shaker, cover, all camera managers, and screen blur. A fabricated
  partial director would not satisfy its object contract.
- Original parameter storage needs `CameraParamChunkID`,
  `CameraParamString`, `CameraParamChunk`, `CameraParamChunkHolder`, and
  `DotCamReaderInBin`. Their root implementations are present. DotCam's
  direct `JMapData*` access needs a narrow representation/compile adaptation
  to the PC shared JMap data owner; its algorithm need not change.
- Original chunk loading fills previously registered chunks; it does not
  instantiate every BCAM row. `GameCameraCreator::scanArea` registers real
  area IDs, `scanStartPos` registers authored starts, and collision code
  collection registers ground IDs. Preserve that lifecycle and its
  defaults/through flags before `loadCameraParameters` and `sort`.

`KCollisionServer::calcFarthestVertexDistance` remains undecompiled in the
root (`src/Game/Map/KCollision.cpp:60`); original `CollisionParts::init`
brackets this call with camera-code collection. If its exact registration
semantics are needed, recover it root first. RMGK01 symbol:
`0x80183208`, size `0x188`. Do not infer which PA rows count as registered.

Small source-first scene helpers still missing include
`MR::getCurrentStartZoneId` (`0x803f75a0`, size `0x24`),
`MR::getCurrentStartCameraId` (`0x803f7a88`, size `0x24`),
`MR::getStageCameraData` (`0x803f7c14`, size `0x8c`), and
`MR::getStartCameraIdInfoFromStartDataIndex` (`0x803f7aac`, size `0x44`).
The retained stage holder catalog is suitable backing after their actual
root semantics are recovered.

`CamKarikariEffector` and `CamHeliEffector` also run unconditionally in the
original manager. Their root bodies exist. The original GCapture/Karikari
queries already define absent scene-object results; copying those getters
would preserve real absence semantics without introducing constant stubs.

## Recommended bounded next step

First close collision provenance and original target/player queries, with
real area/Binder fixtures. Then import the original parameter and creator
classes and obtain a complete catalog/director construction closure before
activating original manager selection. Keep unsupported camera types
explicit; never silently reinterpret their authored parameters as XZ.

A generic read-only camera catalog probe is still needed: enumerate the
retained stage holders, read each archive's `CameraParam.bcam`, and report
zone, ID, type, through/reset/interpolation flags, and matching authored
CubeCamera/ground IDs. No current extracted archives or built resource
probe were available, and no build was started during the root validation.
The old CubeCamera note reports 16 Gateway areas; that historical result is
not a current camera-type inventory or a claim that all selected types are
supported.
