# Original NW4R BRLAN curve evaluation, 2026-09-10

Aurora's step and Hermite evaluators now preserve the search, strict right-key frame tolerance (`-0.001 < difference < 0.001`), duplicate-key selection, endpoint precedence, and offset/reciprocal arithmetic from the complete original NW4R functions. The original source and saved retail instruction evidence were audited in `notes/original-save-owner-20260910/upstream/README.md`. No Game source, animation scheduling, parsing, or ownership code changed.

The native empty-key return remains at the existing API boundary; original functions assume a nonempty resource. Once a nonempty curve is present, the evaluator follows the original algorithm. The host key-index representation uses size_t for its spans, while original resource counts are u16. Target-local CMake/Xmake options disable floating-point contraction for Aurora NW4R: GNU/Clang/AppleClang use `-ffp-contract=off`; MSVC uses `/fp:strict`. This reflects original NW4R's `-fp_contract off`, without changing other Aurora targets or enabling fast math. Only the LLVM/macOS option was executed here; Windows toolchain behavior was not run.

Changed Aurora paths are `lib/nw4r/brlan.cpp`, `tests/brlan_test.cpp`, `cmake/aurora_nw4r.cmake`, and the existing `aurora-nw4r` target in `xmake.lua`. The existing `brlan_tests` target provides the regression entry point; no new target was needed.

## Verification

- Baseline: all six new public-API regression groups fail against the previous evaluator, with the four existing groups passing. `baseline-results.json`, `baseline-tests.xml`, and `baseline-run.log` retain the exact source hashes and failures. The test source is identical between baseline and fixed runs.
- Fixed LLVM 23 `-O2`, ASan/UBSan build and runtime both exit 0; all 10/10 groups pass. Coverage includes strict tolerance equality and adjacent representable floats, step duplicate search versus Hermite's immediately following duplicate, first/last duplicate precedence, single-key clamps, an original arithmetic rounding golden, and slope interpolation through texture/material APIs. See `fixed-results.json` and `fixed-tests.xml`.
- The actual independent Aurora CMake `brlan_tests` target rebuilt and ran successfully, 10/10. `verified-fp-command.json` captures its actual production brlan.cpp compilation with contraction disabled. `cmake-build.json`/`cmake-run.json` record commands and binary identity. No root Xmake command was run.
- `OriginalCurveComparison.cpp` compares public native pane evaluation against the unchanged source-extracted original helper bodies from the prior upstream audit. It covers 64 deterministic sorted curves of varying sizes, duplicate times, slopes/values, regular frames, and near-key boundaries. All 28,672 scalar comparisons match (including host float bit patterns) with LLVM 23 `-O2` ASan/UBSan; build/runtime exit 0. See `original-comparison.json`.

These are bounded native SDK proofs. They do not claim universal cross-platform floating-point identity, full layout rendering parity, or a new gameplay result. Parent owns subsequent shared builds and publication. No commit or push was made by this subtask.
