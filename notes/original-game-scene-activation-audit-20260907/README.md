# Original GameScene activation audit — 2026-09-07

This is a read-only production audit. It creates notes and isolated compiler artifacts only; it does not activate sources, change the root build, or claim native gameplay. The useful next boundary is an actual `GameScene` owned by the native scene session, with the existing stage resources and registered-object lifetimes extracted into services underneath the original scene initialization and execution calls.

## Current ownership

`src/scene/SceneLifecycleService` owns `unique_ptr<StageHostScene>`. `active_scene()` therefore returns a `Scene*` whose dynamic type is `StageHostScene`. `StageHostScene::start()` is empty, `update()` executes the native aggregate movement service, and `calcAnim()` executes the native aggregate animation/view service. Native draw entry points likewise call the host directly. No native `GameScene` is created, and casting the returned pointer to it would be invalid.

`GameSystemSceneControllerService` manages native queued/destroy/init/ready/normal transitions. These are host scene-transition phases, not the original thirteen GameScene nerves. `StageSessionState` is marked Gameplay directly after host initialization. Neither mechanism can substitute for the original scene's current nerve and step.

The original GameScene source is already complete in `decomp/src/Game/Scene/GameScene.cpp`: constructor, initialization, start, update, animation, drawing, all requests and all thirteen nerve executors. `requestStartGameOverDemo()` selects its own original GameOver nerve; `requestEndGameOverDemo()` selects SaveAfterGameOver. Action, ScenarioStarter and GameOver each execute the original movement list. There is no reason to duplicate a GameOver-only state machine in the host.

## A reusable host boundary

The recommended integration has three separately reviewable pieces:

1. A scene session owns a real `GameScene` and the native services it borrows. The existing lifecycle service calls that object's original `init/start/update/calcAnim/draw` virtuals. A native typed active-GameScene binding lets compatibility `GameSceneFunction` entry points call the actual instance until the full original GameSystem/controller owner exists. It must not reinterpret an unrelated Scene or manufacture a GameSystem layout. The original `GameSceneFunction.cpp` can be imported whole later when its genuine singleton/controller dependency exists.
2. Extract the current StageHost resources and placement operations into a stage initialization service. Supply the native `SceneFunction` resource-loading boundary from that service, while retaining the original GameScene call order. A whole `StageHostScene::init()` call hidden behind one hook would repeat the original setup and is unsuitable.
3. Let complete original `SceneExecutor.cpp` own category order. Expose individual category dispatch and connection-queue operations from the native scheduler, keeping its actual object-registration identity, selected JKR domain, callback guards, clipping/suspension checks and lifetime retention. Do not call the existing aggregate movement or animation loop from every category hook.

The service extraction can preserve these existing operations without importing another copy of authored data:

| Original call boundary | Existing native implementation to retain |
| --- | --- |
| Start/wait stage files | `StageAuthoredData::resolve`, stage metadata, ObjectNameTable, PlanetMapCatalog, real DVD/archive services |
| Initialize selected scenario | `StageResourceBinding`, `StageZoneMatrixBinding`, `StageLightSceneBinding`, actual stage session metadata |
| Preload common/scenario actor files | `AuthoredPlacementInstantiator` preflight and resource preload; split completion according to the original requests |
| Start actor placement | Exact StartInfo Mario creation in PlacementPlayer phase, then authored high-priority/ordinary placement passes |
| Finish registered post-placement methods | One ordered registered NameObj pass, real collision publication, actual Sleep synchronization, camera parameter completion and initialization-phase completion |
| Allocate draw buffers | Existing `SceneDrawBufferService` original holder and retained model owners, with actor-list allocation after scene init |

Stage resources and the demo registration owner must exist before actor construction and camera-code registration. Actual CameraDirector publication currently happens in the explicit `SceneObjHolderBinding::initialize_camera_system()` call, not merely its factory allocation. Move that publication to the corresponding original create/init boundary. Similarly, replace the host's fixed `initialize_effect_system(3072, 256)` timing with the parameters selected by the original `GameScene::initEffect()` and its `SceneFunction::initEffectSystem` call. Preserve original owner creation semantics; do not silently turn these calls into inert factory entries.

