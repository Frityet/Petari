# Next tranche: execute CameraTargetPlayer

Read-only source audit, 2026-09-03. This note proposes the next implementation;
it does not activate an actor, change scheduling, or establish new binary
matching. The current camera backend executes original `CameraParallel` and
height-arrangement code, but `src/compat/MarioCameraTarget.cpp` still samples the
normal, unbound actor getters instead of running the original target lifecycle.

## Original source and missing closure

Root `src/Game/Camera/CameraTargetObj.cpp:95-240` contains every
`CameraTargetPlayer` method. No missing target body needs decompilation.
`configure.py:1125` still labels the complete translation unit `NonMatching`.
RMGK01 symbols identify the constructor at `0x800B0418` (0x74 bytes), movement
at `0x800B048C` (0x1A0), and `getLastMove` at `0x800B065C` (0x50), in
`config/RMGK01/symbols.txt:6557-6580`. No binary comparison was performed for
this audit.

The bounded import is the verbatim player-target methods and their local
`sZeroVec`, initially in a compatibility translation unit outside `Game/`.
Importing the entire original file additionally pulls in actor/demo targets
and duplicates the base constructor already supplied by
`src/compat/CameraLocalUtilRuntime.cpp`; that definition must have one owner.
The existing original `CameraTargetObj.hpp` is sufficient for the class.

The required absent MR wrappers are already written in root
`src/Game/Util/PlayerUtil.cpp`:

| Root line | Method |
| --- | --- |
| 27 | `getPlayerGroundingPolygon` |
| 95 | `getPlayerLastMove` |
| 406 | `isPlayerFlying` |
| 482 | `isPlayerInBind` |
| 842, 846 | `isPlayerInWaterMode`, `isPlayerOnWaterSurface` |
| 862, 866 | `getPlayerMovementTimer`, `getCameraCube` |

The mirrored PlayerUtil file is excluded by `src/Game/xmake.lua:170`; its
presence is not an active provider. Extract those exact wrappers together
with their narrow original accessors, rather than enabling the entire unit.
Root `src/Game/Player/MarioAccess.cpp` supplies `isOnGround:49`, `isInRush:86`,
`isFlying:106`, `getCameraCubeCode:118`, `isSwimming:122`,
`getGroundingPolygon:130`, `getLastMove:338`, `isOnWaterSurface:618`,
`getPlayerActor:634`, and `isInWaterMode:726`. Its `getBaseMtx:562` preserves
the actor's forced matrix branch; compare that with the currently active
`src/compat/PlayerUtilCompat.cpp:203` before relying on the bound target path.

Additional exact accessor bodies are root `MarioCollision.cpp:1909`
(`Mario::getCameraCubeCode`), `MarioSwim.cpp:86` (`isSwimming`),
`MarioJump.cpp:24` (`isRising`), and `MarioActorGravity.cpp:29`
(`MarioActor::getGravityInfo`). Camera cube lookup uses position plus gravity
times 100 while rising or in the specified swim state. It must retain that
branch and query the real CubeCamera area manager. These are existing bodies,
not new decompilation. Root `MarioActorCamera.cpp:28` retains its pre-existing
`isLongDrop` FIXME; do not relabel it newly matched.

## Ownership and verified timing

The stage/player adapter should own one persistent real `CameraTargetPlayer`
per attached `MarioActor`, set `mActor` before its first movement, and destroy
the target before actor teardown. The original constructor does not bind the
actor; root `CameraTargetHolder.cpp:31-34` performs that assignment. Keep this
Mario dependency outside the generic `CameraSystemService` and
`OriginalGameCamera` resource/pose wrapper. The wrapper can accept a scoped
original target reference, or consume one cached sample of its getters.

**Run target movement exactly once in the camera target phase, not in every
getter or reader callback.** Root `CameraDirector.cpp:103` connects the
director through `ObjUtil.cpp:334`; `CameraDirector::movement:116-121` calls
`mTargetHolder->movement()` before `updateCameraMan()`, and
`CameraTargetHolder.cpp:14-16` delegates once. Root
`Scene/SceneExecutor.cpp:27,59` executes Camera before Player, whose category
IDs are 0x02 and 0x25 in `include/Game/Scene/SceneFunction.hpp:10,45`.
Consequently the target observes the previous completed player movement,
not a new post-Player update within the same frame.

The current PC runtime publishes WPAD before camera calculation in
`src/runtime/RuntimeContext.cpp:816-825`, then executes scene movement at
line 847. `SceneScheduler.cpp:39-64` also places Camera before Player. Preserve
this ordering and current-input visibility. Do not move the target phase
after Mario. Repeated target movement in the same tick would see an unchanged
movement timer and set `mIsPlayerMoving` false; during a demo that incorrectly
zeros `getLastMove`. Multiple game/event consumers must share the single
phase update. Preserve director pause/movement-disable behavior too.

## Available state and validation boundary

Actual MarioHolder, scene gravity, demo activity, actor dead/clipped state,
and CubeCamera area managers already have providers. The showcase player
slice in `src/showcase/xmake.lua` supplies the live actor getter code;
`src/compat/MarioStateAccessCompat.cpp` demonstrates exact accessor extraction.
The current PC `MarioActor::movement` increments `_378` and populates ground
and shadow data from Binder contacts (`src/Game/Player/MarioActor.cpp:802-835`).
This is available state, not a claim of complete original movement parity.

The original target adds bound-matrix axes, Bee gravity queries, cached area
and triangle identity, and demo movement-timer handling. Validate these with
real owners, including shadow versus ground, independent side vector, normal
up normalization, forced bound matrix, Bee gravity, moving/stationary demo
ticks, repeated reads without repeated movement, and target retirement.
The complete import still needs compilation and runtime validation. Full
CameraTargetHolder/Director ownership and normal camera selection remain
separate closure work; no partial or unconstructed original objects should
be substituted.
