# Original camera selection and director closure

Read-only audit on 2026-09-03, after pushed checkpoint `2e6a86270`.
Only this note and its catalog inventory were written. No source was changed,
compiled, or run for this audit. Source presence is not a claim of complete
binary correspondence or a successful native link.

The most consequential next camera task is to activate the complete original
`CameraDirector` ownership and `CameraManGame`/`CameraManEvent` lifecycle,
including the original catalog, registered chunks, and resource readers.
Adding another selected camera class to the current retained-start owner
would leave the principal behavior gap in place. Do not construct a partial
`CameraHolder` or `CameraDirector`, or reproduce their selection in native code.

## Current executable boundary

- `pc-port/src/showcase/Showcase.cpp:700` resolves the authored start camera;
  lines 715–737 retain one controller and attach the actual Mario target.
  It does not execute original normal camera selection as Mario moves.
- `pc-port/src/camera/OriginalGameCamera.cpp:28` constructs actual
  `CameraParallel` or `CameraFixedPoint`. `OriginalAnimationCamera.cpp`
  separately runs actual `CameraAnim`. The managers are actual base
  `CameraMan` objects, not `CameraManGame` or `CameraManEvent`.
- `pc-port/src/runtime/RuntimeServices.cpp:2725` owns target movement,
  controller calculation, native event state, and view publication.
  `update_authored_game_camera` keeps the retained camera/chunk. The original
  `OnlyCamera` and `CameraViewInterpolator` already execute through
  `camera/OriginalCameraView.cpp`; they should become the actual director's
  instances when ownership is transferred, not run a second time.
- `pc-port/src/scene/StageEventCameraBinding.cpp` attaches the native event
  catalog. `camera/EventCamera.cpp` has one pending and one active event.
  `camera/CameraParam.cpp:178` reads `evpriority`, but no native runtime
  consumer reads `event_priority`. Root `CameraManEvent` has two priority
  slots, each with a current and pending item.
- `pc-port/src/compat/CameraUtilCompat.cpp:437` onward routes requests into
  the native service. Landing-delayed event release at line 546 and
  start-animation completion explicitly report that the original owner is
  absent. Importing controllers alone does not resolve those callers.

The earlier `notes/next-camera-selection-20260903/README.md` is stale in several
material respects. Actual `CameraTargetPlayer` movement and virtual queries
are now present in `compat/OriginalCameraTargetPlayer.cpp`. CubeCamera lookup,
ground-triangle choice, bound/Bee basis, and movement-timer behavior already
come from the original target/accessor bodies. `compat/HitInfoCompat.cpp:72`
now supplies real host actor name and retained placement zone. Root
`KCollisionServer::calcFarthestVertexDistance` and four start/stage scene
helpers have since been recovered. They must not be scheduled as missing
decompilations again.

## Original lifecycle to activate intact

`src/Game/Camera/CameraDirector.cpp:65` constructs the stack, OnlyCamera,
poses, full CameraHolder, chunk holder, creator, rail/register/target holders,
shaker, view interpolator, cover, rotation checker, four managers, and dummy
matrix target. It registers camera movement, owns/activates the game manager,
and creates center-screen blur. Its constructor graph is the integration
contract.

`CameraDirector::movement` at line 116 orders: previous view state; selected
target movement; manager movement; OnlyCamera pose processing; view
interpolation/publication; manager matrix and pose snapshot; subjective
processing; shaker; start-condition and landing-event checks; rotation
checker; reset-request retirement. Keep this in the existing camera movement
phase, before Player movement. It consumes the previous completed player
movement plus current input. Do not advance the target from getters or add
an extra post-player camera tick.

`CameraManGame::calc` (`src/Game/Camera/CameraManGame.cpp:54`) selects a chunk,
checks reset, calculates the camera, records its returned target, copies a
safe pose, then runs Karikari and Heli effectors. The original selection is:

