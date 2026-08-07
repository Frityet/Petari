# Route shim migration

## Outcome

The host-only picturebook-to-Gateway route no longer lives in `src/Game`.

- `Game/Demo/PrologueDirector.{cpp,hpp}` are byte-for-byte identical to the root decomp.
- `Game/System/StorySequenceExecutor.{cpp,hpp}` are byte-for-byte identical to the root decomp.
- `Game/Util/SequenceUtil.{cpp,hpp}` are byte-for-byte identical to the root decomp.
- `Game/Util/JointUtil.hpp` and `Game/LiveActor/ActorCameraInfo.hpp` were also restored exactly so the unmodified PrologueDirector builds against the original matrix/camera surface.
- The old PC-only `StorySequenceService` and the replacement `StorySequenceExecutor` implementation were removed.

The debug handoff is now an externally configured `SceneTransitionRequestService` outside `src/Game`. It observes a generic `NameObj` live-to-dead edge and submits a normal `StageHostRequest`. The route smoke config supplies both the trigger and target:

```text
SMGPC_SCENE_TRANSITION_TRIGGER=name_obj_dead_after_alive:プロローグの絵本
SMGPC_SCENE_TRANSITION_SCENE=Game
SMGPC_SCENE_TRANSITION_STAGE=HeavensDoorGalaxy
SMGPC_SCENE_TRANSITION_SCENARIO=1
SMGPC_SCENE_TRANSITION_APPEAR_AFTER_INIT=1
```

Invalid or incomplete external configuration throws; it is not silently replaced. The same request service handles F10, but the key has no implicit target: it can submit only the externally configured request. No stage name is hidden in the input or compatibility services.
The after-load prologue handoff also requires a real current `UserFile`, a real `GameDataHolder`, and Mario data; missing save state is not treated as a valid substitute.

## Honest build boundary

The original `StorySequenceExecutor.cpp` and `SequenceUtil.cpp` are present verbatim, but are not compiled yet because their real dependency closure (`GameSystem`, `GalaxyMoveArgument`, movie/staff-roll sequence, and related retail sequence services) has not been imported. The build excludes those two translation units explicitly. `compat/SequenceUtilCompat.cpp` currently supplies only the one original API used by the compiled FileSelector, forwarding it to the generalized host request service. This avoids keeping a second fake StorySequenceExecutor in `src/Game`.

## Verification

- Release `smg-pc`: passed.
- Debug `smg-pc`: passed.
- A force rebuild after a concurrent `LiveActorModel` layout change passed, avoiding stale-object ABI mixing in the shared worktree.
- Aurora-native tests: 27/27 passed, including the external configuration and one-shot live-to-dead trigger test.
- Stage-start camera tests: 4/4 passed (optional real-disc case skipped when `SMGPC_REAL_DISC` was unset).
- Route smoke:
  - title frame 90: passed, nonblack `1.0000`, 22 render packets.
  - file select frame 1900: passed, nonblack `0.9998`, 27 render packets.
  - picturebook frame 7600: passed, nonblack `1.0000`, 25 render packets.
  - Gateway handoff frame 10350: passed, nonblack `0.0070`, 604 render packets.
- The parent integration run then force-rebuilt the shared tree and passed all four checkpoints again after the concurrent exact-resource cleanup.
- Gateway placement report remained honest: 242 total rows, 35 real created actors, 135 unsupported actors, 72 ignored helper-data rows, and zero model/alias fallback statuses.
- Source-closeness audit classifies all six route files as `exact-source`; see `source-closeness/source-closeness.tsv`.

The first combined route invocation had a transient Xvfb restart race on the picturebook case (`SDL: No available video device`). Re-running that case on fresh display `:336` passed. This was harness startup, not a game/runtime failure.

## Evidence

- `gateway-handoff-manifest.json`
- `gateway-handoff-expectations.json`
- `gateway-handoff-placement-report.md`
- `gateway-handoff-trace-validator.log`
- `gateway-handoff-frame-10350-320x240.png`
- `title-manifest.json`
- `file-select-manifest.json`
- `picturebook-manifest.json`
- `pc-game-root-route-diff.md`
- `source-closeness/`
