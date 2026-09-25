# Original MtxUtil owner restoration

Baseline `eb0712c1488fdc244f7daa0e8da42ea89455d7df`; donor `1a126cb5da311fedff662f53fb31c5aeaf851408`. Read `game-audit/matrix-alternative.md` and `decomp/AGENT_DECOMP_GUIDE.md` before changes. All seven inspected matrix/fixture paths were initially clean or absent; snapshots and hashes are in `manifest.json` and `before/`. Unrelated dirty tests and source were preserved.

## Changes

- Add full `src/Game/Util/MtxUtil.cpp`: all 77 donor MR definitions, original static temporary matrices, and `MtxUtil_FORCE_MATCH`. Exact donor source except one Aurora include and four native degree-to-short conversions.
- Delete the four replacement providers, 680 lines total: `MtxCompat.cpp`, `OriginalMtxGeometry.cpp`, `OriginalMatrixTransforms.cpp`, and `MatrixTranslationCompat.cpp`. Their 50 donor definitions are now part of the complete actual Game owner.
- Native floating degree conversions call existing `aurora::ppc::truncate_s16`: original fctiwz produces an integer word, then extsh preserves/interprets its low halfword. Direct C++ float-to-s16 conversion was undefined outside the destination range. This is an architecture adaptation; no substitute angle policy was added.
- No header change was required. The donor and native header declare a vector `setMtxTrans` with a commented inline body, while the donor cpp supplies only the scalar overload. No source caller of the vector overload exists. The now-deleted extra compat definition is not retained as a legacy provider; the unused original declaration remains as in the donor.

## Fidelity and FP evidence

The donor corrects up/side cross-product orientation, `calcMtxRotAxis` local-Y and relative-transform composition, degree temporary matrix signs, and `isRotAxisY` tolerance. It restores original table-based yaw/TR/TRS, original null/valid-order preconditions, and the unusual order-4 X/Z/Z sequence without invented fallback behavior. These changes were intentionally retained instead of reproducing old compat expectations.

`retail-fp-instruction-audit.json` records the saved objdiff left/reference instruction stream from `notes/original-mario-math-frontier-20260907/MtxUtil.objdiff.json`. No fused scalar or paired multiply-add/subtract instructions occur in that reference. The rotation/TR/TRS and local-translation calculations explicitly use fmuls plus fadds/fsubs. This supports disabling native FP contraction for this source; it does not establish a new compile match percentage. Existing `notes/original-camera-targets-20260903/README.md` independently documents the signed-angle expression trees and degree fctiwz/extsh path.

`source-validation.json` verifies all 77 MR definitions and exact donor identity after reversing only the documented Aurora conversions. No new decompilation was necessary.

## Focused regression updates

`tests/GameMathRotationTests.cpp` retains prior math/RNG checks and adds:

- Retail-evidenced +Z up/side and local-Y identity-axis expectations.
- Nonidentity roll/pitch relative composition whose -Z result distinguishes the old independent-Z implementation.
- Axis equality inside/outside the original 32-float-epsilon threshold.
- Positive quarter-turn X/Y/Z direction and degree/radian agreement, copying shared temporary matrices before reuse.
- Non-grid-angle table quantization, preserved translation and local-column scale in TR/TRS.
- Full-turn/out-of-range/NaN degree conversion comparisons with explicit original signed-short results.
- A multiply/add cancellation probe that detects accidental FMA in local translation.

## Root wiring and validation

The existing Game/compat globs automatically add the canonical cpp and remove deleted providers. Add source-specific `-ffp-contract=off` for `Util/MtxUtil.cpp` alongside MathUtil; this lane does not edit shared xmake files. No additional source exclusion or dependency is required.

Root should build/run existing `smg-pc-game-math-rotation-tests`, `smg-pc-gravity-math-foundation-tests`, and `smg-pc-original-sensor-matrix-tests`; secondary coverage is AreaObj core and original Xanime core/player, followed by the original-process Gateway smoke. No new test target was added.

Static source validation and targeted whitespace checks pass. Native builds and runtime validation were not run by this lane; source is frozen for root integration. This is not gameplay completion evidence.

## Integrated validation and secondary Area fixture

Root reports full app compile, fresh 600-frame Gateway run, and all three primary matrix fixtures passed. The additional AreaObjCore target initially had two failures: its strict current-donor source comparison and its stale degree-sign expectation. The Area fixture's expected positive X quarter-turn is corrected from -Z to +Z, and positive Z quarter-turn from +X to -X, consistent with the reference instructions already recorded above. Only those two assertions and their text changed.

The source-boundary failure is **preexisting**, not waived: `area-source-boundary-baseline.json` verifies all ten native files compared by that test are unchanged from round6 baseline eb0712c14. The donor AreaForm header now adds a constructor/static method/inline size helpers absent from native; AreaForm.cpp, AreaObj.cpp, AreaObjUtil cpp/hpp, and SleepControllerHolder also have existing differences outside the fixture's allowed pure-virtual/CP932 adaptations. A complete current-donor synchronization needs a separate Area owner task. The assertion remains strict and will continue to report that gap. Initial log and fixture snapshot are saved here; no broad provenance exemption was added.

The XanimePlayer abort was delegated to the SDK agent by root; this lane did not edit or debug it. Area fixture source is frozen for root rebuild.
