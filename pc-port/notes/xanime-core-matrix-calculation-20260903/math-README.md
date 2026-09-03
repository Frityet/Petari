# Quaternion dependency verification

The parent checkpoint adds Aurora's actual `PSMTXQuat`, the original JMath Euler and quaternion lerp functions, and a shared C-compatible `ppc_fres`. This audit changes no production math. It independently decodes the supplied retail instruction words and invokes the production source through a small native library.

| Routine | RMGK01 address | Size | Checks |
| --- | --- | --- | --- |
| `PSMTXQuat` | `0x804B8928` | `0xA4` | All 12 finite-arithmetic output graphs; 4,188 numeric cases |
| `JMAEulerToQuat` | `0x8044268C` | `0xD0` | All 4 output graphs; 69,701 numeric cases |
| `JMAQuatLerp` | `0x8044275C` | `0xFC` | Paired dot graph and all 8 outputs across both branches; 4,171 numeric cases |

All 78,060 cases pass under Apple clang 21.0.0 on the current macOS host. Of these, 77,985 compare every output bit, including signed zero. The other 75 cover extreme exponents and exceptional arithmetic: they compare every finite output bit, infinity signs and NaN classification. The total includes 58 alias cases. It includes all 65,536 signed-short X angles with two nontrivial fixed Y/Z angles, boundary cases on every Euler axis and 4,096 deterministic mixed-axis angles. Ordinary matrix cases span random nonunit quaternion scales from `2^-35` through `2^35`. Separate exponent cases extend from `2^-149` through `2^127`, including underflow/overflow, signed zero, infinity and NaN inputs. Lerp covers positive/negative/zero dot, extrapolation, endpoints and both input/output aliases.

`math-evidence.json` records source/library hashes, the verified DOL hash, graph hashes, result digest and exceptional classifications. `math-compile-commands.json` records the standalone build commands. This is not a full game build, a renderer result, or a claim about unspecified future compiler settings.

## Independent oracle

`math-oracle.py` reads the verified RMGK01 DOL (`25c5959534b3c21246c6c7e42021b916b41fb578`) directly. Its rejecting decoder supports only the scalar, integer/indexing and paired-single instructions encountered in these three functions. It decodes paired merges, horizontal sums, single-lane multipliers, fused multiply-add/subtract and negated subtraction from raw words; it does not rely on LLVM's incorrect disassembly of some paired-single instructions.

Symbolic execution retains each single-precision rounding boundary and each fused operation. Commutative multiply/add operands are canonicalized for the finite-arithmetic graph comparison. Source expressions are parsed separately from the current native code; the original root and native JMath function bodies must also be equal after stripping legacy trailing whitespace from each line. No tokens or internal spacing are normalized. In particular:

- `PSMTXQuat` forms norm as `(x*x + z*z)` and `(y*y + w*w)` with fused accumulation, performs the reciprocal estimate and one Newton step, and scales by two. It retains the original two-stage construction of off-diagonal terms, rather than substituting algebraically equivalent ordinary quaternion formulas. All input quaternion components are loaded before output writes.
- Euler halves signed angles with truncation toward zero, then uses the original signed-short table indexing. The output multiplies and adds/subtracts remain separate single-precision operations.
- Lerp's dot uses paired single fused accumulation, with the original summation order. Negative dot chooses the sign-adjusted branch. Both branches use separate single-precision output multiplies and additions, and do not normalize the result.

The numeric oracle uses libSystem `fmaf` for each decoded fused operation and float rounding for each scalar/paired result. Its reciprocal lookup is independently read from Dolphin's hardware-derived `fres_expected` numeric table. Inputs to these arithmetic operations are float values, so the Gekko multiplier's 25-bit operand reduction does not discard additional significand bits in this corpus. The oracle does not model FPSCR flags or enabled floating exceptions.

Euler sampling deliberately uses the same retained table values exposed by the compiled native table owner. The exhaustive tests therefore validate angle conversion/indexing and arithmetic, **not** an independent match of the host-generated trigonometric table values to a running Wii table. Earlier table-constructor evidence remains a separate dependency.

The native harness includes the actual Aurora matrix/vector translation units and actual `JMathQuaternionCompat.cpp` and trigonometric constructor. It copies no provider formulas. All temporary objects and the shared library remain under ignored `build/xanime-core-matrix-calculation-20260903/math/`.

## Shared reciprocal migration

The script compares the prior `J3DJointCompat.cpp` body at checkpoint `d40387003` with current Aurora `ppc_fres`. All 32 measured numeric table entries are unchanged. The whole integer algorithm matches after mechanical C adaptations: `std::bit_cast` becomes a union view, fixed-width type spellings change, the table reference becomes a pointer, and return bits are written through the union. The native `JMath::fastReciprocal` body must be exactly a call to `ppc_fres(value)`. No duplicate reciprocal implementation remains in that provider. This source check preserves the earlier signed-zero, infinity, NaN quieting, subnormal and large-input behavior without rerunning the earlier 1.3-million-case lookup sweep.

## Explicit exceptional boundary

There is a known NaN-sign difference outside the finite arithmetic claim. Retail `ps_nmsub` negates a normal fused result but preserves an already-NaN result's sign. The current portable `-fmaf(...)` also negates NaNs. A zero quaternion reaches this invalid arithmetic, so output classifications match while some NaN signs differ. NaN payload selection/quieting through all host fused operations and FPSCR state are likewise not established. This checkpoint intentionally documents that boundary rather than adding further math implementation changes. Zero quaternion is not silently converted to identity.

Reproduce the standalone provider build and all checks from the repository root:

```sh
python3 pc-port/notes/xanime-core-matrix-calculation-20260903/math-oracle.py
```

`--no-build` reuses the existing temporary library; use that only when the hashed production math files have not changed. `--cases N` changes the seeded random count per helper while preserving fixed boundary, alias, exponent and exhaustive signed-short tests.
