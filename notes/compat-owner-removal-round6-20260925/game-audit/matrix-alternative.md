# MtxUtil alternative closure audit

Read-only source audit, 2026-09-25. Root HEAD at completion was `eb0712c1488fdc244f7daa0e8da42ea89455d7df`; donor HEAD `1a126cb5da311fedff662f53fb31c5aeaf851408`. No source edits, compilation, tests, staging, or commits were performed. Matrix work is deferred while the lighting closure proceeds.

## Scope and dependencies

The full `decomp/src/Game/Util/MtxUtil.cpp` has 77 MR definitions plus `MtxUtil_FORCE_MATCH`. There is no native `src/Game/Util/MtxUtil.cpp`. Its 50 existing MR definitions are spread across exactly these four providers:

| Provider | Donor definitions | Other definitions |
| --- | ---: | --- |
| `src/compat/MtxCompat.cpp` | 23 | none |
| `src/compat/MatrixTranslationCompat.cpp` | 1 | vector `setMtxTrans(MtxPtr, const TVec3f&)` |
| `src/compat/OriginalMtxGeometry.cpp` | 6 | none |
| `src/compat/OriginalMatrixTransforms.cpp` | 20 | none |

Definition scan found no additional MR signature overlaps. `src/Game/Util/ActorMovementUtil.cpp` retains its three separate LiveActor matrix overloads; these do not collide with MtxUtil. The TVec member named `orthogonalize` is also unrelated.

Required headers already exist: `src/Game/Util/{MtxUtil,MathUtil}.hpp`, `src/JSystem/JMath/{JMATrigonometric,JMath}.hpp`, `src/JSystem/JGeometry/{TMatrix,TVec,TQuat,TUtil}.hpp`, and Aurora `revolution/mtx.h`. Required implementations already exist in `src/Game/Util/MathUtil.cpp` (normalization, plane axes, quaternion turn families, scalar/direction separation), `src/JSystem/JMath/{JMath,JMATrigonometricTable}.cpp`, `src/JSystem/JGeometry/TMatrix.cpp`, and Aurora matrix/quaternion/vector APIs. No new runtime or scene ownership is introduced.

The native Game and compat source globs in `src/Game/xmake.lua` activate a new canonical cpp and remove deleted providers automatically. Native MtxUtil.hpp differs from donor only by include spelling, two omitted NO_INLINE markers, and a parameter name in the commented vector setter body. No declaration repair is required for the full donor cpp. Both headers declare but comment out the vector setter definition; the donor cpp does not supply it. No actual vector-overload caller was found in native sources (all current calls pass xyz). A full removal can either restore that original inline body in the donor header first and mirror it, or explicitly document that the unused declaration remains without a provider. Do not accidentally keep a duplicate scalar setter.

## Semantic differences requiring intentional validation

- `makeMtxRotateY`: current provider uses std::sin/cos on radians; donor quantizes to s16 and uses JMA table values.
- `makeMtxTR` and `makeMtxTRS`: current provider uses host sin/cos; donor uses JMA degree tables and explicit original scaled expressions.
- `makeMtxRotate(s16)`: current provider groups products through named f32 temporaries, donor uses differently associated expressions. Previous retail disassembly in `notes/original-camera-targets-20260903/README.md` identifies separate scalar multiply/add instructions and their order. Do not infer bit equivalence from algebra. The Game donor flags do not explicitly disable contraction; an appropriate native contraction policy needs retail-instruction review, not blindly inheriting the JSystem rule.
- `makeMtxUpSide`: current provider computes Z = Y cross side and X = Z cross Y; donor computes Z = side cross Y and X = Y cross Z. X stays consistent, but Z reverses. For the existing test input, donor Z is +Z, current fixture expects -Z.
- `calcMtxRotAxis`: current provider computes cross(inv(A)*Z, B*Z), falling back to Z; donor computes cross(Y, B*(inv(A)*Y)), falling back to Y. This changes both the selected basis and relative-transform composition.
- `tmpMtxRot{X,Y,Z}Deg`: current providers have the opposite sine signs from donor; donor degree signs now agree with its radian variants. These affect original AreaForm rotation paths, not only diagnostics.
- `isRotAxisY`: current provider uses epsilon 0.001; donor calls TUtil::epsilon(), which is 32*FLT_EPSILON in both donor and native headers.
- `orderRotateMtx`: donor has no default case; current provider treats every out-of-range order as 5. Native callers currently use only literal 0 or 5. Preserve donor's unusual order-4 X/Z/Z sequence; do not silently "fix" it.
- `preScaleMtx`: donor removes the provider's nullptr exception and uses the original matrix-pointer precondition.

## Prior evidence interpretation

Earlier notes call the old providers original and report 99.2–99.6% fuzzy matches. Those percentages are insufficient evidence of semantic identity. The saved `notes/original-mario-math-frontier-20260907/MtxUtil.objdiff.json` shows concrete argument mismatches that support the current donor changes:

- Its left/reference `calcMtxRotAxis` passes the first transformed vector at stack +0x20 to the second PSMTXMultVecSR, then passes the original local vector at +0x2c to the cross product. The right/old candidate instead passes +0x2c into the second multiply and +0x20 into the cross product. This directly explains the current donor's corrected relative composition despite the old 99.51111% score.
- Left/reference `tmpMtxRotXDeg` stores positive sine at offset 0x24 and negative sine at 0x18; right/old candidate swaps those destinations. Current donor follows the reference. The old 99% score concealed an actual rotation-sign difference.
- Left/reference `makeMtxUpSide` also contains cross-product argument-order differences versus the old candidate. Left/right constants for `isRotAxisY` differ by relocation targets; their values were not re-extracted in this bounded audit.

The saved verifier `notes/original-matrix-transforms-20260903/verify-source.py` establishes objdiff's convention here: `-1` is the retail object, `-2` the newly compiled candidate, and summaries use `left` symbols. No new retail comparison or current-donor match percentage is claimed.

## Validation plan

Primary existing targets: `smg-pc-game-math-rotation-tests`, `smg-pc-gravity-math-foundation-tests`, and `smg-pc-original-sensor-matrix-tests`. Secondary integration: `smg-pc-area-obj-core-tests`, original Xanime core/player tests, and a fresh original process Gateway run. `smg-pc-stage-zone-matrix-tests` tests registry ownership and translation bindings; it is not an oracle for the changed trigonometry.

Two fixture expectations in `tests/GameMathRotationTests.cpp` need a retail-evidenced correction with donor activation: line 561's up/side front -Z becomes +Z; line 604's identity rotation-axis fallback Z becomes Y. Keep the quarter-yaw result Y, but explain that donor obtains it as the fallback because yaw preserves local Y. Add nonidentity A and B coverage for relative composition, degree-vs-radian rotation-sign checks, epsilon-boundary comparison cases, and non-grid-angle JMA quantization checks rather than merely updating those two expected vectors. Existing GravityMathFoundation tests cover short-angle delegation and translation-preserving column scaling; OriginalSensorMatrix tests cover directional scale and orthogonalization.

Conclusion: this is an independently coherent complete-owner restoration after reconciling the extra vector setter and validating the known semantic changes. It is larger than a pure relocation, and should not be folded into the lighting migration without its own evidence and focused tests.
