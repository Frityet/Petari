# Scene Registration Scope And Teardown

Timestamp: 2026-08-06T19:07:38Z

## Problem

`StageHostScene` owns only its top-level roots. Original game objects also allocate and connect child `NameObj`, `LiveActor`, and layout objects that are not represented in `_roots`. Destroying a stage therefore left those child registrations in the process-wide `SceneScheduler`, so a later stage could update or draw objects from the replaced scene.

The first recovered version also ended the registration scope after `_roots.clear()`. Scheduler entries contain raw object pointers and `remove_registrations_since()` snapshots their names and dynamic types, so that order could dereference an already-destroyed root during teardown.

## Implementation

- `SceneScheduler` now exposes a monotonic registration marker and can remove/snapshot registrations created at or after that marker.
- The returned snapshot identifies `LiveActor` instances even when they were connected only as movement `NameObj` entries. This lets generic teardown remove their star-pointer targets too.
- `RuntimeContext` owns one active stage-registration scope. Ending it:
  - removes stage-local scheduler entries while preserving registrations that predate the scene;
  - unregisters affected star-pointer targets;
  - erases stage-local live-actor/layout effect-host pointers;
  - deletes effect instances emitted during the scene;
  - unregisters only effect keepers registered during the scene; and
  - refreshes/unbinds affected effect transforms after their host maps have been cleaned.
- Effect emissions and keeper registrations are tracked separately. Emitting an effect no longer makes teardown unregister a pre-existing keeper for that host.
- `StageHostScene` begins the scope in its constructor and ends it before destroying roots. All scheduler pointer inspection consequently happens while the registered objects are still alive.

This is a general scene-lifetime boundary; it does not special-case FileSelect, HeavensDoor, or any actor name.

## Automated Verification

Focused build:

```text
xmake build smg-pc-aurora-native-tests
[100%]: build ok, spent 2.247s
```

The new native test registers a persistent `NameObj`, records a marker, reconnects that persistent entry, and registers a movement-only `LiveActor`. It verifies that scope cleanup removes and classifies only the new actor while the pre-marker object survives.

```text
xmake test
[ok] scene scheduler registration scope cleanup
9 Aurora-native test(s) passed
100% tests passed, 0 test(s) failed out of 1
```

Application build:

```text
xmake build smg-pc
[100%]: build ok, spent 0.159s
```

`git diff --check` passed for all seven files in this slice.

## Runtime Transition Proof

A forced-X11 run waited until FileSelect had applied, held the existing F10 debug input, then continued through HeavensDoor placement. Rendering was skipped so the scene-lifetime path could be exercised quickly; frame pacing remained enabled so key-down and key-up arrived in separate polls.

```bash
SMGPC_EXIT_AFTER_FRAME=240 \
SMGPC_SKIP_RENDER_UNTIL_FRAME=9999 \
SMGPC_FRAME_PACING=1 \
SMGPC_ENABLE_VSYNC=0 \
SDL_VIDEO_DRIVER=x11 \
SDL_VIDEODRIVER=x11 \
SMGPC_DISC_IMAGE=/workspaces/pcport/RMGK01.wbfs \
timeout 30 xvfb-run -a -s '-screen 0 640x480x24' \
  build/linux/x86_64/debug/smg-pc --disc /workspaces/pcport/RMGK01.wbfs
```

The input driver waited for `scene_controller:scene_change_applied (... current_stage=FileSelect ...)`, then used `xdotool keydown F10`, held for 0.3 seconds, and released it.

Observed anchors:

```text
run_status=0
placement:stage_placement_summary (stage=FileSelect;scenario=1;objects=4;created=2;ignored=1;blocked=1)
scene_lifecycle:stage_destroy_scheduler_cleanup (scene=Game;stage=FileSelect;scenario=1;before_entries=77;after_entries=2;removed_entries=75)
placement:stage_placement_summary (stage=HeavensDoorGalaxy;scenario=1;objects=242;created=146;ignored=72;blocked=24)
scene_controller:scene_change_applied (current_scene=Game;current_stage=HeavensDoorGalaxy;scenario=1)
fatal/error/segmentation/abort matches: 0
```

The `77 -> 2` scheduler transition is the direct runtime evidence for this slice: 75 FileSelect-scope registrations were removed, while the two process-level registrations that existed before the scene were preserved. The HeavensDoor scene then constructed and applied normally, and the process reached its configured frame exit with status 0.

## Remaining Route Gap

This fixes stale cross-scene execution; it does not make the Gateway route playable by itself. The same run still reports 24 blocked HeavensDoor placements, and original player/rabbit/demo behavior remains separate work.
