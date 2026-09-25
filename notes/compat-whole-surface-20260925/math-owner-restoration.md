# Canonical MathUtil restoration

Restored complete `decomp/src/Game/Util/MathUtil.cpp` from `1a126cb5da311fedff662f53fb31c5aeaf851408` into the actual `src/Game/Util/MathUtil.cpp` owner, retaining all 141 donor function definitions. There is no major undecompiled gap in this donor; its PPC assembly-only paths required the existing native translations. Added only the three existing native declarations/definitions: `MR::getRandom(long,long)`, `MR::frsqrte`, and `MR::fastSqrtf`.

Deleted four redundant providers:

- `src/compat/GameMathCompat.cpp`
- `src/compat/OriginalJMathSqrt.cpp`
- `src/compat/OriginalVectorOrientation.cpp`
- `src/compat/OriginalPointerVectorQueries.cpp`

Removed seven exact overlapping methods from mixed providers, preserving all unrelated definitions: four MathUtil methods in `OriginalMtxGeometry.cpp`, both `MR::lerp` overloads in `OriginalImageEffectUtil.cpp`, and `MR::checkHitSegmentSphere` in `OriginalCollisionGeometry.cpp`. `calcVelocityMovingPoint` remains in that last file for the MapUtil owner batch. The blend helpers in LightFunctionCompat are anonymous-namespace local functions, not duplicate MR definitions; they were left untouched.

The unsigned `sortSmall` tail was removed from HashSortTableCompat and restored through the complete MathUtil donor. Root was notified immediately and subsequently owns the rest of that hash migration. MathUtil.hpp now declares the LP64 random overload under TARGET_PC, so removal of forced MetrowerksStdCompat inclusion does not hide that declaration.

## Native behavior preserved at its owning method

- `PSVECKillElement`, `vecScaleAdd`, and `PSvecBlend`: preserve existing explicit fused operations, component load-before-store alias safety, and separate fused rounding/negation. Original MW bodies remain under the original architecture guard.
- `JMASqrt`: preserve Gekko reciprocal-square-root estimate and double/single refinement order, with local Clang contraction/reassociation disabled and signed-zero/nonpositive behavior. Reusable PPC estimate remains Aurora/Dolphin SDK support.
- Fixed16 conversions: preserve Aurora PPC shift and saturating conversion helpers, avoiding host undefined shifts/conversions.
- `setNan` and `getRandom`: use bit_cast on native hosts to preserve exact bit patterns without integer/float aliasing.
- `isNan`: use native classification, rather than assuming MSL `__fpclassifyf` numeric constants.
- Vector maximum-component access avoids indexing a scalar subobject as an array while preserving original index/tie/NaN decisions.
- `acosEx` retains the native nonfinite/out-of-domain conversion guard; valid-domain table and interpolation behavior are donor code.
- LP64 random range rejects values that cannot represent a Wii s32 range, as before.

## Removed alternate behavior

`MR::getRandom` now advances only `SingletonHolder<GameSystem>::get()->mObjHolder->mRandom`, exactly as donor. The hidden independent static seed is gone. This also reconnects existing `MR::setRandomSeed`/GameSystem state to every random math call. Tests that previously worked without any real GameSystem cannot assume a fallback RNG; their fixture must own the actual original system/holder.

The original explicit arccos-table initialization is restored. `OriginalGameApplication` already calls `MR::initAcosTable` before creating GameSystem. Standalone fixtures that need `acosEx` must initialize this owner (GameMathRotationTests already does). The previous lazy alternate table is removed.

Ordinary donor algorithms replace nullable shortcuts and rewritten arithmetic from the old compat provider. For example, rebound arithmetic now follows the original multiplication/subtraction order rather than a host vector rewrite; no contraction must be applied implicitly. Original `lerp` green/end-blue behavior and other donor quirks remain unchanged.

## Root integration and validation

Build wiring required (root informed): remove `remove_files("Util/MathUtil.cpp")`; assign `-ffp-contract=off` to `Util/MathUtil.cpp`; remove deleted explicit `../compat/GameMathCompat.cpp` and `../compat/OriginalJMathSqrt.cpp` entries. No xmake files were modified by this agent.

Static checks completed: all 141 donor function signatures retained; precisely three native extras; no remaining duplicate signatures against surviving compat providers; git diff --check passed. Recorded in `math-owner-symbol-coverage.json`. No compiler/build/test was run by this agent, per task scope. Root owns integrated validation.

Recommended existing focused coverage: GameMathRotationTests, GravityMathFoundationTests, original geometry/collision tests, and animation/hash lifecycle tests. Check PS aliasing and signed zero, fixed16 boundary conversions, JMASqrt estimate results, original RNG seed progression, and sortSmall unsigned ordering. Compilation or these tests alone do not establish complete Gateway gameplay.
