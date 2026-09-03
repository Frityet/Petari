# Original collision camera registration and complete camera registry

This audit follows the current root source and the verified RMGK01 rev0 DOL, SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`. The complete registry is not installed in the native scene. The existing native `OriginalGameCamera` executes selected original controllers; it is not an original `CameraHolder`, `CameraParamChunkHolder`, or `CameraDirector`.

## Actual registration and authored-data order

`src/Game/Map/CollisionParts.cpp:38` initializes the actual collision server, sensor, transforms, category and placement zone, then calls `MR::initCameraCodeCollection(sensor->mHost->mName, zoneID)`, `KCollisionServer::calcFarthestVertexDistance`, and `MR::termCameraCodeCollection`. The KCL traversal in `src/Game/Map/KCollision.cpp:64` registers camera codes only for actual visited prisms with valid attribute iterators, through the real `CollisionDirector::mCode`. Iterating every PA row would change this contract.

`src/Game/Camera/CameraPolygonCodeUtil.cpp` routes these calls through the actual `CameraDirector`. Its three forwarding methods use its actual `GameCameraCreator`. In `src/Game/Camera/GameCameraCreator.cpp`:

- The constructor zeroes a 256-byte code array. `initCameraCodeCollection` selects host name and zone; it does not clear that array.
- Codes below 255 set their corresponding byte. `termCameraCodeCollection` visits 0 through 254, clears each used byte, and registers `g:<host-name>:<code>:0` in the selected zone. It then clears the current host and zone. Duplicate chunks use original zone/name identity; no default chunk is fabricated.
- `scanArea` asks the actual `CubeCameraMgr` to run `initAfterLoad`, visits its actual areas, and registers valid authored camera IDs as `c:%04x` in each area's placement zone.
- `scanStartPos` visits the original recursive stage start-object count, gets each start's original `JMapIdInfo`, and registers `s:%04x` in its zone.

The complete close sequence is in `CameraDirector::closeCreatingCameraChunk` (`src/Game/Camera/CameraDirector.cpp:395`): scan areas, scan starts, create start-animation/talk/subjective chunks, let `CameraManGame::closeCreatingCameraChunk` create its defaults and zoom chunks, load all zone camera parameters, sort, set the start flag, and start the original start-position mode. BCAM loading updates only matching registered IDs and skips protected chunks. Creating every BCAM row eagerly, sorting before registration finishes, or treating placement count as ZoneList count would change original behavior.

## Smallest complete construction graph

A genuine registry test owner can construct the full original `CameraHolder`, then its real `CameraParamChunkHolder`, then `GameCameraCreator`. The holder constructs **45 actual camera classes**, their actual translators (**44 translator source files plus `CameraSubjective`'s original inline `CamTranslatorDummy`**), and all virtual tables. Its default index is found by the original catalog-name scan for `CAM_TYPE_XZ_PARA`; it must not be replaced by a numeric constant. `Camera` creates a real `CameraPoseParam`; relevant subclasses create the real `CameraHeightArrange`. `CameraFix` additionally creates a real `CameraTargetMtx`, and `CameraAnim` its original two data-accessor owners.

The parameter holder allocates its original 1024-pointer array, starts unsorted and empty, and creates Event/Game/base subclasses from the ID prefix. It preserves original copying, comparison, version arrangement and sorting. Its original capacity is unchecked, and sorted binary lookup assumes nonempty storage; those are original caller contracts, not permission to silently discard registrations or invent fallback chunks.

That three-object graph is a coherent **construction/registration fixture**, not a production `CameraDirector`. The unchanged production MR routing still requires a genuinely constructed director. Its constructor also creates the real manager stack, OnlyCamera/poses, rail holder, register holder, target holder, shaker, view interpolator, cover, rotation checker and Game/Event/Pause/Subjective managers. Do not assign fields in an unconstructed director, construct a reduced holder, or bypass collection merely to activate CollisionParts. A subsequent complete owner tranche must retain this original graph and its stage/scene dependencies before native activation.

## Compilation evidence and actual missing closure

`probe-native.py` derives the catalog directly from current root `CameraHolder.cpp`, then compiles its 45 cameras, 44 translator TUs and eight registry/reader TUs using actual cached native Game flags plus original-header fallback. It writes only ignored isolated objects. The initial result was **82/97 TUs**. After the bounded math closure and the two explicitly staged compiler adaptations, the result is **96/97 TUs**, with only CameraDPD rejected for its unavailable real Star Pointer depth accessor. `native-probe.json` records the current source hashes and diagnostics; no CameraHolder or full registry source was imported into production. This is a compile probe, not link or runtime evidence.

Twelve camera TUs require the original `TVec3f::angle`, `orthogonalize`, and/or `orientation` surface. `CameraDPD` requires the real `MR::getStarPointerWorldPosUsingDepth` owner described below. `GameCameraCreator` redundantly includes SDK `<mem.h>` despite already including `<cstring>`. `DotCamParams` names the original raw `JMapData*`, whereas native `JMapInfo` retains typed `DataCompat` in a shared pointer. These last two failures need narrow include/type adaptations, not replacement algorithms.

`native-link-frontier.json` records the initial 82-object snapshot of public Game/camera/math unresolved symbols after subtracting the successful graph's definitions and current native archives. The missing constructors in that report belong to the failed TUs and are not missing root decompilation. The real dependency frontier includes:

- CameraDirector registration, original reset/force-change/subjective/manager and view interpolation state; `CameraLocalUtil` register and reset helpers must read those real owners.
- `CameraRegisterHolder` and its actual matrix/vector registrations; `CameraRailHolder::getRider` and retained real rails. Native `SceneSystemUtilCompat::getCameraRailInfo` still explicitly rejects unavailable rail ownership.
- Original `MR::getZoneNum`, `getStageCameraData`, `blendAngle`, and `convergeRadian`. The math bodies already exist in root; the stage functions require the real scenario/stage catalog.
- Actual TripodBoss scene-object presence and joint access (`src/Game/Boss/TripodBossAccesser.cpp`). The original presence check can correctly report an absent real scene object; a fixed false provider or fabricated matrix would not be equivalent.
- The Star Pointer callback/depth/world-position lifecycle. A symbol-only link provider would not establish it.

This frontier is incomplete until the failed source files compile, and symbols present in an archive are not proof that every original behavior behind them is available.

## Reader lifetime and remaining root decompilation

The older camera-selection note's claim that native `JMapInfo::attach` is wholly unavailable is obsolete. Current `resource/JMapResource` supplies bounded retained lookup; unregistered raw pointers are rejected. Native `JMapInfo::DataCompat` retains entry count and the string cache, and copied JMapInfo objects share this identity. DotCam's end check must compare that same identity and count. It must not receive a forged serialized header.

`CameraParamString::copy` (`src/Game/Camera/CameraParamString.cpp`) **borrows** nonempty string pointers. Therefore the attached BCAM JMap resource/string cache must outlive all parameter chunks, even after the local `DotCamReaderInBin` is destroyed. The eventual original stage camera-data provider must retain the raw archive alias and its typed JMap owner for that lifetime. Current generic JMap ownership can supply this boundary; it does not itself install the full camera registry.

Recovered here: root `MR::getStartPosNum` in `src/Game/Util/SceneUtil.cpp`. It delegates directly to `getStageDataHolder()->getStartPosNum()`. The existing StageDataHolder method recursively sums all its start-object tables and child holders. `verify-original.py` compiles the real root TU with GC3.0a3 Game flags: **100% objdiff, all nine relocated instructions equal current retail**, address `0x803F757C`, size `0x24`. No native source mirror or stage-count approximation was added. Reproduce with:

```sh
python3 pc-port/notes/original-camera-registration-20260903/verify-original.py
python3 pc-port/notes/original-camera-registration-20260903/probe-native.py
```

The immediate original rail/start-animation construction path was subsequently recovered and verified in `../original-camera-stage-rails-20260903/`: `StageDataHolder::getCommonPathInfoElementNum` (`0x803474C8`, `0x44`), `MR::getPlacedRailNum` (`0x803F7AF0`, `0x54`), `getCameraRailInfo` (`0x803F7B44`, `0x5C`), `getCameraRailInfoFromRailDataIndex` (`0x803F7BA0`, `0x74`) and `getCurrentScenarioStartAnimCameraData` (`0x803F7CA0`, `0x98`). All eight functions in that recovery, including the necessary iterator-return corrections and private lookup helper, now have exact relocated retail instructions. Native director/rail ownership remains subsequent work.

## Star Pointer dependency: real owner required

`MR::getStarPointerWorldPosUsingDepth` (`src/Game/Util/StarPointerUtil.cpp:491`) returns the selected actual `StarPointerController::mWorldPos`. `StarPointerFunction::getStarPointerDirector` reads `GameSystem::mObjHolder->mStarPointerDirector`. The real director constructor (`src/Game/Screen/StarPointerDirector.cpp:21`) creates two actual controllers, the transform holder, and `StarPointerPeekZ`, binding each callback info pointer to its controller.

`StarPointerPeekZ::setDrawSyncToken` snapshots GX projection/viewport parameters and schedules the original draw-sync callback. The callback validates screen coordinates, converts to framebuffer coordinates, calls `GXPeekZ`, stores `mZDepth`, and marks `mDrawReady`. On the next actual director update, the connected controller runs `storeDataFromCallback`, `storePastPointingData`, then `updateDpdInfo`. Only a ready depth sample unprojects the retained screen/depth value through `TDDraw::invProject` and the retained Star Pointer camera matrices into `mWorldPos`; it then updates view distance, copies past info, and clears readiness (`src/Game/Screen/StarPointerController.cpp:38`). Without a new sample it retains the last actual world position.

Current native pointer helpers use WPAD screen history and the native StarPointer service; they do not own this original controller/depth callback pipeline. Aurora's depth snapshots and real GX draw-sync/peek surface are prerequisites, but are not themselves an original StarPointerDirector. Closing CameraDPD requires the actual controller/director/transform/PeekZ lifecycle, GameSystem ownership, original update ordering, and the TDDraw inverse projection/JUTVideo/input providers. The root `TDDraw::invProject` body was subsequently recovered (`0x80404078`, size `0x234`); `../original-inverse-projection-20260903/` verifies every instruction across all eight paths. It has not been activated natively. No new depth accessor, guessed plane intersection, cached-zero position, or fabricated pointer owner is introduced here.

## Next executable tranche

The bounded SDK/compiler surface and repeat probe are documented in `vector-math.md`. Next build a real complete catalog/parameter/creator fixture using the recovered stage/rail methods against retained authored BCAM; test duplicate IDs, zone identity, code 254/255 behavior and collection-before-load. Production activation must follow construction of the original director and actual scenario/stage/CubeCamera/rail/register/Star Pointer owners, preserving original close order and resource lifetimes. This audit does not claim the complete camera system or collision registration is active.
