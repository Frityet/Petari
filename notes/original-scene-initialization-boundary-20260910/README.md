# Original scene initialization boundaries

The host now exposes separate stage archive start/wait, common actor preload, scenario actor preload, actor construction, camera parameter completion, original scene postpass, draw-list allocation, and final initialization End boundaries. This is integration work for the original GameScene; it does not prove a playable camera, jumping, or a Gateway chase.

## Ownership and order

- `pre_scene_init()` publishes the actual stage session and Scene-owned holder/executor only. The older host aggregate creates its additional services in a separate private helper. No `SceneDataInitializer` or `StageDataHolder` token is installed in a Game SceneObj slot.
- Start loading enumerates every original Scenario ZoneList row in order and mounts each `/StageData/<zone>.arc` against the retained Scene domain. It uses the original FileSelect/Epilogue pause predicates to decide PauseMenu loading. Wait publishes embedded `/arc` files with independent retained decoded bytes and preserves existing mounted identities. The scene removes its mounts during retirement while retaining its heap.
- Common preload snapshots/sorts and requests the two common holders; scenario preload subsequently snapshots/sorts and requests the three scenario/deferred holders. The existing placement order and failure behavior remain in the shared instantiator.
- Production post-placement uses the real `NameObjHolder::callMethodAllObj(&NameObj::initAfterPlacement)` once. A snapshot of the original holder's captured range is used only to acknowledge completion in the native ownership/reporting layers. These acknowledgements release delegation and update accounting without running callbacks again. Objects registered during the original call remain outside its captured end. A failed original callback does not permit replaying completed earlier callbacks.
- Area owner acknowledgement finalizes only managers included in that captured range and does not replay their original callbacks.
- Constructing the actual CameraDirector SceneObj now publishes a native adapter borrowing that director and its actual CameraContext. It no longer depends on the demo invoking a separate camera owner constructor. The adapter retains animation bytes and exposes the original view; it does not choose a camera or calculate motion.
- `MR::completeCameraParameters()` closes original camera parameter creation and publishes camera readiness independently from scene End. `SceneObjHolderBinding::complete_initialization()` now changes only initialization state. The legacy Gateway host and camera test call camera completion explicitly.
- `finalize_scene_initialization()` is End only and requires completed actual execution-list allocation. Current reference `GameSystemSceneController::initializeScene` calls `mScene->init()` then allocation; `exeInitializeScene` assigns End after that function completes. It does not repeat Sleep synchronization, appearance, camera closure, or placement.

## Verified evidence

`native-syntax.json` records 16 successful LLVM 23 syntax checks covering every changed implementation and four changed tests, with exact command arrays and source hashes. Old package hashes were stale; the probe resolves missing include paths against currently installed Xmake packages.

`registry-result.json` records successful compilation of five actual implementation/test objects for the original holder, registry, native NameObj lifetime, registration metadata, and the updated registry fixture. Its link did not run because the old test compatibility object and old root game/common archives are absent in the current build tree. No root Xmake was run by this agent. Runtime claims must await the parent's coordinated build.

Updated fixture coverage:

- `SceneNameObjRegistryTests`: original global callback order, callbacks appending new registrations outside the captured end, and existing 16-generation heap/cache coverage.
- `AuthoredPlacementInstantiatorTests`: split common/scenario preloads; incomplete acknowledgement rejection without consuming state; complete descendant accounting without callback replay.
- `OriginalCameraDirectorTests`: direct original SceneObj creation publishes the native adapter, and `MR::completeCameraParameters` readies the original camera while initialization End remains false.
- `StageInitializationResourceTests`: start/wait separation, real ZoneList archive lifetime, embedded archive copy/identity, and three partial initialization retirements.

## Remaining real-owner frontiers

`SceneFunction::initForLiveActor()` is intentionally not replaced by a partial list. Its original required graph includes AllLiveActorGroup, ClippingDirector, DemoDirector, SensorHitChecker, CollisionDirector, MessageSensorHolder, LiveActorGroupArray, MovementOnOffGroupHolder, LightDirector, AreaObjContainer, CaptureScreenActor, StageSwitchContainer, SwitchWatcherHolder, SleepControllerHolder, TalkDirector, and NPCDirector. Several still lack genuine active original implementations. Returning successfully after constructing only a subset would falsely claim this boundary is complete.

`SceneFunction::initAfterScenarioSelected()` is also not provided as a resource-only shortcut. Original SceneDataInitializer first requests the LuigiLetter menu archives, then initializes the selected scenario data. `LuigiLetter::makeArchiveListForMenu` calls `MR::getLuigiLetterGalaxyName()`. The missing reference method is visible in retail EventUtil assembly at 0x803CEB80 (160 bytes). It depends on:

1. Actual selected scenario and stage queries.
2. `GameDataConst::isPowerStarLuigiHas`, which traverses the original GameEventFlag table via `isPowerStarSpecial` using `SpecialStarFindingLuigi`.
3. Genuine `GameSequenceProgress::mFindingLuigiEventScheduler` queries `isDisappear`, `isHiding`, and `getHidingGalaxyNameAndStarId`.
4. The latter's original saved `LuigiEventState`, `SpecialStarFindingLuigi1..3` flags, and event-table galaxy/star metadata.

Current `StorySequencePlatformCompat::isLuigiDisappearFromAstroGalaxy()` throws an unavailable-owner error. It cannot justify a false/null LuigiLetter result. The port EventUtil file already contains a plausible body, but the reference still marks this method missing, so it must be recovered/verified in decomp before reuse. `initialize_scenario_resources()` remains a typed host resource boundary; it is not represented as the complete original `initAfterScenarioSelected()`.

The original scene object graph and progression dependencies must be completed before the new demo can be verified and repackaged.
