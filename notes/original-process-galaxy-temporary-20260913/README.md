# Original temporary galaxy data during transitions, 2026-09-13

The eleventh production run reached the authored HeavensDoorGalaxy scenario 1 transition, then failed because `GameDataFunction::getStarPieceNum` required the bounded StageSessionState. LLDB confirmed the actual call chain: `MR::requestChangeStageInGameMoving -> GameSequenceFunction::requestGalaxyMove -> GameSequenceProgress::resetGameDataAfterChangeScene -> GameDataFunction::getStarPieceNum`. The stack is in `notes/gateway-wakeup-demo-20260912/original-app-stage-session-backtrace.log`; the debugger process was killed after capturing it.

The original sequence owns `mGameDataTemporaryInGalaxy` before a scene exists. `OriginalSceneCounterQueries.cpp` now resolves that real field through GameSystem/GameSequenceDirector when GameSystem exists. The adjacent restart-ID accessors in `StageSessionGameCompat.cpp` use the same original owner, then call the exact original getter/setter bodies. The original counter/checkpoint helpers themselves remain unchanged.

An execution without GameSystem may still use its explicitly owned standalone StageSessionState. An actual process never falls back to it. No new session, temporary data, stage metadata, or default counter is installed.

References: the anonymous `getGameDataTemporaryInGalaxy` in `decomp/src/Game/System/GameDataFunction.cpp` and `decomp/src/Game/Util/SystemUtil.cpp`; the latter also contains both original restart-ID methods. Both affected native TUs compile (`native-compile.json` includes exact commands and source hashes). No tests or further production runs were performed; root owns the next build/run.

Production integration: eighteenth full smg-pc build passes. Twelfth real-disc Metal launch reaches the authored HeavensDoorGalaxy scenario-1 GameSequence handoff, passes the prior synthetic-stage-session error, then fails in the original worker on resource heap retirement with live layout owners. This is the next native lifetime fix; no loaded Gateway, wakeup or progression claim.
