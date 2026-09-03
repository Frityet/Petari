# Collision owner and placement-zone provenance

The shared collision surface now preserves the original host-name and zone
contracts needed by all Triangle consumers. Production changes stay in
`compat/` and `scene/`; no Game camera, player, or collision algorithm was
changed.

## Original contract

`src/Game/Map/HitInfo.cpp` delegates `Triangle::getHostName` and
`getHostPlacementZoneID` to its CollisionParts. The root
`CollisionParts.cpp` implements these as follows:

- `getHostName` (line 225) checks `mHitSensor`, then its `mHost`, then
  returns that actor's current `mName`.
- `getPlacementZoneID` (line 239) returns `mZone->mZoneID`.
- `init` (line 35) obtains that CollisionZone from the current placement
  zone before camera-code collection.

The native Triangle previously returned its diagnostic KCL resource path
as the host name and returned `-1` for every placement zone. Those values
could never reproduce the original ground-camera key. The old filter tests
also used file paths as if they were actor names.

## Native ownership

`StageCollisionService::Source` and `StageCollisionSurface` now carry an
optional placement zone in addition to the existing sensor and resource
identity. `CollisionPartsCompat` captures the actual
`MR::getCurrentPlacementZoneId()` when registering an original actor's KCL.
The zone survives the end of the construction scope and query rebuilds.
No zone is inferred from the archive name, transform, actor type, or stage.

`Triangle::getHostName` follows the retained sensor to the live host's
`mName`. It deliberately does not copy that name at initialization:
`NameObj::setName` must remain visible just as in the original getter. Null
sensor/host returns null. `source_name` remains the exact archive/KCL path
for diagnostics and is never reused as an actor identity.

`StageCollisionService::surface` already rejects dead, disabled, released,
or absent registrations before returning their sensor. The new getter
therefore does not touch a destroyed owner's storage. Registrations with a
sensor now require their existing shared registration-lifetime object.
Geometry-only fixtures remain supported without an invented host or zone;
a requested unknown zone fails explicitly.

## Construction scope audit

The production `NameObjLifecycleService::construct_and_init` and
`AuthoredPlacementInstantiator` construction adapter already enter
`PlacementZoneNameScope` before original actor initialization. Their scope
uses the placement iterator's real zone and restores the previous checker
state on exit. No production lifecycle changes were needed.

The real-disc `NameObjFactoryPlacementTests` fixture manually called
`InvisiblePolygonObj::init` without its original placement state. It now
installs the actual `PlacementStateChecker` and matching zone scope, then
checks triangle ownership after that scope has ended.

The original `CollisionCategorizedKeeper::getZone` has no root body yet.
The supplied RMGK01 main.dol was inspected at `0x80174b54`, size `0xa0`,
using the existing ignored disassembler. At `0x80174bd0` it directly indexes
`mZones[requestedZone]`; it does not convert an absent `-1` placement to
zone zero. Thus rejecting an unowned production registration preserves the
requirement for an actual placement context. No new decompiled method was
needed for this metadata-only change. `calcFarthestVertexDistance` and
camera-code collection remain outside this tranche.

## Verification

`CollisionTriangleFilterTests` now uses real LiveActor/HitSensor owners,
including different owners sharing exactly the same KCL resource. It checks:

- Actor-based filtering still accepts later contacts and farther rays.
- Two owners retain distinct names and zone IDs despite identical resource
  paths; the diagnostic paths are unchanged.
- Name changes and null sensor-host semantics reach the original getter.
- Rebuilding, death/reappearance, owner destruction, stage clear, and a new
  geometry-only registration preserve or withdraw provenance correctly.
- Missing authored zone data is explicit rather than a fabricated ID.

The optional real-disc FileSelect InvisiblePolygonObj test checks the full
original actor initialization -> CollisionParts registration -> queried
Triangle path, and verifies withdrawal after actor destruction.

Both updated targets compiled and passed on macOS arm64:
`smg-pc-collision-triangle-filter-tests` passed the filters, geometry, and
owner-provenance cases; `smg-pc-nameobj-factory-placement-tests` passed all
four cases with `SMGPC_REAL_DISC` set to the supplied RMGK01 RVZ, including
the real FileSelect wall lifecycle. The latter output is retained locally
in `factory-tests.log`. `git diff --check` passed. The targets needed no
build-file changes.
