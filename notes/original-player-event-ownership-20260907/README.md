# Original player-event owner activation

Imported all seven complete original PlayerEvent TUs and seven typed headers, registered the actual EventSequencer at original SceneObj0x1A, and added eight original player/HUD utility entry points in OriginalPlayerEventUtil.cpp. The common scene-object transaction already runs construction/init in the real scene Game arena, adopts the NameObj and retires it before that arena. Event sequences and their raw callback/frame arrays have no separate external resource handles; their storage belongs to that arena. No secondary event state machine or success flag was added.

The only reference portability change uses HashSortTable::Value for pointer payloads. Registration stores the entire pointer; lookup reads an actual Value local and casts it afterward rather than aliasing EventSequence** as an integer output pointer. Native and reference PlayerEvent files are identical. Fresh Wii compilation of all7 production TUs passes;57 compared method/vtable records include45 exact, minimum95.2381%. The minimum is startEvent: the correctly typed local removes a redundant stack reload before clearFlag, with the same branch/call/store behavior. Other records are>=99.28%. The callbacks were already present in reference; no missing sequence callback bodies were discovered.

All7 native TUs, factory and added provider compile. A standalone native regression links complete production PlayerEvent.cpp and HashSortTableCompat.cpp with ordinary dead-section elimination and runs successfully. It executes actual member-function callbacks and checks inclusive time intervals, status checks, phase transition timing, registration order, stopping, reset, u16 counter/argument wrapping, and complete event pointer retention through real hash-table sorting/search. It does not invoke the death callbacks, scene factory, wipe renderer, save transitions or scene retirement. Those outcomes are not claimed. Xmake target `smg-pc-original-event-sequence-tests` is appended; the direct equivalent compile/link/run is the current proof while parent coordinates root builds.

## Remaining real owners

After the exact player/HUD forwards, seven unresolved providers remain relative to the current built archives (integrated-native-unresolved.json):

- MR::decPlayerLeft calls actual GameDataFunction::addPlayerLeft(-1), addMissPoint(1), incPlayerMissNum. The latter two need the real save-data owner path.
- MR::startDownWipe and startGameOverWipe require actual SceneWipeHolder, selecting authored クッパ and ゲームオーバー wipe layouts.
- MR::setSoundVolumeSetting calls actual AudSystem::setSeVolumeSet; SE permission state alone is not this volume-ramp owner.
- MR::requestStartGameOverDemo, requestEndGameOverDemo, requestEndMissDemo require the real GameScene instance and its NerveExecutor. No throw-only linker provider has been introduced.

The parent must add EventSequencer creation to the active scene setup, following GameScene::init. Factory activation by itself does not mean the scene has requested it.

## GameScene dependency probe

Current host StageHostScene/GatewayDemoScene instances derive from Scene, not the original GameScene, and the original GameSystem singleton/scene-controller route is not active. Casting a host scene to GameScene would be invalid.

Original requestStartGameOverDemo enters GameSceneGameOver unless already there. Its exeGameOver calls the original SceneFunction movementStopSceneController and executeMovementList. RequestEndGameOverDemo enters GameSceneSaveAfterGameOver, whose first frame increments game-over count, starts the real save sequence, and starts pointer pause mode; after the save handle becomes inactive it ends pointer mode and requests scene change. RequestEndMissDemo enters CometRetryAfterMiss only when that actual scene object exists, otherwise requests the actual stage retry. These are real lifecycle operations, not telemetry events.

The complete original GameScene TU freshly compiles for Wii and also compiles natively in a notes-only overlay restoring its original SceneFunction, GameSequenceFunction and DrawUtil declarations plus an explicit SceneObjHolder include. No GameScene production edit was made. GameScene.native-unresolved.json records97 dependencies against current built archives, spanning list execution/stop controller, four sequence owners, resource/placement phases, save/retry flow, pointer/camera, movies, and drawing. This is a full-object dependency inventory; reachability and source-ready changes may reduce the actual final link set. A concrete GameScene/scene-controller lifetime integration is required before its original public request methods are exposed.

Parent owns staging/commits and subsequent full builds. No root/decomp commit was performed by this lane.
