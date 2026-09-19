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

## Bounded replay after stomp restoration

`content-stomp-fixed-16000` completed all 16,000 frames normally in 326.125 seconds, exit 0 with PID 76171 reaped. The immutable main executable SHA256 was `1c822fdbe63f8466f7272e2f405579e0db447a20140a37642af2a57de940af16`. It includes the canonical stomp/sensor restoration, butterflies, mushrooms and WarpPods, but predates CrystalCage/standalone StarPiece activation and the subsequent zone-transform restoration.

The controller followed the current guide ID 846 into Talk1 by frame 3140, then stopped. The frame 4200 image shows the original Korean hide-and-seek dialogue. A single later A press at 8200 was consumed but did not complete that conversation; the collector remained Wait and the rabbits remained NoActive through the final trace at 15990. This is not minigame or catch completion. The attempted detached point driver produced no decisions or accepted input; it was gone when checked after the run. Its launch record is retained, but it provides no navigation evidence. Subsequent operators must stay supervised for their entire lifetime.

This run establishes bounded process survival along the observed route, not a replay of the earlier sensor-driven mushroom stomp. The separate actual-owner stomp regression remains the evidence for that restoration. The frame image also contains a thin ladder-like line into the sky; its rendering source is under investigation and visual parity is not claimed.

`operate_first_catch.py` is a supervised controller-only diagnostic operator for the next replay. It identifies the live guide and active rabbit from the current trace, advances conversation with separate A presses, and accepts an explicit world-space waypoint for the hiding-place approach. It never writes game state. Its importable camera-basis controller now takes a stop distance so a catch attempt can continue into normal sensor contact. It drains stale trace samples before publishing input and records every decision. This operator is prepared but has not yet established a catch.

## Two catches in the corrected world-coordinate replay

`content-world-first-catch-18000` completed all 18,000 frames in 372.303 seconds, exit 0, PID 81050 reaped. Its immutable main SHA256 is `b38e90868eb8ab1f1cdc72487c293cf0f15a3e50c6fd8b14adc846bec7a7d123`, including the original zone/rail getters, removal of eager copied-row transforms, original ModelObj initialization, CrystalCageM and standalone StarPiece activation. The source checkpoint is `066d765bed38f95270b4ea85434d87d0750fd663`.

The supervised controller started from fresh save data. The guide reached Talk1 by frame 2290, and its conversation led to collector Active by 2630. The actual bush SwitchCube revealed rabbit 907 by 3020. Ordinary sensor contact reached Caught at 3490, CaughtTalk at 3500, and CaughtEnd at 3620. The rabbit subsequently transformed into its original Tico; the frame 5500 image shows Mario, that Tico and the original Korean hint dialogue. This establishes the first capture, not completion of the whole minigame.

The first operator stopped at CaughtEnd, before the subsequent Tico conversation. The initial hole driver therefore held movement while the original dialogue correctly held Mario still. This was an external test-operator sequencing error. An updated operator supplied normal A presses and Mario resumed after the Tico talk. During this revision, two operators briefly overlapped: the old hole driver covered frames 6790–12410, and its replacement covered 11730–12230 before their shared temporary filename caused a FileNotFoundError. Both error logs are retained. The old process was explicitly interrupted and reaped. Later operators use a per-process temporary filename and an exclusive input-file lock. No game-side input or dialogue gate was changed.

The route also exercised the ordinary pipe transit and revealed rabbit 902 by frame 12200. The final operator selected that already active rabbit at 16790. Normal contact reached Caught at 17670, CaughtTalk at 17680 and CaughtEnd at 17800. At the final trace frame 17990, both captured rabbits were dead after their original transformations, the second Tico was still in Talk, the hole rabbit 912 remained Hide, and Rosetta remained dead. This establishes two sensor-driven catches and original pipe reveal, with no hole catch or Rosalina appearance claim.

The sampled trace, accepted controller revisions, every operator decision/error log, launch/result JSON and frame image are retained. Position/nerve/switch/story state were never written by the operator. The static neutral stick excluded physical movement outside scripted spans; ordinary physical buttons/pointer were not globally disabled. This is a scripted navigation run, not a claim that physical keyboard play or visual parity was fully tested.

A subsequent sequential route driver will require observing each post-catch Tico Talk and its completion before advancing. A fixed time after Caught is insufficient: the original transformation can begin its talk later than that interval. The next replay will retain all original story gates and use the same immutable main binary.
