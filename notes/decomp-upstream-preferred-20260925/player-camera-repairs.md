# Player and Camera merge repairs

The first serial `ninja -j6 -k0` baseline finished with exit 2 before these edits. I read `decomp/AGENT_DECOMP_GUIDE.md`. No competing build, staging, or commit was performed.

The mechanical upstream-preferred merge retained overlapping fragments from independently recovered functions. These files now match upstream merge input `cae17c10e9a8c59a019e81da7454736f71a84595` byte for byte:

| File | Why the complete upstream version was selected |
| --- | --- |
| `src/Game/Camera/CameraContext.cpp` | Removes the fork-local out-of-line MR::getScreenHeight, now supplied inline by upstream ScreenUtil.hpp. All CameraContext methods remain. |
| `src/Game/Player/MarioActorParts.cpp` | Repairs the hybrid shootFireBall braces and keeps upstream complete carry, transform, throw, and effect algorithms. All original methods are implemented upstream. |
| `src/Game/Player/MarioActorOffensiveMsg.cpp` | Repairs mixed radius/sensorRadius, pull/canPull, touching/isTouch and offset/diff names and the merged attack/item-flow blocks by selecting the full upstream bodies. |
| `src/Game/Player/MarioActorTakeMsg.cpp` | Repairs radius/distance and nextRadius/nextDistance mixtures in tryPullTrans, preserving upstream full item-pull flow. |
| `src/Game/Player/MarioCollision.cpp` | Repairs hit/hitFlags, height/distance and maxMove/maxSlide mixtures in checkGround. This also restores upstream distinct Recovery/Warp guards rather than the merged repeated 0x13 condition. All other functions are already upstream-equivalent. |
| `src/Game/Player/MarioActorRush.cpp` | Removes the malformed mixed endRush structure, duplicate tail and isJump/launched variable mixture; upstream has complete beginRush/endRush and all other current methods. |
| `src/Game/Player/MarioRabbit.cpp` | Restores the complete upstream state implementation consistently with its original offset member declarations. |
| `include/Game/Player/MarioRabbit.hpp` | Restores the matching upstream layout/member names and original inline notice implementation. The fork out-of-line notice body was the same true return, so no recovered behavior is removed. |
| `src/Game/Player/MarioActorSensor.cpp` | Its sole difference from upstream was the Rabbit field rename mJumpAnimationIndex; restores upstream _68 to match the header. |

`player-camera-repairs.json` records exact before/after hashes and upstream equality for these nine files. No nonoverlapping recovered method was discarded in this repair set. Native port source under root src/ was not modified.

The other Player/Camera failures in the first build are caused by a shared MessageEditorMessageTag redefinition in Screen headers outside this ownership scope. The root task owns that repair and serial whole-build validation. The edits above have source-equality evidence; a post-edit compile result is pending root validation.
