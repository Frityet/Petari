# Remove compat services and restore original owners

Removed **31 compat files**, reducing `src/compat/` from 160 to **129 files**. The full removal goal remains active.

- Deleted the unused parallel stage/session/scenario/zone services and obsolete StarPointer scene binding. Original GameSystemSceneController, ScenarioDataParser, StageDataHolder, and GameDataTemporaryInGalaxy supply that state.
- Restored complete donor ModelUtil, DrawUtil, JMapUtil, and JointUtil. This replaces partial implementations and restores original model bounds arithmetic and silhouette/draw helpers.
- Removed ModelManagerOwner: actual ModelManager retains its heap, resource leases, and original model/animation dependencies through draw retirement.
- Moved necessary CollisionParts, KCollision, and DynamicCollisionObj native fixes into their original Game owners; enabled the previously excluded sources and retained their floating-point rules. JUTTexture capture now belongs to the existing SDK implementation.
- Restored two currently used scene draw methods at their proper MapObj owners. Entire actor implementations are not claimed.
- Migrated fixtures that depended on deleted services to the actual process, and retired obsolete synthetic binding contracts. Five mixed LiveActorUtil functions remain in its existing shim pending full owner restoration.

## Reduced verification

At the user's request, no focused test suite or new test campaign was run. `xmake build smg-pc` passed after native declaration/include corrections. One fresh-save real-disc Metal opening run completed **120 frames and exited 0**, with no surviving process. Binary SHA-256: `deb0f63a076c34136cc283bb74678fd4c0a080f59cb63c093189ca3aa6697f87`. See `validation.json`, `build.log`, and `gateway-120.json`/`.log`. No screenshot was requested during this run.

This verifies the integrated working tree's bounded opening, not Rosalina or full Gateway completion. Migrated focused fixtures were not run. Existing unrelated working changes remain, so this is not a clean-checkout attestation.

## Preservation and publication

Changes are staged through a temporary index. The preexisting staged route-note patch remains untouched. The already-dirty collision-owner test is intentionally adopted as part of the required deleted-API migration; only this batch's wiring delta is committed from the dirty tests/xmake.lua. Detailed ownership/snapshots are recorded by each lane; only selected notes/evidence are published, excluding before-copy trees and NAND data.

Previous turn was progress: published e02746442 after eight removals. This larger batch follows the user's faster iteration instruction.
