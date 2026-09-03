# Original J3D matrix-buffer lifecycle

This tranche imports the complete existing `J3DMtxBuffer` implementation into `compat/J3DMtxBufferCompat.cpp`, retaining the real `J3DJointTree`, animation matrices, envelope matrices, view arrays and double-buffer accessors. The native header remains byte-identical to the root header. `initialize` moved from `J3DJointTreeCompat.cpp` to its complete provider. The original model's `calc` and `viewCalc` can now call actual envelope, draw, normal and billboard methods.

This is a typed J3D foundation. It does not turn the renderer's decoded BMD summaries into complete material/shape/model objects, and does not activate the original Mario animation owner. Tests construct intentionally joint-only model data with real objects and explicit array ownership.

The buffer and model destructors are empty in retail. Native owners must retain and release their allocations explicitly. The fixture frees alignment-placement matrix arrays through the matching `operator delete[](pointer, 0x20)` and ordinary pointer tables/animation arrays through ordinary `delete[]`. Shared no-use arrays are never freed. Positive material bump allocation is imported and compiler-verified but is not exercised by the joint-only fixture.

## Source and retail evidence

All addresses refer to RMGK01 Rev.0, verified DOL SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`. Run from the repository root:

```sh
python3 pc-port/notes/original-j3d-mtx-buffer-20260903/verify-source.py
python3 pc-port/notes/original-j3d-mtx-buffer-20260903/verify-math.py
```

`verify-source.py` compiles the root translation units with the actual GC 3.0a3 compiler, `wibo`, `sjiswrap` and `cflags_jsys` from `configure.py`. It compares the objects to the previously extracted retail objects, checks nineteen root/native function bodies, the unchanged native header, and both default constants. Exact commands, object files and full objdiff output remain under `build/original-j3d-mtx-buffer-20260903/`; compact results are in `source-evidence.json`.

All eleven allocation/view/normal-related functions other than envelope and the billboard dispatch match 100%. `calcBBoardMtx` is 99.87654%, and the original Y-billboard body is 99.85507%, with equal code sizes. The existing root envelope inline-assembly body compiles to 508 bytes versus retail's 556 bytes and has a 67.85612% instruction match. This is explicitly not claimed to be an instruction match: its arithmetic and resets are checked independently against the actual retail paired instructions, as described below.

The regular billboard decompilation had used a bare reciprocal-square-root estimate. Retail `J3DCalcBBoardMtx` at `0x804239D0` calls full `sqrt` and rounds to float. Root now uses that original operation and observed scalar store order; the corrected function matches 100%, including its 296-byte size. The Y-billboard really uses the different approximate `JMAFastSqrt` operation. The latter's native shared implementation now follows retail `frsqrte` then multiplication, returning nonpositive inputs unchanged. For example, the result for `1.0f` is `0x3F7FF400`, not exactly one. Positive infinity becomes NaN; negative zero is preserved.

Root-first portable architecture branches supply inverse transpose, normal scaling, matrix-array concatenation and envelope accumulation. The original PowerPC branches remain available. Native extracts preserve the root bodies; contraction is disabled except for explicit `std::fma` operations corresponding to paired fused instructions. `J3DPSMtx33CopyFrom34` was restored root-first in the coordinator-owned ShapeMtx source and extracted here. `j3dDefaultScale` at `0x8055C1D8` and `j3dDefaultMtx` at `0x8055C1E4` are byte-verified retail constants.

The native import retains original resource preconditions. Each envelope must have a positive mix count and valid joint indices; `J3DPSMtxArrayConcat` requires a positive count. It does not manufacture zero-count behavior. Draw mode 1 deliberately retains the retail second loop's full-weight count reload at `0x80432550`, rather than changing that loop to use envelope count.

## Arithmetic verification

`verify-math.py` executes actual DOL instructions with independently decoded paired arithmetic, estimate tables and memory stores, comparing against an isolated library of the production native providers. It makes 27,557 passing native calls:

| Provider | Cases | Coverage |
| --- | ---: | --- |
| Inverse transpose | 4,509 | Singular matrices, finite random matrices, in-place and shifted overlap |
| Matrix-array concatenation | 3,000 | Two-matrix sequences and right-input/output alias |
| 3x3 copy from 3x4 | 4,500 | Separate storage, in-place, shifted overlap |
| Normal scaling, 3x4 / 3x3 | 1,500 each | Independent finite random scales and matrices |
| Shared fast square root | 7,048 | Every float exponent class, both signs, subnormals, infinities, NaNs and random bit patterns |
| Projection concatenation | 4,500 | Coordinator's actual native helper, separate/left/right alias, retaining row-pair store order |
| Weighted envelope calculation | 1,000 | Actual native method on real tree/buffer objects; multiple envelopes, repeated joints, signed weights, per-envelope resets and scale-flag AND |

All finite output bits, including signed zero, agree. Exceptional values are compared by NaN classification and infinity sign; NaN payload/sign and FPSCR flags are not asserted. Matrix random inputs are finite moderate values. The envelope oracle executes the actual retail arithmetic block at `0x804322E4` through `0x80432394`, its entry accumulator resets, every output store, and the final resets, rather than reproducing the native scalar expression. The unchanged integer metadata traversal is covered by the native fixture and direct source inspection.

Ordinary buffer calls to SDK `PSMTXConcat` and Y-billboard `PSVECNormalize` retain the pre-existing Aurora providers. The raw oracle verifies the newly supplied paired helpers; it does not claim a new exhaustive proof of every existing SDK routine.

## Native regression target

`smg-pc-original-j3d-mtx-buffer-tests` has six groups: actual allocation/alignment and per-view bank swaps; original no-animation/concat-view allocation; weighted envelopes and all three draw modes; scale-sensitive normal matrices; both billboard modes; and a real `J3DModel` constructor → virtual `calc` → `viewCalc` chain using two real joints and an original Basic calculator. Shared fast-square-root edge cases are also checked. The actual-model group verifies separate base scale/translation, hierarchy traversal, bank swaps, view transforms and normals, and restores J3D global state after the fixture.

The source and test compile in isolated native probes. The coordinator owns the shared xmake build and execution results; those results should be recorded with the final checkpoint rather than inferred from compilation.
