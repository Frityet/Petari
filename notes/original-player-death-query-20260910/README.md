# Original player death query — 2026-09-10

The first original CameraDirector frame reached CamKarikariEffector::update and failed because the old player service required an externally assigned death-state value. The actual MarioActor is now initialized, so that cached value and its setters have been removed.

PlayerActorBridge now exposes read_nerve_change_enabled. PlayerSystemService::player_dead_state queries this capability on every call and returns its inverse. Missing actors or capabilities remain unresolved. PlayerStateCompat retains the existing explicit exception for unsupported owners. No Game source changes are needed. GatewayMarioOwner's actual MarioActor callback is owned and published separately by the initialization agent.

Reference: decomp/src/Game/Util/PlayerUtil.cpp:47 implements MR::isPlayerDead as !MarioAccess::getPlayerActor()->isEnableNerveChange(). Retail 0x803F3540 calls getPlayerActor, calls isEnableNerveChange, then inverts the boolean. MarioActor::isEnableNerveChange at 0x802B5450 accepts only Wait and NoRush. The current original src/Game/Player/MarioActor.cpp query has those exact checks. Generic LiveActor's mIsDead flag is not used as a substitute.

Validation: direct LLVM 23 native compilation of RuntimeServices.cpp, PlayerStateCompat.cpp, PlayerUtilCompat.cpp, and migrated RestartStageSessionTests.cpp all exited 0. Link against actual current Game/common/Aurora libraries exited 0. Real RMGK01-disc test run exited 0 with 8/8 groups passed. The migrated test verifies unresolved generic ownership, live callback inversion, immediate false/true updates without frame synchronization. Full commands and logs are in result.json and adjacent logs. rg finds no old death-state setter/clearer callers in src or tests.

This test establishes service propagation and the surrounding stage/restart regression, not a rendered demo or native execution of every Mario nerve. The exact actual-actor callback and rendered camera remain part of the parent's subsequent demo validation. No global Xmake build was started by this subtask.
