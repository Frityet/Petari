# Original GameSequenceProgress direct native dependency audit

The complete reference TU compiles unchanged to an arm64 native object with LLVM 23.1.0, C++23, `-O2 -g` and the current debug definitions. All 25 declared original methods have source bodies. No recovery or production source change was needed for this probe. This is compilation and a direct emitted-symbol inventory, not linked or running sequence integration.

## Compile frontier

Scratch source and seven full original headers are under `probe/`. Every copy is byte-identical to its reference file, with hashes in `progress-audit-manifest.json`.

Six headers are absent from native: `GameSequenceProgress`, `PlayerMissLeft`, `FindingLuigiEventScheduler`, `GalaxyCometScheduler`, `GameSystem`, and `LuigiLeftSupplier`. The seventh is original `GameSequenceFunction.hpp`: native currently declares only its save-oriented subset. With six headers supplied, compilation reports 19 diagnostics covering 12 absent original sequence function declarations (some diagnostics repeat calls or are follow-on suggestions). Replacing that header inside the scratch overlay yields object compile exit 0. No other declaration changes are required for this TU.

The exact command is in `probe-command.json`; run its argument list from its recorded `cwd`. Each successive compile and overlay manifest is recorded in `probe-history.json`. Three existing LiveActor override warnings remain; no errors.

## Direct symbol frontier

The optimized object has **106 direct undefined symbols**. Of these, **67 have definitions in the current debug Game archive**; the same 39 remaining symbols also lack definitions in the current showcase. Three are ordinary C++ runtime imports (`_Unwind_Resume`, `__gxx_personality_v0`, and the ABI class-type-info vtable), leaving **36 missing project definitions** below. Archive and executable hashes are recorded in `direct-symbols.json`, alongside all 106 exact mangled symbols and availability bits. This inventory does not expand the imports of these missing providers, analyze their virtual tables, or infer which functions a specific dead-stripped executable retains.

| Missing direct project symbol | Existing original source body |
| --- | --- |
| `PlayerMissLeft::PlayerMissLeft(char const*)` | `decomp/src/Game/Screen/PlayerMissLeft.cpp:21` |
| `GameDataFunction::addStockedStarPiece(int)` | `decomp/src/Game/System/GameDataFunction.cpp:384` |
| `GameDataFunction::resetGameDataGoToGalaxyFirst()` | `decomp/src/Game/System/GameDataFunction.cpp:306` |
| `GameDataFunction::resetGameDataGoToGalaxyRetry()` | `decomp/src/Game/System/GameDataFunction.cpp:315` |
| `GameDataFunction::onGalaxyScenarioFlagAlreadyVisited(char const*, int)` | `decomp/src/Game/System/GameDataFunction.cpp:56` |
| `SceneControlInfo::setStartIdInfo(JMapIdInfo const&)` | `decomp/src/Game/System/GameSystemSceneController.cpp:65` |
| `SceneControlInfo::setScene(char const*)` | `decomp/src/Game/System/GameSystemSceneController.cpp:57` |
| `SceneControlInfo::setStage(char const*)` | `decomp/src/Game/System/GameSystemSceneController.cpp:61` |
| `LuigiLeftSupplier::syncWithFlags()` | `decomp/src/Game/System/LuigiLeftSupplier.cpp:7` |
| `LuigiLeftSupplier::LuigiLeftSupplier()` | `decomp/src/Game/System/LuigiLeftSupplier.cpp:4` |
| `GameSystemFunction::isResetProcessing()` | `decomp/src/Game/System/GameSystemFunction.cpp:102` |
| `GameSystemFunction::tryToLoadSystemArchive()` | `decomp/src/Game/System/GameSystemFunction.cpp:72` |
| `GameSystemFunction::setAutoSleepTimeWiiRemote(bool)` | `decomp/src/Game/System/GameSystemFunction.cpp:211` |
| `GameSystemFunction::setResetOperationApplicationReset()` | `decomp/src/Game/System/GameSystemFunction.cpp:106` |
| `GalaxyCometScheduler::syncWithFlags()` | `decomp/src/Game/System/GalaxyCometScheduler.cpp:320` |
| `GalaxyCometScheduler::restoreStateFromGameData()` | `decomp/src/Game/System/GalaxyCometScheduler.cpp:338` |
| `GalaxyCometScheduler::update()` | `decomp/src/Game/System/GalaxyCometScheduler.cpp:294` |
| `GalaxyCometScheduler::GalaxyCometScheduler()` | `decomp/src/Game/System/GalaxyCometScheduler.cpp:287` |
| `GameSequenceFunction::startScene()` | `decomp/src/Game/System/GameSequenceFunction.cpp:106` |
| `GameSequenceFunction::isReadyToStartScene()` | `decomp/src/Game/System/GameSequenceFunction.cpp:102` |
| `GameSequenceFunction::getClearedStarPieceNum()` | `decomp/src/Game/System/GameSequenceFunction.cpp:142` |
| `GameSequenceFunction::isPowerStarAtResultSequence(char const*, int)` | `decomp/src/Game/System/GameSequenceFunction.cpp:160` |
| `GameSequenceFunction::resetStageResultSequenceParam()` | `decomp/src/Game/System/GameSequenceFunction.cpp:199` |
| `GameSequenceFunction::storeSceneStartGameDataHolder()` | `decomp/src/Game/System/GameSequenceFunction.cpp:281` |
| `GameSequenceFunction::reflectStageResultSequenceCoin()` | `decomp/src/Game/System/GameSequenceFunction.cpp:190` |
| `GameSequenceFunction::updateGameDataAndSequenceAfterStageResultSequence()` | `decomp/src/Game/System/GameSequenceFunction.cpp:287` |
| `GameSystemSceneController::requestChangeScene()` | `decomp/src/Game/System/GameSystemSceneController.cpp:87` |
| `GameSystemSceneController::startScenarioSelectScene()` | `decomp/src/Game/System/GameSystemSceneController.cpp:279` |
| `GameSystemSceneController::startScenarioSelectSceneBackground()` | `decomp/src/Game/System/GameSystemSceneController.cpp:283` |
| `FindingLuigiEventScheduler::updateOnStageResult(char const*, int)` | `decomp/src/Game/System/FindingLuigiEventScheduler.cpp:48` |
| `FindingLuigiEventScheduler::clearLostAndFoundCount()` | `decomp/src/Game/System/FindingLuigiEventScheduler.cpp:156` |
| `FindingLuigiEventScheduler::initAfterResourceLoaded()` | `decomp/src/Game/System/FindingLuigiEventScheduler.cpp:20` |
| `FindingLuigiEventScheduler::update(GalaxyMoveArgument const&)` | `decomp/src/Game/System/FindingLuigiEventScheduler.cpp:25` |
| `FindingLuigiEventScheduler::FindingLuigiEventScheduler()` | `decomp/src/Game/System/FindingLuigiEventScheduler.cpp:17` |
| `MR::startGlobalTimer()` | `decomp/src/Game/Util/ScreenUtil.cpp:357` |
| `MR::requestChangeSceneAfterBoot()` | `decomp/src/Game/Util/SequenceUtil.cpp:59` |

