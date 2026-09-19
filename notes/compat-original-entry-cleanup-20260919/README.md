# Original process entry and scene-owner cleanup

The original `GameSystem` process is now the only application entry. `src/app/main.cpp` calls `run_original_game` directly; `Application` retains only bootstrap configuration and real disc-image lifetime. The old service-provider process graph is removed. DVD image support is now a mandatory Aurora dependency instead of a warning followed by apparent bootstrap success.

Removed the obsolete Gateway spin checkpoint, Gateway showcase scene, title/blank-file selection replicas, and their separate scene controller/initialization/transition owners. Exact file lists are `retired-production-files.json` and `retired-route-tests.json`. The removed showcase's only routes were title, gateway and gateway-spin. Its old stage-construction probe also used the duplicate host lifecycle and is retired. Existing original-process observers/probes, controller replay, renderer, real SceneObj lifetime bindings and subsystem services remain.

Parent owns RuntimeContext removal of the duplicate scene lifecycle plus build/task/packaging cleanup. All existing macOS app bundle and run-hook changes belong to the user and must be retained for the `smg-pc` target. This agent did not stage or commit anything.

## Tests preserved and retired

The twelve retired test programs exclusively instantiated the deleted scene replicas. Their assertions are not claimed as newly passing under OriginalProcess; GPU/model/visual assertions from those programs are not automatically established by this cleanup. Actual-process CollisionArea, shadow, sensor, placement transform, PunchingKinoko, Butterfly, WarpPod, CrystalCage and StarPiece probes remain available. Keep any future rendering regression in the original-process owner graph rather than rebuilding a replacement stage.

The independent `GravityRealOrAbsentTests`, `NameObjFactoryPlacementTests`, `AuroraNativeTests`, `StageStartCameraTests`, `SceneObjHolderRealOrAbsentTests`, and `StorySequenceRealOrAbsentTests` remain. Changes remove only references/assertions about the deleted owners, plus relocate reusable strict placement preflight to `scene/StagePlacementPreflight.*`. The obsolete `should_apply_host_appear` policy and its test are gone. There is no compatibility alias for it.

## Actual original loading coverage

`scene/OriginalPlacementCoverage.*` reads the five actual `PlacementInfoOrdered` queues from the initialized original `StageDataHolder`, in the same order as `StageDataHolder::initPlacement`. It also includes the selected actual Mario start. The report reads retained original JMap links; it neither reparses the scenario nor invents placement rows. It preserves duplicate group occurrences and rejects inconsistent group link counts.

The original `NameObjFactory::getCreator` null behavior is unchanged. An independently generated identity inventory contains all 1,183 names from canonical `cCreateTable`, including original actors with no archive name. Active original PlanetMap catalog membership and model-changing factory descriptions are used as appropriate. This distinguishes supported, known-but-unlinked, and genuinely unknown names without substitute constructors.

StageObjInfo describes zone composition and is marked metadata. **DemoObjInfo is actor-bearing in OriginalProcess**: its canonical DemoGroup row constructs a real DemoExecutor. The old standalone host's separate demo-loader policy is deliberately not reused. Tests cover this distinction and exact basename matching.

At original `SceneFunction::startActorPlacement`, the compatibility boundary emits a summary before delegating to the unchanged original initializer. `SMGPC_ORIGINAL_PLACEMENT_REPORT_PATH` requests detailed JSON. `SMGPC_STRICT_PLACEMENT=1` rejects known-unlinked actors before placement begins; default loading behavior is unchanged, and unknown names retain canonical skipping even in strict mode. A requested report that cannot be written fails explicitly.

`tests/OriginalPlacementCoverageTests.cpp` tests actual original queue insertion/links, duplicate rows, exact metadata filtering, DemoExecutor membership, ordinary/model creator classification, retained row provenance, the strict gate, unchanged unknown-name lookup and malformed list counts. It does not construct a stage or claim all actor dependencies work.

## Validation status

- `python3 scripts/generate_nameobj_catalog.py --check`: PASS, 1,183 canonical factory identities.
- Source reference scan: removed production class names remain only in explicit absence assertions, excluding parent-owned work while it was in progress.
- Owned `git diff --check`: PASS.
- Coordinated parent validation now passes main linkage, original placement queue checks, and a 1,200-frame real Gateway opening. A 360-frame original player regression preserves the useful state/walking/camera/utility assertions. See `../compat-original-runtime-20260919/README.md` and `../compat-original-provider-cleanup-20260919/retained-player-owner-tests.md` for exact artifacts and limits. The default FileSelect scene remains incomplete, and older standalone effect/resource fixtures still lack original process owners.
