# Ordinary Gateway scenario route audit — 2026-09-10

Read-only source/archive audit. No root build, stage launch, or production edits were made. Current Gateway placement counts have **not** been measured; older placement reports must not be presented as current.

## Exact original route

Gateway is the internal stage `HeavensDoorGalaxy`, scenario1. `decomp/src/Game/System/StorySequenceExecutor.cpp:994` (`overwriteGalaxyNameAfterLoading`) chooses it when the selected actual file has passed `ピーチ城浮上後` or is Luigi, unless GrandStar1 already redirects to the observatory. An untouched Mario file instead starts the prologue/PeachCastleGarden route. The current showcase explicitly establishes that saved-story checkpoint; it does not prove original file-select/bootstrap execution.

Original `SequenceUtil.cpp` requests after-loading move type6. `GameSequenceProgress::requestGalaxyMove` executes story selection, saved/temporary state and scheduler changes, then publishes actual next `SceneControlInfo` (scene, stage, scenario, selected scenario and StartInfo) to actual GameSystemSceneController. The controller's async initialize state creates the original Scene, its NameObj list and SceneObjHolder, calls GameScene::init, allocates draw lists, then waits at ReadyToStartScene. `GameSequenceProgress::exeNormal` first calls its own `startScene`, then `GameSequenceFunction::startScene` starts the controller. Progress startup owns the story-start callback, screen/pointer policy, timers, wipes and spin permission (`!isPassedStoryEvent("スピン権利")`). The controller starts the real Scene and resets its leave watcher. Current native host services must not pretend this original Progress work occurred.

## What ordinary native StageHost currently does

StageHostService -> native GameSystemSceneControllerService -> SceneLifecycleService::create_stage_scene constructs an actual GameScene, attaches StageInitializationService and the actual scene execution/lifetime bindings, calls pre_scene_init + GameScene::init, completes placement/list allocation and immediately starts the scene. SequenceBootService and SceneTransitionRequestService independently own host transition metadata and one StorySequenceExecutor. This is still parallel native orchestration rather than an active original GameSystem/GameSequenceDirector/GameSequenceProgress graph.

The ordinary initializer uses Strict authored placement (`StageInitializationService.cpp:623`). `prepare_actor_plan` performs full authored preflight first and StartInfo factory preflight second. Its StartInfo constructor uses the authored object name and retail Mario actor label under PlacementPlayer phase. It has no Gateway-specific fallback. A supplied explicit object does not exempt the remaining stage rows from strict preflight.

## First verified boundaries

The current Game archive contains GameScene::init but only undefined references for all five of: MR::requestChangeArchivePlayer, MR::waitEndChangeArchivePlayer, SceneFunction::initForLiveActor, GameSequenceFunction::isNeedMoviePlayerForStorySequenceEvent and MR::hasRetryGalaxySequence. None is defined in the other three current native archives. See current-game-archive-symbols.json; this is a measured link frontier, not an observed runtime throw or a complete link-error inventory.

Beyond those providers, original initForLiveActor requires actual scene-slot ownership for DemoDirector, NPCDirector, SensorHitChecker and the actor groups. Current GatewayDemoScene manually establishes its retained DemoSceneRuntime and selected actual scene slots; SceneObjHolderCompat lacks several original slots. GameScene also requests audio watchers, ResourceShare, PlanetMapCreator and StarPieceDirector not currently mapped. Missing slot cases return null; a missing case alone does not prove the exact first runtime crash.

The separate StartInfo gate is certain from source: Showcase explicitly rejects production factory creators named Mario/MarioActor, then creates GatewayMarioOwner directly. Once strict placement preflight succeeds, ordinary StartInfo cannot construct Mario until his actual factory/lifetime/resource closure is published. This should be a general exact creator/owner integration, not a Gateway manager.

Original progress/controller dependencies remain broader than this stage route: GameSystemSceneController constructs IntermissionScene, PlayTimerScene and ScenarioSelectScene; GameSequenceDirector constructs SaveDataHandleSequence, Progress, process-lived temporary galaxy data and NWC24. Actual six-chunk GameDataHolder/UserFile storage is now active (notes/original-save-owner-20260910/README.md); earlier notes saying it is absent are stale. Native GameSequenceFunction remains a save-only facade, and SaveDataHandleSequence remains a compatibility owner. Activate coherent original prerequisites and replace overlapping host ownership, rather than copying just startScene's spin assignment.

## Existing executable and proof boundaries

`src/debug/StageConstructionProbe.cpp` / target `smg-pc-stage-construction-probe` accepts:

```sh
SMGPC_STAGE_PLACEMENT_REPORT_PATH=notes/original-gateway-sequence-20260910/stage-route/strict-placement.md \
  build/macosx/arm64/debug/smg-pc-stage-construction-probe \
  --disc "$SMGPC_REAL_DISC" --stage HeavensDoorGalaxy --scene Game \
  --scenario 1 --start-id 0 --start-zone-id 0
```

No current debug executable exists. It currently omits RuntimeContext::initialize_scenario_catalog and initialize_particle_resources before scene creation; Showcase does both. Full linking/startup must be fixed before this command can be called runtime evidence. The report is debug-only and is written when authored preflight is reached, even when unsupported rows then throw.

`tests/NameObjFactoryPlacementTests.cpp:427` already resolves all Gateway scenario1 ordinary placements with the actual factory, but does not report/count Gateway blockers; its exact two-blocker report covers FileSelect only. `tests/StageInitializationResourceTests.cpp` proves retained real Gateway archives, authored StartInfo/camera data, zones and retirement across3 scene domains; it deliberately never creates Mario or places actors. `AuthoredPlacementInstantiatorTests` proves strict support-only/nonmutating gates and typed selected actor lifecycles, not a complete Gateway route.

A useful next diagnostic is a generic metadata-only mode in the existing StageConstructionProbe: initialize actual scenario catalog, resolve_stage_placement_objects(dvd, stage, scenario), invoke preflight_stage_placements_or_throw. It can report current total/complete/ignored/blocked rows without constructing a Scene or bypassing readiness. Keep its result explicitly separate from full StageHost startup. No such mode was implemented by this audit.