The reference alternative is also available: `SceneDataInitializer` already has all eight methods, and `StageFileLoader` has its original five methods. They lead into the real `StageDataHolder`, GalaxyStatusAccessor, nested archive iteration and heap/file services. A complete owner activation could use them instead of the extracted host service, but it is a larger dependency graph. Current host StageResourceBinding is not a StageDataHolder and must not be cast to one.

## Original order that affects jumping and camera

The exact original `SceneExecutor::executeMovementList()` checks StopSceneController, processes scene movement requirements, runs Camera and MirrorCamera, updates the pointer camera and loads the view matrix, then runs ClippingDirector. SensorHitChecker follows ScreenEffect. The four moving-collision categories then run their movement **and calcAnim** before CollisionDirector and the ordinary environment/map/NPC/player/enemy categories.

The current aggregate scheduler computes clipping before camera movement and places all animation in a later aggregate pass. Although its movement category order is deliberately original, this is not the complete original movement/animation interleave. Using category-level dispatch under the original SceneExecutor closes this shared timing discrepancy for collision matrices and camera-relative movement. It must also remove the aggregate scheduler's injected sensor pass when that same work is dispatched at the original SensorHitChecker category, so contacts are not computed twice.

`SceneDrawBufferService` already exposes `entry(camera_type)`, `draw_opaque(category)` and `draw_translucent(category)` using the actual original DrawBufferHolder. These are suitable for the CategoryList draw-buffer boundary. Its private NameObjListExecutor currently owns only the draw list and holder, while Scene's native `mListExecutor` stays null. Share or deliberately transfer that actual owner; never install a second unregistered executor and claim it is executing the scene.

`src/Game/Scene/Scene.cpp` currently suppresses `initNameObjListExecutor()` and omits original list-executor deletion. That compatibility belongs outside Game if retained during the transition. The reference Scene initializes a genuine SceneNameObjListExecutor and owns its deletion. Either close that original ownership graph or use an explicit host execution binding; do not populate fields with unrelated native objects.

Original GameScene drawing also orders mirror, volume/silhouette/alpha shadow, screen capture, image effect, layout and draw-sync work beyond the current aggregate normal scene draw. Activating original draw requires the corresponding category backend, rather than calling both full draw paths. The newly restored original image-effect owners make this a useful shared next step.

## Initialization and destruction invariants

The original controller creates/selects Game heap, constructs the scene, initializes its list executor and SceneObjHolder, calls scene init, allocates draw-buffer actor lists, then starts the scene. GameScene initialization starts stage/player archive loading, initializes its nerve and NameObj/LiveActor systems, creates the effect/general objects, waits files, initializes the selected scenario, creates NPC/message/camera/layout/wipe owners, creates its sequences, places actors, completes camera/pointer/event setup and invokes registered `initAfterPlacement` methods. Preserve that order and exactly-once postpasses.

`GameScene::initSequences()` always constructs GameStageClearSequence, GamePauseSequence, GameSceneScenarioOpeningCameraState and GameScenePauseControl. The opening-camera state owns ScenarioTitle; the pause control owns PauseButtonCheckerInGame; the pause sequence clones the callback to the control and can create PauseMenu. The native session needs explicit lifetime coverage for raw non-NameObj children as well as registered LayoutActor descendants. Leaving mPauseCtrl null cannot support original `update()`.

The original GameScene destructor calls `destroySceneMessage`, `NPCFunction::deleteNPCData` and `onStarPointerSceneOut`. These services and the typed active scene binding must still be usable during that destructor. Only afterward should registered actors/layouts and raw scene children be retired, draw registrations/resources drained, SceneObj/list ownership released and finally Game heap retired. The SceneObjHolder itself remains owned exactly once by Scene. Apply the existing registered-identity rollback discipline if any initializer throws; verify a second scene can initialize after teardown.

## Concrete dependency frontier

