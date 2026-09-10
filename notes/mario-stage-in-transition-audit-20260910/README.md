# Mario stage-in terminal animation audit — 2026-09-10

The current decompilation has an inverted boolean in `XanimePlayer::updateBeforeMovement`, mirrored into the port. This is a concrete reference mismatch that explains the completed StageInA remaining selected. The audit made no production, test, or decomp changes and ran no build.

## Exact reference discrepancy

`src/Game/Animation/XanimePlayer.cpp:412` and the same reference source currently require `!_7E` in the completed-animation default handoff. Retail `RMGK01` function `updateBeforeMovement__12XanimePlayerFv` at `0x8001BCF4`, size `0x90`, instead contains:

```asm
8001BD28  lbz   r0, 0x7e(r3)
8001BD2C  cmpwi r0, 0
8001BD30  beq   .L_8001BD50
```

`.L_8001BD50` skips the default-animation call. Nonzero `_7E` continues to the attribute check (0 or 3), then calls `runDefaultAnimation` at `0x8001BD4C`. The correct predicate requires `_7E`, without negation. The complete bounded original assembly is preserved in `retail-update-before.asm`; original symbol address is in `decomp/config/RMGK01/symbols.txt:791`.

Both actual XanimePlayer constructors set `_7E = true`. Original Mario rush setup deliberately clears it (`MarioActorRush.cpp:28`); resetCondition restores true at line77. That is consistent with enabled automatic return by default and suppression during actor-driven rush animation, and inconsistent with the current negated predicate.

## Why the trace looks frozen

MarioActor init chooses `ステージインA` for its default initial-animation case (`MarioActor.cpp:374`), which maps to `StageStartGround`. Its animator uses a genuine XanimePlayer with default animation `基本` (`MarioAnimator.cpp:73–80`). Original LiveActor movement updates the actual ModelManager, which calls Xanime updateBeforeMovement and updateAfterMovement.

`after-layout-phase.log` first reports the endpoint at tick65 (line631) and still reports `bck_frame=63.9990005`, `bck_rate=1`, `bck_end=64` through tick600. This endpoint/rate combination is not itself a broken stop predicate: native J3DFrameCtrl mode0 clamps to `end - 0.001`, clears rate, and sets terminal state bit1. Original Xanime updateAfterMovement preserves the previous rate when completion occurs (retail `0x8001BE18–0x8001BE24`) and does not advance an already completed controller. Therefore rate1 is compatible with a completed animation. Xanime isTerminate and actual ModelManager isBckStopped inspect terminal state bit1.

On the next movement, the wrong negation prevents returning to the default animation while `_7E` retains its constructor value true. MarioAnimator isAnimationStop tests whether current animation equals default; with StageInA still selected it remains false. The stage-in callback itself only emits the authored dust effect at frame50; closing it on terminal state does not switch the animation.

## Existing regression must be corrected with the recovery

`tests/OriginalXanimePlayerTests.cpp:310–315` currently asserts that constructor `_7E=true` holds a terminated group and clearing the flag returns to default. This test repeats the reference error. The suitable existing target is `smg-pc-original-xanime-player-tests`.

Recommended next bounded change: follow AGENTS_DECOMP_GUIDE.md to correct the single predicate in decomp, compile/compare the Wii function, copy that source correction to the port, and fix the regression to exercise both true-enabled automatic return and false-suppressed return. Keep explicit stop and countdown behavior covered. Then rerun the original-player fixture and actual demo past tick65 to observe the animation handoff.

This audit does not establish that the same correction fixes grounding, position drift, or jumping. Those require another live trace after the genuine animation handoff. It does establish a concrete wrong branch before those independent questions need to be resolved.