`MR::startGlobalTimer` is an empty original body, and the retail reference confirms that this is intentional: `ScreenUtil.s:1227`, address `803F8E4C`, contains only `blr`. Its absence therefore needs source activation, not a guessed timer implementation. The other listed functions have bodies in the referenced TU; their individual Wii fidelity and transitive native dependency closure were not re-proven in this audit.

## Link presence does not establish working behavior

Six direct imports already resolve to deliberate `unavailable(...)` throws in `src/compat/StorySequencePlatformCompat.cpp`: `GameSequenceFunction::{hasStageResultSequence,getClearedStageName,getClearedPowerStarId,hasPowerStarYetAtResultSequence}`, `GameSystemFunction::setPermissionToCheckWiiRemoteConnectAndScreenDimming`, and `MR::requestChangeSceneTitle`. The permission setter is unconditionally called by `GameSequenceProgress::startScene`, so merely filling the missing link symbols cannot complete startup. There are additional adjacent shadow providers, including a process boolean for comet activity; those are not counted as direct unresolved imports here.

The scratch object defines `SingletonHolder<GameSystem>::sInstance` through the original template. That definition is zero-initialized pointer storage, not a constructed GameSystem. Progress directly dereferences its actual `mSceneController`; original GameSequenceFunction also routes through `mSequenceDirector` to progress, temporary galaxy data and save owners. There must be a real initialized original owner graph, not a cast of host services to GameSystem or another duplicate StorySequenceExecutor.

## Coherent integration direction

1. Import the complete original Progress source and six missing headers, restoring the full sequence-function declaration surface. This fixes only the measured compile boundary.
2. Integrate the actual GameSystem/sequence-director/scene-controller lifetime and attach the existing real StorySequenceExecutor, save-data, scene and pointer ownership through their original owners. Parent and the lifecycle audit own this decision.
3. Activate original child providers (PlayerMissLeft, FindingLuigiEventScheduler, GalaxyCometScheduler, LuigiLeftSupplier) and original sequence/result providers, removing duplicate facade implementations as each whole original owner becomes available. FindingLuigi creates a real LuigiMailDirector; the comet scheduler creates its time/state graph; those additional imports require a separate measured probe rather than assuming this 36-symbol list closes them.
4. Replace the permission/reset/archive platform frontier through real original owners and generalized platform services. Original `startScene` must run before original scene-controller `startScene`, so its conditional spin permission and story/pointer policies occur at their authored boundary.
5. Then link and execute a focused real-owner startup proof. No such runtime improvement is claimed by this audit.