1. Start-position camera, then zoom (`selectCameraChunk`, line 177).
2. Determine Normal/Swim/WaterSurface/GCapture/Foo state.
3. In Normal, the target's cached CubeCamera area selects its zone plus
   `c:%04x` Arg0 ID (`setCubeChunk`, line 627).
4. Otherwise, a valid ground triangle with camera ID below 255 selects
   `g:<host actor name>:<camera ID>:0` in the host's placement zone
   (`updateNormal`, line 560).
5. Without either, reapply the previous chunk or use the original null/default
   camera. Preserve original through, reset, interpolation, and zone handling.

`CameraDirector::checkStartCondition` (line 319) ends start selection only
when the game manager is current, the original post-incremented start timer
passes 30, and `CameraTargetHolder::isMoving` reports lastMove length above 1.
The native retained-start owner has no corresponding automatic selector
transition. This is an immediate observable gain from activating the director.

`CameraManEvent::start/updateChunkFIFO/applyChunk/checkReset` preserves the
two authored priorities and target arguments. `CameraDirector::push/pop` and
`startEvent/endEvent` handle actual activation/reset, initial pose/view
seeding, interpolation, and return to game selection. `checkEndOfEventCamera`
uses the target holder's actual grounded/water predicates for landing release.
These complete methods already exist in root; no replacement queue is needed.

## Native import and ownership inventory

| Group | Root files under `src/Game/Camera/` | Native status |
| --- | --- | --- |
| Ownership/selection | `CameraDirector.cpp`, `CameraManGame.cpp`, `CameraManEvent.cpp`, `CameraManPause.cpp`, `CameraManSubjective.cpp` | Implementations absent; Director header only |
| Catalog | `CameraHolder.cpp` and all catalog camera/translator files | 45 registered types; only Parallel, FixedPoint, Anim imported |
| Parameters | `CameraParamChunk.cpp`, `CameraParamChunkHolder.cpp`, `CameraParamChunkID.cpp`, `CameraParamString.cpp`, `DotCamParams.cpp` | Implementations absent; some declaration headers present |
| Registration | `GameCameraCreator.cpp`, `CameraRegisterHolder.cpp`, `CameraRailHolder.cpp` | Absent |
| Target ownership | `CameraTargetHolder.cpp`, `CameraTargetArg.cpp` | Holder absent; native `CameraTargetArg::setTarget` is empty |
| Manager effects | `CamKarikariEffector.cpp`, `CamHeliEffector.cpp` | Absent |
| Director effects | `CameraShaker.cpp`, `CameraShakePattern.cpp`, `CameraShakePatternImpl.cpp`, `CameraShakeTask.cpp`, `CameraCover.cpp`, `CameraRotChecker.cpp` | Absent |

See `catalog.tsv` for every original type, exact class/translator source, and
current PC source presence. All 45 class source files exist in root. That
inventory does not prove that every inherited dependency can already link.
Import corresponding headers and the actual helper classes reached by each
constructor/vtable, not dummy translators or default-camera substitutions.

The full catalog reaches rail riders, register lookups, collision/gravity,
player camera actions, and specialized target methods. `RailRider.cpp` is
already imported and its camera constructor uses `MR::getCameraRailInfo`.
`CameraCover` reaches actual CaptureScreenActor, category execution,
JUTTexture, and image-effect drawing; native CaptureScreen and captured-frame
blur facilities now exist, but this audit did not build that combined graph.
The director's `MR::createCenterScreenBlur` provider already exists in
`compat/CapturedFrameBlurService.cpp:244`.

## Exact remaining root recovery frontier

These declarations/comments lack bodies in the current root sources. Addresses
and sizes are from `config/RMGK01/symbols.txt`; recover and verify them with the
original compiler against the current DOL before mirroring their semantics.

