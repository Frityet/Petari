# Exact FileSelect invisible-wall closure

Date: 2026-08-08 UTC

## Outcome

The retail `InvisibleWall10x10` row in RMGK01 `FileSelect` is now an exact Game-source actor backed by a generalized host ResourceHolder/CollisionParts boundary. No FileSelect-specific archive lookup or collision path exists in compat. Strict placement remains fail-closed and moves honestly from three actor blockers to two: `FileSelector` and `SphereSelectorHandle`.

The complete retail `InvisiblePolygonObj` factory/archive family was exposed because it shares the same real source and generalized provider:

- `GhostShipCavePipeCollision`
- `InvisibleWall10x10`, `InvisibleWall10x20`
- `InvisibleWallJump10x10`, `InvisibleWallJump10x20`
- `InvisibleWallGCapture10x10`, `InvisibleWallGCapture10x20`
- `PolygonCodeRecoveryPlate`, `PolygonCodeRecoveryBowl`

## Source boundary

These port files are byte-identical to the root decomp, including terminal-newline state:

- `src/Game/MapObj/InvisiblePolygonObj.hpp`
- `src/Game/MapObj/InvisiblePolygonObj.cpp`
- `src/Game/MapObj/InvisiblePolygonObjGCapture.hpp`
- `src/Game/MapObj/InvisiblePolygonObjGCapture.cpp`

Hashes are recorded in `verification.log` and enforced by `smg-pc-game-source-mirror-tests`.

## Generalized compatibility work

- `ResourceHolderService` retains exact requested RARC archives through `DvdFileSystemService`; missing archives fail and no actor/placement-name inference exists.
- `MR::createAndAddResourceHolder`, `MR::initCollisionPartsFromResourceHolder`, and `MR::getCollisionBoundingSphereRange` are supplied outside Game.
- CollisionParts retains the real KCL and optional PA spans, placement matrix, source identity, sensor, bounding radius, and a shared registration lifetime.
- `StageCollisionService::register_kcl` returns the decoded KCL farthest-vertex radius and binds triangles to an owner registration. Dead actors stop contributing; appearing actors resume; release before actor destruction prevents a dangling dead-flag pointer.
- The exact `makeMtxTransRotateY` scalar and LiveActor providers live in `compat/MtxCompat.cpp`; the LiveActor overload matches the root decomp body.
- The generic stage integration point builds registered collision before
  `initAfterPlacement` so exact callbacks can query it, then rebuilds once more
  after every SceneObj/root callback to include registrations made there.

The SMG KCL decoder/query is already an existing host-side stage service. Moving it wholesale into Aurora would not have been a bounded primitive extraction in this slice; the new archive and actor ownership APIs remain general and are not tied to FileSelect or the invisible-wall family.

## Real RMGK01 evidence

The focused lifecycle test reads the real `/ObjectData/InvisibleWall10x10.arc` and asserts:

- `InvisibleWall10x10.kcl`: 1222 bytes
- `InvisibleWall10x10.pa`: 96 bytes
- `CollisionVersion`: 7 bytes
- no map hit before the post-placement BVH build
- a real map hit after build
- dead owner: no hit
- appeared owner: hit restored
- destroyed owner: no hit

The strict construction probe report is retained in `strict-file-select-stage-placement.md`. Its expected nonzero exit is the proof that no root actors are fabricated while two retail actors remain unsupported.

## Remaining exact closure ranking

1. `SphereSelector` and `SphereSelectorHandle` are now reconstructed and
   mirrored exactly on PC. Their generalized SceneObj, pointer, layout, camera,
   matrix, and teardown services are present, but the Handle remains explicitly
   unavailable because retail requires real atmosphere-level sound playback on
   rotating frames. The host has no such playback backend; it throws instead of
   logging a silent success.
2. `FileSelector` has much stronger decomp status (192/209 functions, 74.48% code, 97.41% fuzzy) but its mandatory retail lifecycle still owns the selector child graph, TitleSequenceProduct, Mario autorush, save/RFL, camera, and item transitions. The real JKRMemArchive/nw4r ResFont layer now exists, but that is only one compile/runtime dependency, not the actor closure.

These two should not be presented as one coherent closure. A global MAIN TitleSequence host would bypass retail FileSelector ownership and was rejected. The retained strict report was regenerated after the Sphere work and records its precise audio blocker.