`sceneobj-inventory.json` records every literal SceneObj ID created by the original GameScene/SceneFunction and the native factory snapshot. Missing direct cases include SceneDataInitializer, NameObjExecuteHolder, StopSceneController, SceneNameObjMovementController, CinemaFrame, EventDirector, StarPieceDirector, NamePosHolder and the live-actor/demo/sensor group owners. Some already have an independent native system serving their semantics; that requires ownership integration, not placeholder NameObjs. SceneWipeHolder work is concurrent in another agent, so refresh that one factory result before activation. Audio owners may follow the project's explicit audio scope, but that policy must not erase scene/nerve ownership.

`cohort-unresolved-before-platform.json` is an object-symbol inventory against the current game archive, not a link failure report. It subtracts definitions from the three candidate objects and `libsmg-pc-game.a`; 92 symbols remain, including Aurora SDK and C++ platform symbols. Seventy-five start with MR, Scene or Game names. Main groups are:

| Group | Examples / coherent owner |
| --- | --- |
| Scene children | GamePauseSequence, GameScenePauseControl, GameSceneScenarioOpeningCameraState, GameStageClearSequence |
| File/scenario service | Seven SceneDataInitializer calls; player archive request/wait; scenario decided; registered postpass and initialization completion |
| Execution owner | Actual controller list-executor access, connection/disconnection requirement queues, StopSceneController and SceneNameObjMovementController |
| Scene metadata/sequence | Start wipe policy, retry/welcome, timer, miss/GameOver transitions, movie/story policy |
| Scene objects | NPC data, scene messages, StarPiece creation, CinemaFrame, camera completion, pointer scene-out |
| Drawing | ODH capture, clearZBuffer, mirror queries, normal bloom/path/thread decisions and Talk draw-sync token |
| Audio | AudSceneMgr start, scenario wave requests/completion, stage BGM selection |

This inventory does not recursively instantiate every new child's dependency graph and is not an assertion that all symbols already in the archive have complete semantic ownership. In particular, compiling GameScene's vtable/nerve graph reaches all original executors, not only its Action/GameOver routines.

## Proof and next bounded implementation

The initial twelve-TU isolated probe is in `isolated-source-probe.json`. SceneDataInitializer, StopSceneController, SceneNameObjMovementController, GameStageClearSequence, ScenarioTitle and PauseButtonCheckerInGame passed native syntax. Pause-control/opening-camera/pause-sequence probes reached the existing missing `JSystem/JAudio2/JAIAudible.hpp` include through the fallback AudSystem graph; this is a header/provider boundary, not proof of a bad original method.

The complete unchanged original `GameScene.cpp`, `SceneFunction.cpp` and `SceneExecutor.cpp` each compiled to a native object with exit 0. The audit used byte-identical reference Scene.hpp and SceneFunction.hpp overlays plus exactly two declarations already present in reference headers: `GameSequenceFunction::isNeedMoviePlayerForStorySequenceEvent()` and `MR::clearZBuffer()`. Commands and diagnostics are in `original-object-probe.json`; source hashes and call spellings are in `source-call-inventory.json`. Native headers otherwise take precedence, with decomp include fallback for unimported declarations. This is compile evidence only, with no full link, new Wii matching proof or gameplay run. `.o` files are local scratch artifacts and excluded from the notes checkpoint manifest.

Suggested implementation order:

1. Expose scheduler movement/animation category and requirement-queue entry points; prove exact original order, collision animation interleave, camera-before-clipping, one sensor pass and correct callback allocation domain.
2. Extract stage initialization resources from StageHost without changing placement behavior; prove real-disc metadata/StartInfo/rail/camera resources and failed-init/retry ownership.
3. Activate actual GameScene plus its exact mandatory sequence/control children, typed binding and initialization hooks. Route GameSceneFunction requests to this instance and retire overlapping native request providers.
4. Activate the original SceneExecutor and call the original scene virtuals once per host frame. Close real factory/provider dependencies as found; keep missing features explicit rather than throw-only linker implementations.
5. Validate actual original start-to-Action transition, camera and jumping, pause/stop gates, GameOver-to-save and miss/restart requests on the same original scene instance, then two complete create/destroy cycles with heap overwrite. Record original nerve identities/steps and real actor/camera state; compilation and synthetic category ordering alone are not gameplay proof.