| Root file/function | RMGK01 address / size | Consumer |
| --- | --- | --- |
| `Util/SceneUtil.cpp`: `MR::getStartPosNum` | `0x803F757C / 0x24` | `GameCameraCreator::scanStartPos` |
| `Scene/StageDataHolder.cpp`: `getCommonPathInfoElementNum` | `0x803474C8 / 0x44` | Original rail-count forwarding closure |
| `Util/SceneUtil.cpp`: `MR::getPlacedRailNum` | `0x803F7AF0 / 0x54` | `CameraRailHolder` constructor |
| `Util/SceneUtil.cpp`: `MR::getCameraRailInfo` | `0x803F7B44 / 0x5C` | `RailRider(s32,s32)` |
| `Util/SceneUtil.cpp`: `MR::getCameraRailInfoFromRailDataIndex` | `0x803F7BA0 / 0x74` | `CameraRailHolder` constructor |
| `Util/SceneUtil.cpp`: `MR::getCurrentScenarioStartAnimCameraData` | `0x803F7CA0 / 0x98` | `CameraDirector::createStartAnimCamera` |

Root `MR::getCurrentStartZoneId`, `getCurrentStartCameraId`,
`getStartCameraIdInfoFromStartDataIndex`, and `getStageCameraData` are present
at `src/Game/Util/SceneUtil.cpp:153,188,192,199`. They are missing from the
native providers, and native `Util/SceneUtil.cpp` is excluded by Game/xmake.lua.
`MR::getZoneNum` also needs the real active stage's zone catalog provider.
Root `StageDataHolder` already supplies recursive start count, start ID lookup,
and common-path table lookup. The native `scene/StagePlacementResolver.hpp`
retains holder occurrences, ancestry, load batches, JMap tables, and zone
transforms; use those actual resources at the platform boundary, retaining
the original query/absence rules.

The root comment `// getAnimCameraCurrentFrame` has no corresponding current
RMGK01 function symbol or declaration found in this audit. Do not invent a
new required recovery from that comment alone.

## Concrete compatibility work before activation

- **BCAM attachment and lifetime:** `JMapInfo::attach` currently returns false
  (`pc-port/src/Game/Util/JMapInfo.cpp:86`). Original DotCam invokes it, then
  reads version and rows. Supply bounded, retained resource attachment via
  the general archive/JMap boundary. `DotCamReaderInBin::hasMoreChunk` also
  spells `JMapData*`, whereas native mData is shared `DataCompat`; adapt only
  that representation/access. Preserve iterator/data identity and strings
  across chunk loading. A header cast is insufficient.
- **Original registered-chunk order:** `CameraParamChunkHolder::loadFile`
  fills previously created chunks and applies version arrangements; it does
  not instantiate every BCAM row. Execute area/start scans, default creation,
  event declarations, then load/sort, respecting protected `_64` chunks.
  Shared collision registration must call the actual creator. Root
  `CollisionParts::init` brackets `KCollisionServer::calcFarthestVertexDistance`
  camera-code visits with owner name and zone. Native collision providers
  currently have no `initCameraCodeCollection/registerCameraCode/term...`
  calls. Use the recovered prism/attribute visitation contract, not a guessed
  set of all PA rows.
- **Rails:** `compat/SceneSystemUtilCompat.cpp:118` currently throws for camera
  rail lookup. Back all recovered rail helpers with retained zone CommonPath
  tables and actual path/point indices. Preserve original arg0 filtering,
  rail-ID sort, and `RailRider` construction in `CameraRailHolder`.
- **Actual player animation owner:** after start/zoom selection yields,
  `CameraManGame::checkStateShift` calls `isWaterMode` for a normal uncaptured
  player. `CameraTargetPlayer` -> `MarioAccess::isInWaterMode` ->
  `MarioActor::isAnimationRun` -> `MarioModule::isAnimationRun` ->
  `XanimePlayer::isRun` is the existing original chain. Native
  `MarioAnimator::init` still sets mResourceTable/mXanimePlayer/upper to null
  (`pc-port/src/Game/Player/MarioAnimator.cpp:37`). The newly active renderer
  XanimeCore is not this owner. Activating normal selection now would reach
  an invalid null player; complete the original MarioAnimator/XanimePlayer
  resource/state ownership first. Do not make the water predicate return
  false merely to enter Normal.
