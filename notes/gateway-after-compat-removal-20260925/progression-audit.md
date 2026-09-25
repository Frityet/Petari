# Gateway progression source audit after round 22

Read-only production audit on 2026-09-25, baseline `a71fc2ea7`. No source edits, builds, tests, or controller actions performed by this audit. Existing route notes were left untouched.

## Finding

**No concrete missing implementation or non-original bypass was identified in the three-catches-to-Rosalina state chain.** Do not add a stage-specific switch write, forced catch, or demo completion shortcut. Current source contains the original catch, forced conversation, Tico whiteout, authored demo action, and Rosalina appearance transitions. The current supervised route, rather than the 120-frame opening smoke, is the appropriate regression check after the owner migrations.

Historical evidence already reaches this endpoint: `notes/demo-system-verification-20260919/final2-run.md` records three catches at frames 3190/4690/10480, tower activation 11140, Rosalina alive/unhidden 11440, and normal exit after 30,000 frames. It explicitly does **not** prove an unobstructed rendered Rosalina. Earlier missing-creator and incomplete-route notes are not current blockers: round 17 subsequently imported the remaining Game sources and restored the factory.

During this audit, a read of the current operator log at frame 4970 showed two completed catches and Tico conversations (bush completed 3540, pipe completed 4730), with navigation toward the hole group underway. This is an intermediate observation of the parent's run, not a completed current acceptance result.

## Actual transition owners

| Gate | Current owner / evidence | Failure evidence to collect if the live route stops |
| --- | --- | --- |
| Catch contact | `RunawayRabbit.cpp:218`, `isCaughtable():472`: Catch sensor/player contact requests the real puppetable demo. | Catch-sensor overlap, nerve, not-catchable timer, demo request state. |
| Catch completes | `RunawayRabbit.cpp:385/416/432/468`: seven steps plus ground contact, forced talk completion, then Toss animation completion and Stop. | Ground binding for Caught; real message end for CaughtTalk; BCK end for CaughtEnd. |
| Three distinct groups | `RunawayRabbitCollect.cpp:200/224`: Stop is counted once, group bits select `appearMamaComment`. Pipe alternatives are one group. | Collector group IDs/completion bits and the final Tico's `mIsAllCaught`. |
| Tower demo starts | `RunawayTico.cpp:124/230/249/284`: final talk ends, whiteout completes, WhiteIn step 75 starts `チコガイドデモ` at `高楼出現[デモ]`. | Talk end, wipe state, current Tico nerve, requested executor/part. |
| Tower and Rosalina appear | `DemoActionKeeper.cpp:87` implements authored ActionType4/5 switch writes and ActionType0 appearance. `DemoTimeKeeper.cpp:37` advances the real time sheet. | Executor cast membership, current part/step, Rosetta switches A1015/B90 and appearance state. |

The preserved real sheet (`notes/original-rabbit-tower-chain-20260919/tico-guide-sheet.json`) writes Rosetta's A/B switches in `高楼出現[デモ]`, then appears her in `[デモ後]` after the first part's 300 frames. `RosettaDemoHeavensDoor1` registers the required callbacks and initially makes her dead. Its later proximity-500 spin-get trigger occurs **after** the requested spawn endpoint.

`EventDirector.cpp` is donor-equivalent after normalizing CP932 literals/includes. It owns star/comet/time-attack facilities, not the rabbit-to-tower transition. No audio-completion predicate gates the inspected catch/whiteout/demo timing chain; intentionally disabled sound output is not evidence of a progression block here.

## Concrete native source drift, not demonstrated endpoint blockers

1. **RunawayTico initialization UB:** `src/Game/NPC/RunawayTico.cpp:56` declares `s32 colorID;`; `decomp/src/Game/NPC/RunawayTico.cpp:51` initializes it to `arg0`. When `tryRegisterDemoCast` returns false, native code passes an indeterminate value to `initBase`. Restore the donor initialization as a narrow correctness fix. The Gateway guide/comment Ticos are authored demo casts, and the live run has already completed two comments; this has not been established as the current route blocker.
2. `RosettaDemoHeavensDoor.cpp:130` omits the donor `スピンゲット[デモ1]` condition from a level-sound predicate. This is a later audio-only discrepancy; it cannot explain failure to spawn Rosalina. Restore when reconciling that source, not as a progression fix.

The seemingly empty RunawayRabbit NoActive/TryCaughtDemo nerves are also empty in the donor. Likewise, the collector's final `MR::isValidSwitchA(this)` is present in the donor; the real tower switch writer is the demo action sheet. Neither justifies an invented replacement behavior.

## Next action

Complete the current input-only route and inspect its first failed gate if any. If all three catches and the authored tower part complete, prioritize a direct rendered view of Rosalina: that remains the explicit historical evidence gap. The old Grand Star audit concerns later spin/crystal/launch-driver progression and supplies no additional proven blocker before this endpoint.

## Subsequent authorized donor restoration

Both differences above are now corrected in the native sources: `RunawayTico::init` initializes `colorID = arg0`, and `RosettaDemoHeavensDoor1::exeDemo` includes the donor spin-get demo1 sound condition. Both files were clean before these scoped edits; exact snapshots are under `donor-drift-fixes/before/src/Game/NPC/`. No host-specific reason for either omission was found. Source inspection confirms the donor expression/initialization; no build, test, or commit was performed by this lane.

The supervisor separately reported all three catches and Rosalina alive/unhidden by frame 7100 in the already-running executable, before these two changes. These restorations therefore are not credited with that successful progression. The running process was left undisturbed.
