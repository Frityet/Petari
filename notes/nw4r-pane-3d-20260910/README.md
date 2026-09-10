# General NW4R three-dimensional panes

The original `GameSceneLayoutHolder` reached its actual `SubMeterLayout("FlyMeter")` constructor and failed because the shared layout host rejected authored X/Y pane rotations. Recovered the remaining common transform behavior from retail NW4R instructions and extended the existing renderer path. No `Game/` source or FlyMeter-specific behavior changed.

## Implementation and reference

- `nw4r::lyt::Pane::CalculateMtx` builds **T × Rz × Ry × Rx × S**, then concatenates the parent's full 3×4 global matrix. Reference: `notes/gateway-audit-20260907/restoration/retail/asm/nw4r/lyt/lyt_pane.s:440–534`.
- The host now retains that full local/global matrix, including Z translation and cross-axis terms. Pictures, windows and text use transformed XYZ vertices. Bounds remain the orthographic XY projection; pane scale uses the full three-dimensional basis-vector lengths.
- Live SDK pane records accept and publish X/Y rotations and Z translation through the same retained animation/property state. Matrix queries and SDK globals copy all twelve values from the renderer's authoritative state.
- Aurora BRLAN `RLPA` channels 2/3/4 now retain translation Z and rotation X/Y. Layer merges, committed frames and per-pane animations retain those channels instead of silently dropping them.
- Original follow mode 2 uses the full inverse local matrix and preserves its Z translation when replacing local XY. Mode 3 transforms the local offset but adds only XY, retaining global Z. Reference: `Game/Screen/LayoutPaneCtrl.s:235–377` in the same retail assembly tree. A 3D pane whose XY projection is singular still has a valid invertible transform.
- The layout orthographic projection uses the original near/far values **−1000/+1000**, mapping `z_clip = −z / 2000 − 0.5`. Reference: retail `Game/Util/DrawUtil.s:232–235` and constant definitions at `1803–1809`; `Game/Screen/LayoutManager.s` calls `setupDrawForNW4RLayout(1, true)` and uses an identity DrawInfo matrix. The incomplete decompilation's ±10000 values were not used. Other render spaces retain their existing projection.

## Verification

- All four changed C++ units compile successfully in isolated invocations of the current root build commands: `native-compile.json` and corresponding compile logs.
- Extended Aurora BRLAN GoogleTests run **4/4 passing**, including the new translation Z / rotation X/Y step and Hermite values. `brlan-results.json` records the build command; `brlan-run.log` records the actual GoogleTest result. The isolated test links the local Dolphin GoogleTest sources and actual Aurora parser/evaluator.
- The root `smg-pc-original-layout-group-tests` target builds successfully. Its real-disc run exits **0** in about **1.61 seconds**, with **32** repeated synthetic ownership cycles. Tests verify analytical parent-Y/child-X rotation composition, child Z translation, complete SDK/render matrix agreement, and both local follow cases.
- The same run constructs the actual original `SubMeterLayout` with the real disc's FlyMeter archive, calls its original initialization and life-ratio methods, verifies authored 3D panes and finite SDK matrices, and completes three draw/end-frame cycles on Metal at ratios 1, 0.5 and 0.125. Teardown completes successfully. This is runtime construction/draw evidence, not a pixel-comparison claim. See `layout-run.json` for the exact binary hash and `layout-run.log` for the result.
- The parent demo's `notes/demo-camera-jump-20260910/layout-3d-run.log` advances past FlyMeter and the remaining HUD construction into Mario parts. Its subsequent failure was a separate native texture-offset relocation issue, documented by the parent task.
- `git diff --check` passes for the exact changed root paths.

## Publication

Aurora is committed and pushed as codex in **`f14bed4e8ef79b3d41541ea2e339e7d203122426`**, with the exact remote SHA verified. `aurora-publication.json` records the three paths. `source-manifest.json` identifies the five root source/test paths and the three Aurora paths by SHA-256. The parent task owns the root checkpoint and updated gitlink.

Objects, executables and the full compiler output are local supporting artifacts; the source, compact JSON records and relevant logs are sufficient to review this checkpoint. This change does not establish playable Mario jumping or camera success by itself.