- **Scene predicates:** copy original `MR::isPlayerGCaptured` from
  `src/Game/MapObj/GCapture.cpp:816` and Karikari getters from
  `src/Game/Enemy/KarikariDirector.cpp:158` as needed. They already express
  real absent-scene-object behavior; fixed false/zero replacements are not
  required. Keep actual object behavior when the scene object exists.
- **Director access:** replace scoped controller-only target lookup in
  `compat/CameraLocalUtilRuntime.cpp` with the original director path for
  actual owned game cameras. Restore `CameraTargetArg::setTarget` exactly;
  it selects object/matrix/actor/player in order and invalidates matrix
  last movement. Route Game CameraUtil requests to the real scene Director.
  Retire duplicate service target ticks, event FIFO, shake calculation, and
  OnlyCamera/view processing when the original director owns them.

## Architecture and source-verification gates

- `CamTranslatorAnim` currently casts the 32-bit general mNum1 payload to an
  animation pointer. Current typed native animation ownership avoids invoking
  that conversion. Full original chunk/translator activation requires a
  general retained native resource-pointer binding; never truncate pointers
  or fabricate Game chunks/holders.
- `CamTranslatorSpiral.cpp:7` and `CameraGeneralParam::getNum1Low/High` read
  signed 16-bit halves through native memory. Preserve the Wii big-endian
  half ordering explicitly on little-endian hosts. This is an architecture
  fix, not a change to authored values.
- Do not treat every suspicious root loop as a decompilation error. Retail
  `CameraManEvent::isEventActive` really starts at slot 1 and increments
  (`0x800A4034`, `0x800A407C`), matching the root source's commented quirk.
  Retail `CameraShaker::createSinglyHorizontalTask` really loops seven times
  (`0x800ADEF0` compares 7), although the declared horizontal array has three
  entries and is followed by infinity-task storage. Before native activation,
  audit actual valid-call invariants and reproduce necessary console memory
  semantics through a documented architecture boundary, rather than allowing
  host undefined behavior or silently correcting retail logic.
- `src/Game/Camera/CameraContext.cpp::updateProjectionMtx` explicitly states
  that its current reconstruction needs major fixing (`0x80097708 / 0x2D4`).
  Direct import is not binary evidence. Its root setters also call projection
  update, while current retail setNearZ/setFovy symbols are only 8 bytes.
  The existing view-output boundary is distinct from a verified full original
  projection context; audit/recover this source before claiming that closure.

The two loop observations were read from the existing verified-DOL split at
`build/xanime-core-matrix-calculation-20260903/entrypoints/retail/asm/Game/Camera/`.
No new original-compiler comparison was performed in this audit. The verified
DOL is `build/compat-math-oracle/main.dol`, SHA1
`25c5959534b3c21246c6c7e42021b916b41fb578`.

## Coherent delivery order and evidence

1. Recover the six small scene/rail functions and close general bounded JMap
   attachment, zone resource/start/rail access, and real camera-code
   registration. Independently complete the original player animation query
   owner. These prerequisites can progress without changing the demo camera.
2. Import and compile the entire 45-type catalog, translators, chunk classes,
   creator/holders, effectors and manager/director graph. Use original
   construction. Resolve the documented architecture issues and missing
   platform providers explicitly; do not provide partial objects to make the
   constructor appear successful.
3. Activate the original director once in the existing camera phase. Route
   declarations and actor/event requests to it, then remove the superseded
   native selection/event ownership. Showcase publishes the real Mario target
   and stage resources; it must not choose a camera recipe.

Acceptance should demonstrate actual start-to-normal transition, CubeCamera
priority/zone selection, ground host-name/zone camera keys, authored through
and no-reset behavior, both event priorities and return to game, landing
release, and freeze/resume. Use genuine original holders/managers and retained
retail resources. A Gateway camera report should list every registered chunk,
matched BCAM row/type, and actual selected IDs while moving; it is evidence
of the generic pipeline, not a stage-specific selection rule. Preserve the
existing actual target and OnlyCamera/view tests through the ownership change.
