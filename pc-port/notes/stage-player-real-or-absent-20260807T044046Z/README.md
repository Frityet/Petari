# Stage player real-or-absent boundary

## Outcome

The compatibility-owned `StagePlayerRuntime`/`StagePlayerActor` implementation was removed completely. The production build no longer compiles or references that stand-in, and its dedicated movement, input, spin, follow-camera, and actor tests/target were deleted with it.

`StageHostScene` now does only scene-generic work:

- it clears the previous scene's player attachment at the stage boundary;
- it resolves and retains genuine `StageStartInfo` data;
- it resolves and installs a genuine stage-start camera independently of player availability;
- it creates the scene-owned retail `ClippingDirector` generically alongside the other required stage scene objects;
- it never constructs a Mario-shaped actor, derives player motion, consumes player input, creates spin sensors, or creates a follow camera;
- after placement initialization it reports either a genuinely attached player actor or the explicit `stage_player_unavailable` boundary.

The FileSelector remains the real source lifecycle. No title, file-select, picturebook, stage, actor-name, or route-specific exception was added.

## Strict route policy

The route validator was made stricter rather than taught to accept the missing player:

- `title` and `title_decide` require live `TitleLogo` layout packets and forbid `sequence:title_activation_unavailable`;
- `file_select` and `file_confirm` retain the same forbidden event and their existing strict visibility/BTP checks;
- `gateway_handoff` no longer injects `SMGPC_SCENE_TRANSITION_*` environment variables, so HeavensDoor must be requested by the real sequence;
- the handoff still requires Mario render evidence, but if it reaches a stage before MarioActor is linked it also validates the explicit `reason=real_mario_actor_not_linked` diagnostic.

The strict title check is intentionally red until real Mario auto-rush exists. The retained trace proves the failure is explicit rather than masked:

- `sequence:title_activation_unavailable` count: 1;
- `player:stage_player_unavailable` count: 1 for the real FileSelect `StartInfo` row;
- `TitleLogo` state: dead, zero render packets.

Evidence is under `strict-title/title/` (`.trace.sqlite`, `.png`, application log, and validator log). Earlier `route/` and `route-pass/` captures record the validator audit that led to strengthening the checkpoint; they are deliberately not claimed as passes. `gateway-route/` is an interrupted pre-cleanup capture and is retained only as an audit log of removing the forced transition.

## Focused verification

- `xmake build smg-pc-game`: passed before the concurrent LodCtrl source migration began.
- `xmake build smg-pc`: passed before the concurrent LodCtrl source migration began.
- `xmake run smg-pc-player-util-real-or-absent-tests`: 4/4 passed. The added case proves clearing player state removes the actor/matrix while a genuine camera pose remains independently available.
- `xmake run smg-pc-stage-start-camera-tests`: 4 tests passed (real-disc case skipped unless `SMGPC_REAL_DISC` is set).
- `xmake run smg-pc-aurora-native-tests`: 27/27 passed.
- `rg` over production/tests/scripts found no `StagePlayerRuntime`, `StagePlayerActor`, stage-player motion helper, `stage_player_created`, or follow-camera compatibility symbols.
- `ar`/`nm` inspection found no removed StagePlayer object or symbol in the debug production archive/executable.

The final aggregate rebuild must be repeated after the concurrent exact LodCtrl migration settles; its in-progress compilation temporarily changed shared sources during this audit.
