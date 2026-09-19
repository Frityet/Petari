# Earlier runtime checkpoint before final build-ownership cleanup — 2026-09-19

The rebuilt normal entry completed a neutral 1,200-frame Gateway opening with the authored opening camera/character sequence visible. Ordinary no-stage startup completed 180 frames but its FileSelect content is visibly incomplete. Strict Gateway placement correctly rejected a known-unlinked actor. These are bounded startup/opening checks; they do not establish rabbit captures, Rosalina appearance, complete FileSelect functionality, or visual parity.

## Results

| Run | Outcome | Wall elapsed | Placement queue inventory |
| --- | --- | --- | --- |
| `ordinary-startup` | 180 completed frames, exit 0, PID 5547 reaped | 4.1925 s | FileSelect: 3 supported, 2 known-unlinked, 0 unknown |
| `gateway-opening` | 1,200 completed frames, exit 0, PID 5623 reaped | 24.9120 s | HeavensDoorGalaxy scenario 1: 175 supported, 63 known-unlinked, 0 unknown, 5 metadata |
| `gateway-strict` | Expected explicit rejection, exit -6, PID 6246 reaped; no completed-frame marker | 1.0214 s | Same real Gateway queues and availability counts |

Both successful runs emitted the exact requested completed-frame marker. All three processes disappeared after wait, used fresh save directories, stayed within timeout, and retained the same executable hash. The placement reports inspect the actual original placement queues and classify creator availability; their supported count is not a separate count of constructed or exercised actor instances.

The strict error was:

```text
Aurora OS guest thread failed: Original stage contains an unlinked retail actor: RestartCube (zone=-1;table=areaobjinfo;row=0;reason=real_mario_update_and_restart_dispatch_runtime_unavailable)
```

The coverage report and strict check occur in `SceneFunction::startActorPlacement` immediately before `SceneDataInitializer::startActorPlacement` (`src/compat/SceneInitializationCompat.cpp`). The error is an explicit missing implementation, not a successful strict gameplay run. No unsupported actor was fabricated to satisfy this check.

## Visual inspection and limits

Both original PNG captures were opened with the image viewer.

- `gateway-opening-frame1000.png` shows Mario on the flower-covered starting surface, facing the glowing opening character/transformation effect with sparkles and authored letterboxing. The opening sequence visibly progressed; this neutral run stops before the gameplay objective is completed.
- `ordinary-startup-frame179.png` shows Mario and the ordinary HUD against a solid blue background. The actual selected scene is FileSelect, whose report lists `SphereSelectorHandle` and `FileSelector` as known-unlinked. This confirms the normal application boundary runs, but it does **not** confirm a working file-selection screen.

Gateway's final 300-frame window measured 57.745 wall FPS (5,195.224 ms). Its whole frame loop averaged 20.481 ms/frame. The 180-frame no-stage run averaged 20.513 ms/frame. Timing includes polling, retrace/device waits and the single requested capture; this is not a universal gameplay performance guarantee.

## Reproduction and evidence

`run_smoke.py` launches the signed native app executable directly, with no obsolete `--original` flag. Each label owns a fresh save directory. Inherited `SMGPC_*` settings are removed so the runs cannot accidentally reuse a prior input script, live input file or scene setting. No debugger, controller script or manual controller input was used. Runs were serialized with all other build/GPU work.

```sh
python3 notes/compat-original-runtime-20260919/run_smoke.py ordinary-startup --frames 180 --screenshot-frame 179 --timeout 120
python3 notes/compat-original-runtime-20260919/run_smoke.py gateway-opening --frames 1200 --stage HeavensDoorGalaxy --scenario 1 --screenshot-frame 1000 --timeout 180
python3 notes/compat-original-runtime-20260919/run_smoke.py gateway-strict --frames 1200 --stage HeavensDoorGalaxy --scenario 1 --expect-strict-failure --timeout 120
```

Use new labels when rerunning; the runner refuses to overwrite existing evidence/save paths. Completion requires exit zero, the exact completed-frame marker, process disappearance, and unchanged executable hash. The strict case requires nonzero exit, the explicit unlinked-retail-actor error, no completion marker, and a real placement report containing known-unlinked rows. Timeouts remain recorded failures. `.log.gz` files preserve the exact raw logs with deterministic gzip metadata.

Final rebuilt direct executable: SHA256 `6fd4824a3217cb18bb7f6533936202a4854c6475a7fbef895176a2ba91ffbb43`.
The launched signed app executable: SHA256 `7e9c0bcb9351f43e9eaf87f3eebc5ef2c34f60d6da6da4704d7d75ebfb1b6aa0`.
Both have LC_UUID `3F7A8E7F-BDCC-375E-AA17-79FE38E29E34` and link/bundle timestamp 18:07:23 local. The bundle's different code-signature size explains the different final hash; each run records the actual launched bundle hash before and after execution.

The renderer checkpoint includes Aurora destination-alpha commit `8c19ab45`. The native cleanup removes the alternate effects/lifecycle application route and redundant compatibility providers. Builds are owned by the coordinating agent; this directory contains runtime evidence only.

Curated publication: this README, `run_smoke.py`, the three final `.json` records, three `-launch.json` records, three `-placements.json` reports, three `.log.gz` logs, and two original frame PNGs. Exclude raw duplicate `.log` files, save directories, Python bytecode and the disc image.
