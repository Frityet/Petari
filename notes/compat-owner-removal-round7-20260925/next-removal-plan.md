# Next compatibility removal batch

Current working tree: **162 files** under `src/compat` (108 cpp, 53 hpp, one CP932 mapping TSV). Exact paths, line counts, and current dirty status are in `remaining-compat-inventory.tsv`. This is an inventory and bounded next-owner review, not a claim that all remaining files have been audited.

## Recommended: remove obsolete stage/session publication

Delete these **six files / 557 lines** as a coherent batch:

| Files | Duplicate state removed | Actual owner already present |
| --- | --- | --- |
| `StageSessionState.cpp/.hpp` | Second scene/stage/scenario identity, restart/temporary-data object, execution phase, thread-local publication stack | `GameSystemSceneController`, its SceneControlInfo, actual GameDataTemporaryInGalaxy, and GameScene |
| `StageScenarioMetadataResolver.cpp/.hpp` | Separate DVD/RARC scenario decode and manual comet enum; unused stage-audio bootstrap | Actual ScenarioDataParser/GalaxyStatusAccessor/CometEventKeeper, using FileLoader-owned archives |
| `StageZoneMatrixRegistry.cpp/.hpp` | Copied holder hierarchy/matrices and JMap DataCompat-to-holder map; thread-local binding | Original StageDataHolder hierarchy and native-width raw range lookup |

**Evidence:** searches over all source consumers find no production caller outside these six files. Current construction/calls are only in old fixtures. `begin_stage_audio` and `end_stage_audio` have no callers at all. `SceneUtil::getCurrentStageName`, scenario getters, and both `getZonePlacementMtx` overloads already follow their `decomp/src/Game/Util/SceneUtil.cpp` counterparts. `SystemUtil::getPlayerRestartIdInfo` already obtains the actual process temporary data, matching the donor. `GalaxyStatusAccessor::getCometName` and `CometEventKeeper::initCometStatus` already implement the original authored-data semantics, including a missing Comet field.

This batch requires no substitute service and no new Game behavior. The substantive work is replacing fixtures that still assert publication of objects the game no longer uses.

## Finite fixture closure

1. Replace `StageZoneMatrixRegistryTests.cpp` with original StageDataHolder tests through the actual process/scene fixture. Preserve first immediate child lookup, repeated/empty child occurrence order, nested iterator lookup, raw byte range identity and stable original matrix addresses. Retain invalid lookup checks at `StageDataHolder`'s null-returning functions: the original MR facade dereferences a valid lookup and never promised the shim's exceptions. Do not change production to preserve old nested thread-local binding semantics.
2. Replace the stage/metadata sections of `RestartStageSessionTests.cpp` with actual scene-controller and save temporary-data assertions. Keep independent JAISoundID, audio-service, and explicit player capability checks in their proper existing targets. Query both HeavensDoor and FileSelect through the actual parser for the missing-Comet-field contract; no DVD cache reparse is needed. Do not retain the invented unresolved/purple metadata override API just to satisfy this fixture.
3. Migrate `OriginalCollisionPartsOwnerTests`, `OriginalImageEffectOwnershipTests`, and `GameActorPhysicsRealOrAbsentTests` from standalone RuntimeContext + fake session to an actual original process fixture. These already depend on real FileLoader/ResourceHolderManager now; simply dropping an include is insufficient. Preserve rollback/borrow retirement assertions with baselines taken after actual process setup.
4. `OriginalStarPointerOwnerTests` currently uses StageSessionBinding only for its hidden StarPointerSceneBinding side effect. Test original layout creation/mode entry/exit against the actual process pointer director. Nested priority-counter tests can issue original mode requests directly; do not fabricate nested stage sessions. Once this is migrated, remove the now-unused StarPointerSceneBinding class/methods from the existing depth owner files as part of this batch if the last-callsite search confirms it.
5. `OriginalShadowControllerOwnerTests.cpp` has only the obsolete StageSessionState include, so remove that include after verification.

Production deletion risk is low because the six files have no external production consumers. Fixture migration risk is moderate: several old tests bootstrap resources without GameSystem/FileLoader and are already incompatible with current owner boundaries. `tests/xmake.lua` and some named fixtures are dirty, so snapshot and apply precise edits; do not reset whole files. Initial status was captured in the inventory for source files; recheck tests immediately before implementation.

## Validation and wiring

Use existing targets `smg-pc-stage-zone-matrix-tests`, `smg-pc-restart-stage-session-tests`, `smg-pc-original-star-pointer-owner-tests`, `smg-pc-original-collision-parts-owner-tests`, `smg-pc-original-image-effect-ownership-tests`, and `smg-pc-game-actor-physics-real-or-absent-tests`. Actual process variants need `smg-pc-app` and `aurora-main`. Rename fixture/target labels when their former registry-only contract is replaced, rather than retaining an old API path. Then build the app and run fresh Gateway 600 frames **including teardown**. Require zero remaining source references to the removed owners; do not interpret a test compile alone as runtime coverage.

## Subsequent active-owner batch, not part of this deletion

`ModelManagerOwner.cpp/.hpp` now contains only actual Game/SDK ownership and two ResourceHolder native tokens; the former ResourceHolderService dependency is gone. It is a strong subsequent consolidation candidate, but not simply deletable: ActorRuntimeRegistry, SceneDrawBufferService, SceneScheduler, MarioAnimatorLifetime, LiveActor, and shadow lifetime tests retain its shared model/heap lifetime. Canonical ModelManager already exists and follows the donor. A correct follow-up puts native child/resource retirement into that actual class and lets packet owners retain the actual model state, eliminating the parallel manager wrapper. Preserve captured original XanimePlayer/Core restoration and Mario animator dependency order. This needs model/packet/animator lifetime regressions and a separate checkpoint.

Also available afterward: restore the missing full canonical `Game/Util/ModelUtil.cpp` from the existing 643-line donor, replacing ModelCreation/PostLoad/Fog and OriginalModel fragments with one owner. This is provider consolidation, not a reason to delete ModelManager's live resource borrows; verify all symbols and native J3D assumptions independently.
