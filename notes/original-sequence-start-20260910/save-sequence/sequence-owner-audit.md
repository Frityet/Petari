# Original sequence ownership integration

Current sources, independently read on 2026-09-10. This report intentionally does not implement a partial GameSystem or copied Progress startup routine.

## Original owner graph

`GameSystem::GameSystem()` is an inert original constructor. `GameSystem::init()` constructs the actual process graph, including GameSystemObjHolder, GameSequenceDirector, GameSystemSceneController, system error/dimming/reset owners and stationed resources. The original source also contains the platform `main` and frame loop, so activating it whole needs a native entry/platform adaptation; just filling its pointers from unrelated host services would not establish its actual init contract.

`GameSequenceDirector` owns SaveDataHandleSequence, GameSequenceProgress, **process-lived** GameDataTemporaryInGalaxy, and NWC24Messenger. It registers save callbacks before constructing Progress. `GameSequenceFunction.cpp` accesses these through the actual GameSystem singleton and its original members. A standalone Progress cannot replace this owner graph while leaving that original TU unchanged.

Progress owns StarPointerOnOffController, StorySequenceExecutor, FindingLuigiEventScheduler, and LuigiLeftSupplier from construction. Its resource-loaded phase additionally owns GalaxyCometScheduler and PlayerMissLeft, and initializes the finding-Luigi mail owner. `LuigiLeftSupplier.cpp` is a particularly small whole-source prerequisite (constructor plus `syncWithFlags`, depending only on original GameDataFunction::isDataMario), but activating it alone does not make Progress resource-ready.

GameSystemSceneController construction creates actual IntermissionScene, PlayTimerScene and ScenarioSelectScene in addition to its NameObjHolder and original nerve state. Its resource initialization requires their actual resources; its stage initializer uses the real scene heap, ScenarioDataParser, WPad reset and scene factory. These are original owners to bring forward, not scene-name placeholders to substitute.

## Existing ownership to consolidate

| Current host owner | Actual original owner/action |
| --- | --- |
| `StarPointerDepthOwnership::State` constructs a separate StarPointerOnOffController, and its update calls that controller | Borrow `GameSequenceProgress::mStarPointerOnOffController`; preserve host pointer-depth/input work but remove the duplicate mode owner and duplicate update. |
| `SceneTransitionRequestService` owns its own StorySequenceExecutor and selects native StageHostRequests | Use the actual Progress child for original move policies; make the host service transport/platform lifetime only and retire its conflicting policy. |
| `StageSessionState::TemporaryData` constructs GameDataTemporaryInGalaxy per scene | Borrow the actual Director's process-lived temporary data so stage-result/retry state survives scene replacement. |
| GameDataSession has host-owned selected/current scene-start data | Bind to the original SaveDataHandleSequence current and backup UserFiles; original backup is a distinct serialized snapshot. |
| Native SceneControlInfo/controller fields are maintained in parallel | Read the actual GameSystemSceneController current/next records; preserve original selected-scenario `_C`, start ID and move type when transporting host requests. |

Existing actual GameScene ownership is useful: `SceneLifecycleService` constructs and initializes the original `GameScene` under a scene heap and has explicit original object teardown. It does not justify a second synthetic scene controller.

## Startup order that must change together

Current `SceneLifecycleService::initialize_scene` calls `start_scene()` immediately after initialization. The host scene controller then enters ReadyToStartScene and Normal in the same `apply_pending_change` call. Retail keeps ReadyToStartScene until the original sequence observes readiness (including scenario selection), performs `GameSequenceProgress::startScene()`, then calls `GameSequenceFunction::startScene()` and the original controller's `startScene()` to start the actual Scene.

That Progress operation resets flags, selects real StarPointer modes, starts story sequence state, controls system permission/dimming, restores wipes, kills PlayerMissLeft and applies the original spin-story permission. The missing sequence startup explains the demo fixtures' explicit precondition setup; do not elevate those fixture assignments into host gameplay policy.

Original process update order also matters: after input/system checks, GameSequenceDirector updates before the Scene's update. Current RuntimeContext::begin_frame executes the actual scene and only later GameSystemService updates SequenceBootService. Native frame services must be separated from original gameplay execution to admit the real update order, with each owner updated once.

## RequestGalaxyMove scope

The recovered original function was previously compiled against Wii and scored 94.47305%; see `notes/original-sequence-galaxy-move-20260910/README.md`. Its actual integration needs Luigi/finding-Luigi, StorySequenceExecutor, GalaxyCometScheduler, stage-result and actual save-data effects, plus the original controller and PlayerMissLeft lifecycle. Comet synchronization and stage-result queries occur even on ordinary moves; unsupported branches cannot be assumed away by the Gateway demo setting.

The current host StorySequence-to-StageHost conversion also loses selected scenario and move type, and its request path publishes current/next metadata differently. Activating only the original move method while retaining that duplicated host policy is unsafe.

## Concrete next cohort

The bounded SaveDataHandleSequence/NANDErrorSequence probe in the adjacent README gives precise compiler and provider evidence. Both whole original TUs compile with only a missing exact Game header and four SDK constants. They need the original reset service methods to link, and the real six-chunk GameDataHolder/data cohort to initialize correctly. These are substantial but concrete original prerequisites, unlike an owner shell or copied startScene body.

After the save/data owners are operational: activate whole Progress/Director/sequence utility sources with the real mail/comet/reset prerequisites, move persistent ownership to the original graph, and replace host start/update scheduling in the same coherent integration. No existing proof here establishes full GameSequence startup or persistent saves.

One small missing provider should not be misclassified: original `MR::startGlobalTimer()` is actually empty in retail (`ScreenUtil.s`, address 803F8E4C, size 4, `blr`), rather than an undecompiled timer implementation. Whole ScreenUtil is currently excluded; its eventual activation needs appropriate original source ownership.
