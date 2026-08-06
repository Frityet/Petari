# Source-close SimpleEffectObj / Steam PC port

Updated: 2026-08-06T19:46:06Z

## Outcome

The PC placement factory now constructs the original `SimpleEffectObj` for all eight HeavensDoor `Steam` placements. The HeavensDoor scenario 1 frontier changed from `154 created / 16 blocked` to `162 created / 8 blocked`, with no stage, zone, placement-row, switch-ID, or route-specific object behavior.

`pc-port/src/Game/Effect/SimpleEffectObj.cpp` is byte-identical to `src/Game/Effect/SimpleEffectObj.cpp`. The PC header differs from the root header in one deliberate host-lifetime adaptation: RMGK02 returns an effective zero clipping offset through expired stack storage, while the host returns the same zero vector from persistent static storage. The root source remains unchanged for binary fidelity.

The root RMGK02 audit measured `SimpleEffectObj` at 1688/1688 code bytes, 17/17 functions, and an exact 288-byte effect-data table. Its `Steam` row is:

```text
{"Steam", nullptr, "SE_OJ_LV_HD_STEAM", -1, nullptr, 0}
```

Accordingly, each Steam actor emits the `Steam` effect once when entering Move and submits the looping `SE_OJ_LV_HD_STEAM` sound every Move frame.

## General compatibility boundary

The original translation unit is supported by generalized host providers outside the actor:

- safe `MR::isEqualString`;
- actor effect deletion and existing effect emission;
- actor one-shot and level-sound submission;
- weak/normal/strong camera-shake kinds;
- named rumble-pattern dispatch;
- demo registration, shared-group, and clipping-policy seams.

The last group currently has honest general no-op/null behavior because those host systems do not yet exist. The host scheduler currently keeps registered actors active, so the absent clipping implementation does not suppress Steam behavior. No provider tests for `Steam`, HeavensDoor, or a placement coordinate.

## Automated verification

```text
xmake build smg-pc
[100%]: build ok

xmake test
[ok] SimpleEffectObj host compatibility
12 Aurora-native test(s) passed
100% tests passed
```

The new native test proves the clipping offset is persistent and zero, string equality is null-safe, and weak/strong shake requests remain distinct with their frame identity.

## Real-disc runtime evidence

Raw transient artifacts: `/tmp/smgpc-steam.x8EJA2/`

The smoke starts normally in FileSelect and holds F10 long enough to exercise the existing debug stage-request input. Rendering was skipped during this placement/behavior pass so all 180 logical frames could run quickly; the separately preserved full-sequence route regression still proves the rendered title, file-select, and picturebook checkpoints.

```sh
SMGPC_FRAME_PACING=1 \
SMGPC_SKIP_RENDER_UNTIL_FRAME=10000 \
SMGPC_EXIT_AFTER_FRAME=180 \
SMGPC_STAGE_PLACEMENT_REPORT_PATH=/tmp/smgpc-steam.x8EJA2/placement.md \
SMGPC_DISC_IMAGE=/workspaces/pcport/RMGK01.wbfs \
./build/linux/x86_64/debug/smg-pc
```

The X11 wrapper focused the `Super Mario Galaxy` window, held F10 for 0.1 seconds, released it, and received a zero application result.

```text
FileSelect scheduler cleanup: 78 -> 2 (76 removed)
HeavensDoor placement: objects=242;created=162;ignored=72;blocked=8
Steam original-factory placements: 8
Steam first-step effect emissions: 8
Steam looping level-sound submissions: 1024
fatal/segmentation/abort/crash matches: 0
```

Durable compact evidence is under `artifacts/`; the full raw log remains in `/tmp` because per-frame level-sound logging is intentionally verbose.

## Remaining blocker frontier

| Count | Object |
| ---: | --- |
| 3 | `DemoRabbit` |
| 2 | `RailCoin` |
| 1 | `YellowChipGroup` |
| 1 | `StarPieceGroup` |
| 1 | `FlipPanelObserver` |

The next low-risk generalized placement slice is the normal rail/coin substrate followed by `RailCoin`. YellowChip is progression-critical because group completion reveals the main SuperSpinDriver, and its root source has now been reconstructed, but its PC dependency surface is larger.
