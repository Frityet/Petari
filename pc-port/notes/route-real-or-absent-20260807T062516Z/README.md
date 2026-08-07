# Real-or-absent route checkpoint

Captured 2026-08-07 with the Aurora-native PC executable, `RMGK01.iso`, and
Xvfb display `:500`.

Command:

```text
xmake aurora-route-smoke --disc=../RMGK01.iso --display=500 --no-build \
  --work-dir=notes/route-real-or-absent-20260807T062516Z title
```

The capture intentionally fails the visual threshold at frame 90. The image is
black (`nonblack_ratio=0`) because the real retail closure is incomplete:

- the sequence requests the retail `FileSelect` stage;
- `FileSelector` is blocked with
  `retail_file_select_actor_runtime_unavailable`;
- the stage player is absent with `real_mario_actor_not_linked`;
- the start camera is absent because `CAM_TYPE_FOLLOW` is unsupported;
- title activation is absent because Mario's auto-rush binder event is not
  installed.

This is the expected strict boundary after removing the invented FileSelector
geometry, synthetic save data, forced transitions, and fake player/camera
state. The log contains no `CFG1`, `SYS1`, or `EVNM` serializer use. Nothing
substitutes a fake title, six-slot file selector, or picturebook transition.

Artifacts:

- `title/title-app.log`: complete semantic/runtime evidence;
- `title/title-frame-90.png`: the honest black frame;
- `title/title-frame-90.trace.sqlite`: packet and semantic trace;
- `manifest.json`: failed visual expectation and threshold.

The desired retail route remains title -> six-slot FileSelect -> five-page
picturebook -> HeavensDoor. The next implementation work must recover the real
FileSelector/MiiSelect/Mario/camera closure instead of restoring any of the
removed substitutes.
