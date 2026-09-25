# Compat and scene removal, round 16

Replace the duplicate scene NameObj registry with registration back-references on the actual NameObj and ordered removal on its actual NameObjHolder. Delete the unused child-owner helper and its registry capture/delegation policy. Delete StageCollisionService and its duplicate geometry/transform caches; the actual CollisionParts and original keeper zones supply collision data and generated-data validation.

Remove unused placement resolver, zone scope and object-name catalog plus the optional alternate placement-coverage report. The original StageDataHolder, PlacementInfoOrdered and PlacementStateChecker remain authoritative.

Import 691 missing Game/JSystem/nw4r donor headers without overwriting native files. Restore the complete original GCapture, GCaptureRibbon and SpringValue translation units and actual capture-target interface, deleting the query-only compat provider. Replace four custom flat-array-algorithm calls with ordinary loops in their existing Game functions and delete the injected std helper.

This is an incremental removal batch; the remaining scene factory/catalog and lifecycle/draw services still need replacement with complete original implementations. The next broad source import can now use the full declaration surface. No claim is made that imported declarations alone activate actors or that the opening smoke validates blue-star or Rosalina gameplay.

Validation is limited to one integrated application build and one fresh-save 120-frame Gateway opening run, with retries only for concrete failures. No standalone fixture suite or full route is run.

The integrated build passed first try in 37.6 seconds. The fresh-save Gateway opening completed 120 frames, exited 0 and left no process. Binary SHA-256 remained 48c8b57d3b4250c25a19a5199ea08ec5bf6cbd925fe8c4e5f61241ec36dc3256. The batch reduces src/compat from 41 to 39 files and src/scene from 36 to 22.
