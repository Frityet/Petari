# Actual GameScene ownership frontier and prerequisite owners

## Implemented original source cohort

Imported complete original MovementOnOffGroupHolder, NPCDirector and NPCParameter CPP/header pairs, byte-identical to current decomp. No GameScene/NPC gameplay algorithm or reference body was changed. The existing SceneObj factory now constructs actual MovementOnOffGroupHolder and NPCDirector. The movement-group owner uses the exact original label, explicitly retained as a static CP932 native string. NPCDirector constructs its actual caps/item parameter readers and acquires NPCData.arc through the existing ResourceHolder owner.

SceneInitializationCompat now provides the complete original sixteen-call SceneFunction::initForLiveActor body. This adds the missing original setup routine without shortening its scene-object list or synthesizing absent members. Its creation calls do not cast the TalkDirector slot. That slot currently contains the native TalkRuntime; this checkpoint does not claim it is the original TalkDirector or make typed original TalkDirector access safe.

NPCParameter's entire native TU compiled successfully with the current LLVM23 compilation flags, proving its actual scalar/range/template/JMap dependency closure. No compile-only source delta was needed. Source equality and hashes are in source-manifest.json; native-probe.json contains the one command. Parent owns the combined build; no Xmake or repeated test sweep was run here.

The original parameter helpers contain pointers/scalars/fixed vectors; their scene allocations have no native C++ caches requiring a new ownership service. Actual ResourceHolder native backing already has its separate owner. The movement group's child NameObjGroups use existing registration capture and NameObjGroup member-array destruction. This does not introduce a fake owner for GameScene.

## Existing coherent actual GameScene route

SceneLifecycleService::create_stage_scene already constructs the complete original GameScene in an actual JKR scene domain. It constructs StageInitializationService and GameSceneBinding, publishes session and SceneObj/execution bindings, calls original GameScene::init, completes list allocation/initialization, then calls original start. Update/calcAnim/draw dispatch to that same actual scene. Retirement snapshots original scene children, runs the original derived destructor, then retires bindings/children and releases the scene domain. GameSceneBinding only binds an actual GameScene and queries its actual nerves.

GatewayDemoScene currently constructs a bounded Scene and uses selected original actors plus native loading/execution ownership. Adding a GameSceneBinding to that unrelated Scene, constructing a second GameScene only to answer queries, or setting a scene nerve manually would not establish the complete original init contract. None was done. RunawayTico::exeGuide0 can request its original timekeep demo immediately, so its actual canStartDemo GameScene query remains an explicit boundary until full scene startup runs.

## Exact remaining source/owner dependencies

| Boundary | Actual required owner/source |
| --- | --- |
| MR::requestChangeArchivePlayer / waitEndChangeArchivePlayer | Original SystemUtil forwards to GameSystemFunction and GameSystem::mStationedArchiveLoader. Whole GameSystemStationedArchiveLoader includes PlayerHeapHolder, stationed resource conditions, original nerve progression, async execution, HeapMemoryWatcher and resource-heap disposal. Neither a null loader nor an unconditional done result provides that contract. |
| GameSequenceFunction::isNeedMoviePlayerForStorySequenceEvent | Original GameSystem→GameSequenceDirector→GameSequenceProgress→StorySequenceExecutor. The native transition service's separate StorySequenceExecutor is not that original owner graph. |
| MR::hasRetryGalaxySequence | Original SequenceUtil→GameSequenceFunction→Director's process-lived GameDataTemporaryInGalaxy::_4. Current StageSession owns another temporary data object per scene, which cannot be treated as the original persistent retry owner. |
| SceneFunction::initAfterScenarioSelected | Defined only by excluded original SceneFunction.cpp. The existing StageInitializationService::initialize_scenario_resources is a concrete native resource boundary, but the original wrapper/loading owner route still needs to be connected in the full startup cohort. |
| MR::suspendAsyncExecuteThread | Defined only by excluded original SystemUtil.cpp; requires actual async task suspension semantics. |
| Unconditional GameScene SceneObj requests | Actual AudCameraWatcher, AudEffectDirector, ResourceShare, PlanetMapCreator, EventDirector and StarPieceDirector factory owners are still missing. Conditional movie/comet/staff owners remain additional branches. |
| Typed TalkDirector consumers | Current native TalkRuntime slot remains an explicit frontier. Complete original DemoUtil methods casting that slot cannot be activated over it. |
| Final startup sequencing | Original Progress::startScene must execute before original scene start. Current SceneLifecycleService immediately starts its scene, so complete process sequence activation must also consolidate duplicated native startup scheduling, not just fill link symbols. |

Four of the previously recorded five direct GameScene symbol gaps remain after this checkpoint; initForLiveActor is now supplied. Source inspection also identified the two excluded-only utility definitions listed above and the missing typed scene owners. This is a source/ownership audit, not a newly linked GameScene executable claim.

The next coherent work is the actual stationed archive/sequence owner graph and its native platform/task/resource boundaries, followed by full original GameScene init/start. Current scope deliberately keeps RunawayRabbitCollect unavailable when those requirements cannot be satisfied, as directed by parent; it does not fabricate scene-query answers.
