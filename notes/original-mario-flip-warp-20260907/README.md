# Original Flip, Warp and Faint state closure

Restored the missing original MarioFlip update and all four missing MarioWarp virtual/update-jump methods from the verified Korean retail executable. Corrected two incomplete existing Warp methods required by that state: calcAxis and Mario::doCubeWarp. Resolved Faint's undefined external string pool with six exact retail literals. Changes were made in decomp first, freshly Wii-compiled and reviewed against relocated retail instructions, then copied byte-identically into native Game. No build-list edits or Mario.cpp changes.

| Method | Fresh retail match | Retail / compiled bytes |
| --- | --- | --- |
| MarioFlip::update | 99.76649% | 1456 / 1456 |
| MarioWarp::calcAxis | 99.729324% | 532 / 532 |
| Mario::doCubeWarp | 100% | 384 / 384 |
| MarioWarp::updateJump | 99.560814% | 592 / 592 |
| MarioWarp::start | 99.72656% | 512 / 512 |
| MarioWarp::update | 99.65328% | 1096 / 1096 |
| MarioWarp::close | 98.125% | 1024 / 1024 |
| MarioFaint::start | 97.28814% | 472 / 468 |
| MarioFaint::close | 99.6875% | 128 / 128 |

All three constructors and vtables remain 100%. Existing function scores do not regress; Faint start/close improve from 96.99/98.13%, Warp calcAxis from 49.65%, and doCubeWarp from 80.56%. Every newly recovered Flip/Warp routine preserves the direct-call sequence including virtual callback scheduling. Whole native LLVM23 object compilation succeeds for all three TUs using actual current headers. This is source/compile evidence, not a claim of live movement or warp gameplay.

Flip retains the original timed recoil phases, direction friction, animation/sound/effect changes, wall-normal reflection with original multiplication order, yaw decay and jump/stick exit gates. Warp retains its four authored modes, virtual update call during start, timers, sine-shaped trajectory, hide/restore fields, camera transitions, player-mode differences and original jump/fall exits. Null checks present in the recovered code are the retail checks, not new fallback behavior.

The old Warp arc code used the wrong angle for each mode, sqrt(sin(angle)) instead of geometric radius/height, and a fixed duration. Retail uses default pi/4, recovery pi*0.4, radius = half-distance / sin(angle), height = sqrt(radius squared minus quarter-distance squared), and integer radius/25 followed by the original minimum120 and authored mode2 override. The exact floating-point expression grouping is retained in source. doCubeWarp now looks up the actual pair, exits when absent, records that destination and copies its position before the original state transition. Parent owns the separately recovered WarpCubeMgr::getPairCube provider/header.

Faint's six CP932 strings are directly verified at retail0x805C64D8: 後方小ダメージ, 前方小ダメージ, ノーダメージ, 声小ダメージ, ダメージ and 基本. Direct literals replace the undefined lbl symbol and offset arithmetic; no invented external alias is added. String evidence also records every Flip/Warp retail pool entry and verifies its bytes exist in the compiled object.

`refresh-proof.py` reproduces six full Wii compilations and retail comparisons using the existing pinned compiler/header setup. Full objects and objdiff output remain local note artifacts; compact commands, source hashes, function/call evidence and retail instruction listings accompany this checkpoint. Remaining fuzzy differences are storage/register scheduling, constant labels and equivalent branch layout; source-level and retail-call evidence does not establish cross-architecture floating-point identity.

Three decomp paths and three matching native paths are listed in the checkpoint manifests. Parent owns commits and coordinated root build/runtime checks.
