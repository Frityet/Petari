# Round16 orphan scene catalog removal

Baseline `16d91cbb0` (full hash in owned-manifest.json). Sixteen paths were snapshotted before edits; all were clean at handoff. Exact before/after snapshots and `catalogs-only.patch` preserve this lane's changes before collision-agent follow-ups.

Deleted eight obsolete scene files: StagePlacementResolver, PlacementZoneScope, nameobj/ObjectNameTable, and OriginalPlacementCoverage pairs. The first three pairs had no production callers. SceneFunction's sole debug coverage include/invocation was removed, leaving its original SceneDataInitializer placement call. Real StageDataHolder/PlacementInfoOrdered/PlacementStateChecker/name-table behavior was not changed or replicated.

Minimal existing-fixture cleanup:

- Deleted ObjectNameTableTests.cpp and OriginalPlacementCoverageTests.cpp, which only tested the removed wrappers.
- OriginalProcessPlacementTransformTests keeps original process, StageDataHolder matrix/rotation, rail, restart, actual WarpPod, and switch-area checks. Removed optional report/provenance assertions and the duplicate resolver/world-metadata comparison block.
- AreaObjRealOrAbsentTests retains independent area owner/manager/forms/query checks. Removed the three old resolver-based CubeCamera/MessageArea/SwitchArea disc cases and their unused disc-finder helper. No replacement fixture was written.
- SphereSelectorRealOrAbsentTests retains its three independent existing cases and removes the resolver-based FileSelect-row case/disc-finder helper.
- NameObjFactoryPlacementTests drops the resolver-only diagnostic block. Its original-process wall lifecycle now obtains the existing FileSelect common object table through actual FileLoader/MR::mountArchive + JMapInfo and finds the authored InvisibleWall10x10 row. Collision assertions remain for the collision lane's migration.
- Both NameObjFactoryPlacementTests and OriginalCollisionPartsOwnerTests replace the obsolete scope with a tiny local restore of actual MR placement-zone state. No new runtime helper was introduced.

Root build wiring: remove targets `smg-pc-object-name-table-tests` and `smg-pc-original-placement-coverage-tests` from tests/xmake.lua. Existing Game recursive source selection automatically drops the eight deleted files. No xmake files were edited here.

Coordination: the two collision-related fixtures were handed back to gateway_gap_audit after these snapshots. The collision agent will capture this result before modifying its own collision/child-owner calls. Production is frozen; no broad factory edits started in this batch. The complete donor restoration split remains in round15/round16-scene-catalog-split.md; missing-file manifest was captured before root's central header and GCapture/Ribbon/Spring imports.

Validation boundary: no builds or tests run. Scoped whitespace check passed, and source/test text search found no surviving references to the deleted scene APIs except the two test target definitions awaiting root removal. Retiring service-specific assertions is a reduction of obsolete wrapper coverage, not evidence of new gameplay success. Root owns integration build and short smoke.
