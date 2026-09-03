# Original shared blend and translation helpers

The core matrix path needs the general five-argument `MR::PSvecBlend`, its
`MR::vecBlend` wrapper, and both `MR::setMtxTrans` overloads. These providers do
not contain actor, animation-name, or stage conditions.

## Root and native changes

- `src/Game/Util/MathUtil.cpp::PSvecBlend` retains its original Metrowerks
  assembly branch exactly. A non-Metrowerks branch implements its arithmetic
  using typed float locals and the float overload of `std::fma`; `<cmath>` is
  included only for that branch.
- `pc-port/src/compat/GameMathCompat.cpp` receives that exact full function and
  the unchanged root `vecBlend` body. Existing normalized/spherical blends are
  not altered by this tranche.
- Root `MtxUtil.hpp` previously contained only a declaration and commented body
  for the vector translation setter. `src/Game/Util/MtxUtil.cpp` now contains
  that wrapper, recovered from retail: load XYZ, call the scalar overload.
- New `pc-port/src/compat/MatrixTranslationCompat.cpp` copies both actual root
  translation-setter bodies. There are no new public declarations or signatures.

The first and second float parameters are independent weights; they are not
required to sum to one. `vecBlend` computes `1.0f - rate` with single-precision
subtraction, then delegates. Neither helper clamps a weight or rate.

## Retail arithmetic and aliasing

Verified RMGK01 `PSvecBlend` at `0x803E75C8`, size `0x2C`, does the following for
each component:

```text
scaled = float32(from * weightFrom)
result = fused_float32(to * weightTo + scaled)
```

The first weight arrives in FPR1 and the second in FPR2. The source assembly
names its fourth argument in the two multiply instructions but uses literal
FPR2 in the fused operations; that is the fifth C++ argument, not another copy
of the first weight. The later load into FPR1 does not change this behavior.

All six source components are loaded before either retail store. The native
branch likewise snapshots all six components before storing any output. Thus
the arithmetic is preserved when the destination aliases the first input, the
second input, or both. No memcpy/layout reinterpretation is required.

The translation-vector wrapper at `0x801B5BF0` loads XYZ before tail-calling
`0x803ECFB0`. The scalar setter stores only matrix offsets `0x0C`, `0x1C`, and
`0x2C`: the translation column. It does not modify, normalize, or scale the nine
basis elements. The vector wrapper's argument evaluation preserves the original
read-before-write behavior.

## Verification

```sh
python3 pc-port/notes/xanime-core-matrix-calculation-20260903/verify-helpers.py
```

The verifier checks the four native bodies against root, compiles exact
individual root function extracts with GC 3.0a3 and the configured Game flags,
and compares all instructions with the verified retail DOL. PSvecBlend and the
scalar translation setter are byte-identical. The vecBlend and vector setter
also match all instructions after accounting for their constant/tail-call
relocations; the retail literal value and branch targets are checked explicitly.

An independent paired-single decoder builds each output's arithmetic graph from
the retail words. A separate parser builds the graph from native float loads,
products, and `std::fma` expressions. All three outputs and the complete
read-before-store alias boundary agree. `helpers-evidence.json` records those
graphs and checks. Full compiler artifacts remain under
`build/xanime-core-math-helpers-20260903/`; the retail dumps are under
`build/xanime-core-model-data-20260903/`.

Two finite witnesses help distinguish the required arithmetic in native tests.
The input columns are IEEE float bits:

| from | to | weightFrom | weightTo | Required output | What it checks |
| --- | --- | --- | --- | --- | --- |
| `3f800001` | `bf800000` | `3f800001` | `3f800002` | `00000000` | First product must round before FMA; keeping it in double gives `2^-46` |
| `3f800000` | `3f800001` | `bf800000` | `3f7ffffe` | `a8800000` | Second product/add must fuse; separate float multiply/add gives zero |

The verifier derives those witnesses with host `fmaf` as arithmetic examples;
it does not execute the native providers or claim a native runtime test. Parent
owns native builds and tests, including distinct/aliased output vectors and
translation-column isolation.
