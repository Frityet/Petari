# Exact AreaObj stage integration

This evidence records the final AreaObj integration state on 2026-08-08. The
goal is a strict, source-faithful boundary: retail Game sources remain exact,
generalized ownership and platform services live outside `src/Game`, and an
incomplete actor is unavailable instead of simulated.

## Exposed retail placements

The complete descriptor registry is the single source of truth for both the
NameObj factory and scene-owned AreaObj managers. It exposes these four passive
Gateway families:

```text
object                  form       retail manager order   capacity
PullBackCylinder        Cylinder   17                     0x40
ViewGroupCtrlCube       Cube2      32                     0x40
LensFlareArea           Cube2      33                     0x40
BlueStarGuidanceCube    Cube2      40                     0x10
```

The exact `AreaObj::init` consumes the real placement row, applies its JMap
name/type, and registers with the canonical manager. A real-disc focused test
constructs one of these rows and verifies that post-init behavior.

## Honest RestartCube boundary

`RestartCube.{hpp,cpp}` and the stage-session/audio compatibility services are
present as byte-identical/source-generalized foundations, but RestartCube is
not registered as supported. Production still lacks the real Mario actor
attachment and the retail `tryToUpdatePlayerRestartIdInfo` movement caller; the
current audio layer intentionally models logical identity rather than audible
playback. The factory therefore reports
`player_restart_dispatch_and_audible_stage_bgm_runtime_unavailable`.

There is no stage-name exception, synthetic restart result, or fallback actor.

## Real-disc verification

- RMGK01 title probe: 1,994 Korean messages, 3,327 particle resources,
  `TitleLogo.arc` 18 entries, `PressStart.arc` 5 entries, and the exact title
  sequence completed at frame 373.
- RMGK01 HeavensDoor scenario 1 strict construction: exit 1 with 205 blocked
  placement rows; first blocker is `RestartCube` in
  `jmp/placement/common/areaobjinfo`.
- The failed stage emitted no `stage_host_started`, `stage_host_constructed`,
  `StageBgmStart`, or gravity-registration event.
- RMGK02 decomp build remains canonical:
  `sha1sum --check config/RMGK02/build.sha1` reports
  `build/RMGK02/main.dol: OK`.

## Automated verification

- `xmake test -g aurora -j 1`: 35/35 test targets passed, 0 failed.
- AreaObj core: 6/6.
- AreaObj runtime/ownership: 6/6.
- Restart/stage-session: 7/7.
- Aurora native compatibility: 25/25.
- Exact title sequence probe: exit 0 at frame 373.

The generalized Metrowerks functional and logical JAudio compatibility layers
are Aurora commits `7b80ef30bb813f544637fb9350bf99644babc7a6` and
`129e5d73a31d701420ab4d35f746969bbed1496d`, respectively.

As a final source-boundary cleanup, the PC `EventUtil.cpp` is now an exact
retail mirror and excluded from the host build; its three supported picturebook
providers live in `compat/EventUtilCompat.cpp`. Six additional retail
audio/game-data implementation TUs are also retained as byte-identical,
excluded mirrors while the generalized compatibility layer owns their host
symbols.

The resulting PC Game-source inventory is 379 files: 268 byte-identical to the
root decomp, 108 still divergent, and 3 PC-only build/metadata files. Every one
of the 36 mapped Game files in this change is byte-identical to its root file.

See `verification.log` for the concise command/result transcript.
