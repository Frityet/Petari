# Original Mario base matrix recovery — 2026-09-10

Recovered `MarioActor::calcAndSetBaseMtx` in the decompilation reference, then copied exactly that function to the native port. Every byte outside this function remains unchanged in each source, preserving the port's existing architecture fixes. Decomp commit `5ad8d92829d200146869744b29056146b60f04b3` was authored/committed as codex, pushed, and verified against `origin/pcp-decomp`.

The live shadow failure was downstream of invalid Mario base/joint matrices. Retail `0x802B3800–0x802B4880` uses one main matrix throughout; the unfinished source had split it into several uninitialized matrix objects/pointers and left the binding result uninitialized. The repair restores the complete retail function, including forced poses, Bee/Teresa transformations, correction matrices, press/recovery, blending, and final state updates. No finite-value guard, shadow bypass, or mode-specific shortcut was added.

## Validation

- Fresh published reference TU compiled with Metrowerks: exit 0. Fresh published native TU compiled with LLVM 23: exit 0. Exact commands/source hashes: `published-compile-results.json`.
- Fresh original-object comparison: **94.456955%**, 4208 bytes against retail 4228 bytes. Previous implementation: 58.933773%, 4000 bytes. Exact result and retail object SHA: `result.json`; detailed comparison: `published-objdiff.json`.
- Uninitialized matrix-pointer warnings present in the baseline have disappeared. Remaining warnings concern pre-existing header union members and unrelated source expressions.
- The final function is byte-identical between reference and port (`source-manifest.json`). Only this function was modified.
- Independent retail review by the initialization agent covered normal/forced/press/recovery and Bee branches. See `../mario-teresa-base-matrix-20260910/retail-press-audit.md`; that agent separately recovered the original `updateBaseMtxTeresa` helper.
- The parent's earlier complete draft run built successfully and passed initial movement/animation without the nonfinite shadow failure, then reached the loaded Gateway controls log. That run still aborted before first presentation (exit -6); it is not proof of a playable demo. See `../demo-camera-jump-20260910/restored-pose-run.json` and its adjacent log. Final demo testing remains with the parent.

## Retail details restored

| Retail location | Recovered behavior |
| --- | --- |
| `0x802B386C–0x802B3914` | Initialize binding result false and retain `HitSensor::receiveMessage` result. |
| `0x802B3914–0x802B3AE4` | Only `_EA5` installs `_EA8`; `_EA6` early return and displacement from prior position follow retail. Preserve the forced matrix snapshot. |
| `0x802B3AE4–0x802B3BF4` | Build one posture matrix and append Y rotation; call recovered Teresa helper in mode 6. |
| `0x802B3BF4–0x802B42E4` | Restore Bee predicates (Stick `0x16`, SideStep `0x15`, Bury `0x1B`), flag conditions, spring integration, axis orientation, angular limits, and translation blend. Preserve retail table offsets even where existing names seem counterintuitive. |
| `0x802B42E4–0x802B438C` | Add position to the main matrix; forced correction is inverse(main) × forced base, normal correction uses actual local matrix storage. |
| `0x802B438C–0x802B476C` | Restore press cases and countdown recovery, both ceiling query calls in retail order, lateral decrement **0.01**, timer boost **0.02**, cosine curve, and movement bit 3 guard. |
| `0x802B476C–0x802B4880` | Blend from the old model matrix with division before multiplication; copy the same working matrix to `_3EC` and the model, and clear `_EA5`. |

The remaining object differences are frontend inlining/register decisions (notably the initial 12-float matrix copy), equivalent short-circuit lowering, vector/table expression evaluation, and constant-pool relocation identities. The review also corrected the prior audit's mistaken reading of `@79802`: retail `.sdata2+0x74` is `0x3c23d70a` (**0.01f**), used by both Bee direction comparisons and lateral press recovery.

`retail.asm` and `retail-short.asm` preserve the source instruction evidence. `decomp-before.cpp` and `port-before.cpp` preserve the initial source snapshots. Intermediate drafts and object dumps are local evidence; publication can keep this README, `result.json`, `published-compile-results.json`, and `source-manifest.json` as the compact proof set.
