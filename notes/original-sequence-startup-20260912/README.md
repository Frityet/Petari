# Original sequence and GameScene startup forwarding

The native build can now use complete reference `SequenceUtil.cpp` (23 methods) and `GameSceneFunction.cpp` (16 methods), plus the complete small `PlacedHiddenStarScenarioTable.cpp`. All three are byte-identical to `decomp/`; no reference recovery or Game behavioral edits were needed. The table keeps its original HeavenlyBeachGalaxy scenario 6 → 3 entry and returns -1 for other inputs.

The concrete live startup methods restored here include `MR::requestChangeSceneAfterBoot`, `requestChangeStageAfterStageClear`, `hasRetryGalaxySequence`, `executeOnWelcomeAndRetry`, and `GameSceneFunction::loadScenarioWaveData`. These now follow the original GameSequence/GameScene owners. `executeOnWelcomeAndRetry` starts the actual comet event and requests the actual HP meter appearance. Scenario sound loading is delegated to the actual scene controller.

## Removal and ownership

Removed `OriginalGameSceneFunction.cpp` and `SequenceUtilCompat.cpp` entirely. Removed only three duplicated StorySequencePlatformCompat definitions (`MR::isExecScenarioStarter`, `requestChangeScene`, `requestChangeSceneTitle`) and the resulting unused StageSessionState include. Root's earlier removal of `MR::isStarCompleteAllGalaxy` is preserved; the incremental patch against the pre-task WIP is `StorySequencePlatformCompat.owned.patch`.

Removed GameSceneBinding's action/query enums, function-pointer dispatch and query delegates, and public lookup/dispatch/query APIs. The binding still tracks one actual GameScene lifetime, captures its raw children before derived destruction, and retires unclaimed NameObj children and cached original controls at the Scene base retirement boundary. SceneLifecycleService is its sole caller and compiles against the reduced API. No actor, scene, or process identity was fabricated.

The original GameSceneFunction getter obtains `SingletonHolder<GameSystem>::get()->mSceneController->mScene`. The original sequence functions obtain the real GameSequenceProgress through GameSequenceFunction. These functions require completed original owner construction, as on Wii; they no longer offer the former missing-owner proxy API. Calling them with no GameSystem is outside their original contract. Startup validation must construct the actual process graph first.

## Build handoff

Root should remove exactly these two exclusions in `src/Game/xmake.lua`:

- `remove_files("Scene/GameSceneFunction.cpp")`
- `remove_files("Util/SequenceUtil.cpp")`

The new System table is included by the existing source glob. Both original headers already match the reference and are unchanged. `script/game_execution_charset.lua` was audited: its current generic Clang token wrapper has no generated source subsets or individual function providers. No edits were made to that shared script or the shared build files.

`source-manifest.json` records the seven changed/deleted production paths, the unchanged GameSceneFunction source to activate, source hashes, reference identities, and narrow shared-file scope.

## Validation and limits

All six isolated native translation-unit compilations passed: the three original TUs, StorySequencePlatformCompat, GameSceneBinding, and its existing SceneLifecycleService caller. All three original TUs freshly compiled using the original Metrowerks compiler. Exact argument vectors, logs, objects and native symbol tables are retained here. Existing native compiler warnings are preserved in logs; there were no errors. This is source/compile evidence, not linked process startup or Gateway gameplay proof. Root owns the integrated startup build and run.

The source provider audit found no remaining compatibility definitions for the restored methods. GameSequenceFunction remains the complete original lower provider. Required lower owners are the real GameSystem/GameSequenceDirector/GameSequenceProgress, actual GameSystemSceneController and GameScene, plus existing restart-ID and comet/HP-meter owners. None are replaced by a StageSession field.

An old fixture needs migration: `tests/OriginalSceneCounterOwnerTests.cpp` lines 92–99 calls `MR::isExecScenarioStarter` while changing only StageSession execution phases. That former compatibility assertion cannot prove the restored GameScene nerve state. The test was not changed here and no fake GameSystem was added to keep it passing.

The deleted SequenceUtilCompat was the only production writer to `SequenceRequestService`. Its remaining host state and consumer paths in RuntimeServices, RuntimeContext, SceneTransitionRequestService and ParityTrace were deliberately left out of this direct startup prerequisite. They are now an obsolete service cleanup frontier, not an alternative route used by the restored original utility.
