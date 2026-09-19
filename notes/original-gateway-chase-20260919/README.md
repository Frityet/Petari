# Original controller-only guide chase

The external `follow_actor.py` operator reads the debug actor trace and writes
only ordinary controller spans through the opt-in live input file. It never
writes actor memory, game flags, positions, switch state, or story state. Actor
IDs are runtime generations and must be selected from the current run. The first
operator used the previous run's ID767 and exited before writing; the guide was
ID769 in this process.

`live-guide-chase-12000` used fresh save data, eight explicitly held A presses
through frame2110, then a camera-derived stick controller. The original
DemoRabbit entered Guide by frame2310. The first follow attempt then veered away
under changing camera/original movement behavior; it does not establish a
compatibility defect. The guide eventually returned to Wait. Neutral input was
accepted at frame3496, stopped Mario normally, and the operator retried from
frame3900. The game window loop exited normally after4419completedframes in
377.868seconds, exit0 and PIDgone, before the12000frame limit. This is not a
completed bounded12000frame run. Rosalina remains inactive and no rabbit catch
is claimed. The old external driver was explicitly stopped after game exit.

The user's next priority is fixing slow runtime. This chase used an unoptimized
debug executable, SHA874c080f050c3c532af8379b5fe74ac96c9366b4a451be84210e069af30757f6.
Performance measurements and the optimized build are tracked separately under
`../original-performance-20260919/`. No movement workaround was added.

## Optimized content run preparation

The user accepted the measured late-gameplay34FPS and requested content work to continue. The next fresh run will include the exact retail-verified Mario::retainMoveDir restoration, all14 PunchingKinoko placements and both original WarpPods. Trace interval10 records clipping validity as well as clipped state, so camera visibility and original movement suspension can be distinguished. Controller input remains external and explicit; no story-state writes are allowed.

## Original guide and minigame progression observed

`content-o2-guide-12000` used main SHA256 `d71704b6e2ed0e6805a00c450e46c30e930c4fecba81d85bdd6becfea765b877`, fresh save data, the same eight opening A presses, then the external trace-following controller aimed at this run's guide ID843. The guide reached its original Goal nerve by frame2670, and original fade/reposition/conversation handling reached Talk1 by2790. The driver immediately published neutral stick and stopped. This substantially exceeds the preceding chase's progress; it does not isolate the movement restoration from the finer controller feedback interval as a cause.

A second explicit A sequence beginning4260 advanced the conversation. The original collector reached Active by4360 and all four rabbit alternatives reached their original Hide state. These changes came from the original authored demo and switch/message logic. No actor position, nerve, switch, event flag or story state was set by the test operator. `content-o2-progression.json` records the actual trace transitions.

Unscripted movement first appears at frame4400, after the driver's stick span had ended2880. Later movement combines that input with the scheduled A buttons, so this phase is not a deterministic controller-only replay. An optional user-control clarification was sent; future scripted buttons were cleared when the unscripted input was noticed. The process had already crashed by then, so that final neutral revision was not consumed.

The process exited with SIGSEGV (-11), PID68211 reaped, after111.730seconds, with last trace frame4800. It did not complete the requested12000frames; the scheduled5500 screenshot/layout dump was never produced. No catch or Rosalina appearance is established.

The matching macOS crash report identifies MultiEmitter::create -> EffectKeeper::createEmitter -> MarioActor::trampleJump -> MarioActor sensor handling -> PunchingKinoko::attackSensor. This is a real world/sensor-driven mushroom contact, beyond the earlier actor-initialization probe. `content-o2-crash.json` retains only the relevant report fields and stack, omitting unrelated system metadata.

The native MarioActorSensor reconstruction used incorrect stomp animation/sound/effect literals, including effect `踏み`; the canonical/retail effect is `ふみつぶし`. Independent retail checks also found wrong state fields/constants in that module. The complete canonical module is being restored with CP932 wrappers only, and a separate actual-owner action regression will validate its real resources. No null-emitter fallback or mushroom-specific suppression is added. This restoration is subsequent work, not part of the crashed executable.

Published logs/traces are stored as gzip with deterministic timestamps; their uncompressed names in launch metadata identify the original local capture paths.
