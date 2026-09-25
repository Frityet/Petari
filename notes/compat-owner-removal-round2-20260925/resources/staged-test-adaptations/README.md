# Minimal committed-source resource/camera test adaptation

These saved C++ files derive from committed source at `93b469e438c2eea1afbde6e4613d3eb6cbcd75e5`. Root may use each top-level saved C++ file as the replacement index blob for `tests/<basename>`. `head/` retains the exact base; `resource-head-base-migration.patch` contains only the scoped delta.

Neither working test nor the Git index was written. `validation.json` records before/after working-file SHA256 hashes; both were unchanged. The larger preexisting working rewrites remain separate.

- NameObjFactoryPlacementTests: remove the StageResourceBinding include and unused construction (two removed lines). No replacement stage catalog exists: production already uses original StageDataHolder, and the removed compatibility object had no production query consumer.
- OriginalCameraDirectorTests: remove both retired provider includes and the unused StageResourceBinding construction. Replace the CameraUtilCompat wrapper call with the existing actual camera owner's `retain_animation`, then call original `MR::declareEventCameraAnim` within the actual Game allocation domain. Explicit host/Game scopes preserve CANM storage and chunk allocation ownership. The source bytes are still overwritten afterward to test retained animation lifetime.

No new helper provider, fake stage object or catchall service was added. No build or runtime test was run on these saved variants.

## Limits of this minimal publication delta

The older committed fixtures have preexisting lifecycle limitations: they still construct standalone SceneExecutionFixture graphs without publishing an actual original GameSystem scene controller or bootstrapping its StageDataHolder. Removing the inert stage binding cannot provide those missing prerequisites. Full runtime validation requires the actual process rewrites already present separately in the working tests; this patch does not claim those old fixtures pass.

Against the current broader dirty working source, the saved NameObjFactoryPlacementTests also refers to removed `scene/StagePlacementPreflight.hpp`; the saved OriginalCameraDirectorTests refers to removed `scene/SceneExecutionService.hpp` and `RuntimeContext::scene_execution`. Those older interfaces still exist in the stated HEAD base. This source adaptation intentionally does not fold unrelated scene-execution/preflight migration into the resource/camera-provider deletion commit. Root must keep their separate publication scope coherent.
