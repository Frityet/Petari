# Original fixed-point camera and player orientation

The PC `Game/Camera/CameraFixedPoint.cpp`, `CameraFixedPoint.hpp`,
`CamTranslatorFixedPoint.cpp`, and `CamTranslatorFixedPoint.hpp` are
byte-identical copies of their root source/header counterparts. No compiler
fix, Game algorithm, constant, or layout change was necessary. Root
`configure.py` marks these original translation units Matching; this import
does not claim a new binary match. Whole-file hashes and exact actor getter
body correspondence are recorded in `source-correspondence.json` and checked
by `verify-source.py`.

`CameraHolder.cpp:69` associates `CAM_TYPE_EYEPOS_FIX` with this original
controller. Its translator passes `mWPoint` and `mNum1` directly to
`CameraFixedPoint::setParam`; these fields carry a position and mode, with no
retail pointer payload. `reset` copies the actual manager pose and calls
`calc`. `calc` retains the previous watch direction, calls the original
`CameraLocalUtil::makeWatchPoint` with `0.1f / 15.0f`, transforms the authored
eye through the zone matrix, and selects the original up behavior:

- Mode 0 transforms world up through the zone rotation.
- Mode 1 rotates the previous up using the quaternion between old and new
  watch directions.
- Mode 2 reads global `MR::getPlayerUpVec`, independently of the event target.

All modes publish the target's up as watch-up. The camera does not construct
a height arranger. The persistent `OriginalGameCamera` and
`EventCamera` integration, common parameter application, manager safe pose,
and retained local-offset state are described in `integration.md`. No native
camera calculation is added.

## Original player-vector contract

Root `src/Game/Util/PlayerUtil.cpp:131-141` routes all three orientation
queries to the player actor. Root `src/Game/Player/MarioActorCamera.cpp:84-94`
copies these fields directly: up is `MarioActor::mUpVec`, front is
`Mario::mFrontVec`, and side is `Mario::mSideVec`. These three getter bodies
already exist unchanged in PC Game. They neither normalize the vectors nor
derive them from render/base-matrix columns. `CameraTargetPlayer::movement`
is a distinct consumer: it selects base-matrix axes while bound and normalizes
its cached up afterward.

The extracted retail RMGK01 DOL was checked again with SHA1
`25c5959534b3c21246c6c7e42021b916b41fb578`. Its six accessor disassemblies
confirm the source contract:

| Function | Address / size | Retail behavior |
| --- | --- | --- |
| `MR::getPlayerUpVec` | `0x803F3790 / 0x34` | Gets the player actor, then calls `MarioActor::getUpVec`. |
| `MR::getPlayerFrontVec` | `0x803F37C4 / 0x40` | Gets the player actor, then calls its virtual front getter at vtable offset `0x80`. |
| `MR::getPlayerSideVec` | `0x803F3804 / 0x34` | Gets the player actor, then calls `MarioActor::getSideVec`. |
| `MarioActor::getUpVec` | `0x802B88F0 / 0x10` | Copies the vector at actor offset `0x2D0`. |
| `MarioActor::getFrontVec` | `0x802B88C8 / 0x14` | Loads Mario through actor offset `0x230`, then copies vector offset `0x208`. |
| `MarioActor::getSideVec` | `0x802B88DC / 0x14` | Loads Mario through actor offset `0x230`, then copies vector offset `0x310`. |

The actor methods tail-call vector assignment at `0x80018E78`. The MR helpers
use `MarioAccess::getPlayerActor` at `0x803047E8`. There is no matrix lookup or
normalization in these instruction ranges. Raw function slices and LLVM
disassembly remain in ignored `build/original-fixed-point-camera-20260903/`;
the note retains only their addresses, sizes, and hashes. This is live
instruction inspection of existing decompiled functions, not a new compiler
or object-diff matching result.

## Compatibility ownership and regression

`PlayerActorBridge` now has optional `read_up_vector`, `read_front_vector`,
and `read_side_vector` capabilities. Each takes a concrete `LiveActor`
reference and an output vector. `PlayerSystemService::copy_actor_*_vector`
calls the installed reader without changing its result. `PlayerUtilCompat`
prefers those readers; the real Mario owners in Showcase, Mario walk, and
the spin checkpoint call the actual typed `MarioActor` getters. The front
getter remains a virtual call through that typed reference. Generic actor
fixtures without these capabilities retain the previous normalized
base-matrix fallback. An absent player still follows the existing visible
unavailable behavior, and null output pointers remain no-ops.

`MarioGatewayWalkTests::verify_mario_camera_target_accessors` now samples
raw up `(0,2,0)`, front `(3,0,0)`, and side `(0,0,-4)` through all three global
queries. It repeats the queries while bound to a live host with a real
Binder and a forced matrix whose side/up/front axes are Y/Z/X. The raw global
values must be preserved while the actual bound `CameraTargetPlayer` uses
those matrix axes. The existing unbound camera target check continues to
require normalized up and the distinct shadow position. All modified Mario
fields are restored before the assertions. This catches accidental
normalization, swapped axes, and reuse of bound target or render-matrix state
for the global actor accessors.

See `integration.md` for the completed build and live-fixture validation.
FixedPoint event tests distinguish mode-2 global player up from both render
matrix up and the event target's watch-up.
