# RMGK02 DemoRabbit reconstruction

Updated: 2026-08-06T20:03:18Z

## Outcome

`src/Game/NPC/DemoRabbit.cpp` now reconstructs the full original rabbit actor instead of stopping after archive selection. All 48 code symbols are mapped; 32 are exact, and the 4,668-byte `.text` section measures 99.46015%. The lowest remaining functions are `updateRun` at 98.23364% and `exeGuide` at 98.29214%.

The reconstructed actor retains the original general state machine: Appear, Demo, Talk0/Talk1, Wait, Guide, Goal, Runaway, Change, and StartBGM. It also restores the rail-guided movement, collision response, talk/time-keep actions, model choice, sounds, clipping transitions, and post-guide BGM behavior. Target `.data` evidence corrected the demo action to the exact Shift-JIS string `フェードアウト`.

No `pc-port` Game actor was added in this slice. Keeping the root reconstruction separate makes its remaining PC dependencies explicit instead of replacing the sequence with a route-specific stand-in.

## Placement evidence

`HeavensDoorMysteriousZone` layer A contains exactly three `DemoRabbit` rows, all in demo group 0. Cast 0 alone has message 0 and `CommonPath_ID=0`; casts 1 and 2 have neither a message nor a rail. This matches the actor logic:

- cast 0 loads `TrickRabbitBaby`, owns the talk/guide/fade/talk/runaway actions, and follows the guide rail;
- casts 1 and 2 load `TrickRabbit`, wait in Demo, and register only Runaway.

The compact row evidence is in `artifacts/heavensdoor-demo-rabbit-placements.tsv`.

## Verification

From the repository root:

```text
ninja build/RMGK02/src/Game/NPC/DemoRabbit.o
[focused object succeeds]

ninja
[default RMGK02 build succeeds]

sha1sum -c config/RMGK02/build.sha1
build/RMGK02/main.dol: OK
```

The exact DOL SHA-1 remains `54b71431af0d509097bfdef4ec28617afc487e89`.

`ninja all_source` is not a validation target for this slice: it still fails in unrelated pre-existing CameraDirector and DemoDirector/DemoTimeKeeper/DemoExecutor source paths. The focused DemoRabbit object and normal exact build are clean.

## Honest PC import boundary

The PC side still needs a source-close `NPCActor` base and generalized providers for demo-action nerves/functors, time-keep fade and talk, rail movement, collision line queries, quaternion blending, and the Tico-guide game-event flag. In particular, the current host `tryRegisterDemoCast` returns false, which would suppress the fresh-first-visit cast behavior if DemoRabbit were copied over prematurely.
