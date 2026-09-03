# Original J3D joint traversal and matrix calculators

This tranche enables original J3D joint construction, hierarchy traversal, and
Basic/Softimage/Maya transform calculation. The parent owns the live joint tree,
matrix-buffer storage, renderer integration, and native builds. The gateway
agent owns `OriginalJ3DJointTraversalTests.cpp`. No Game algorithm is changed.

## Source surface

`pc-port/src/JSystem/J3DGraphAnimator/J3DJoint.hpp` imports the original class,
callback type, fields, accessors, no-animation calculator template, and mode
helpers. The material include is replaced with the direct J3DSys dependency;
`addMesh(J3DMaterial*)` retains its original signature as a declaration. Its
material-dependent body is not needed by joint traversal. There is no fake
material implementation. `J3DMtxCalc.hpp` is a byte-identical root header copy.

`compat/J3DJointCompat.cpp` provides the original constructor, `appendChild`,
mode initialization/calculation bodies, calculator globals, and recovered
`recursiveCalc`. It also provides the directly required original matrix utility
bodies. Explicit local pragmas preserve the JSystem contraction-off setting.
`J3DSys` matrix/scale globals and the original `setMtxBuffer` provider belong to
the parent's joint-tree provider, not a second state map.

Root-first restorations are in their original source translation units:

- `src/JSystem/J3DGraphAnimator/J3DJoint.cpp`: `recursiveCalc` and its static
  `mCurrentMtxCalc` definition.
- `src/JSystem/JMath/JMath.cpp`: `JMAMTXApplyScale` creates a scale matrix then
  calls `PSMTXConcat`; `JMath::fastReciprocal` calls the original compiler's
  `__fres` intrinsic. No inline assembly was added.
- `src/JSystem/J3DGraphBase/J3DTransform.cpp`: the missing default transform
  constant, verified against all 32 original bytes at `0x8055C1B8`.

The existing root translation/rotation matrix overloads are imported unchanged.
The PC JMath header adds the missing scale-wrapper declaration and exposes the
shared reciprocal provider rather than an inline division approximation.

## Recovered traversal semantics

Retail `recursiveCalc`, `0x80438B1C` / `0x148`, does the following in order:

1. Save current matrix, cumulative scale, and parent scale.
2. Use this joint's calculator when present, otherwise inherit the current
   calculator. Set the current joint before calculation.
3. Cache the callback pointer after calculation, then invoke phase zero.
4. Traverse the child subtree.
5. Restore matrix and scales. Restore the saved calculator only when that saved
   pointer is nonnull, exactly as the original does.
6. Invoke phase one through the same cached callback, ignoring its return value.
7. Traverse the younger sibling.

The global current-joint pointer is not restored. An initially null inherited
calculator does not cause an unconditional reset to null on return. Phase-one
callbacks observe the restored parent matrix/scale state; a child's callback
cannot replace the parent's already cached callback. These are original
behaviors, not cleanup policies imposed by a native traversal wrapper.

The three original matrix modes preserve different scale rules. Basic embeds
local scale in the current matrix and tracks cumulative scale for its flag.
Softimage keeps the current orientation matrix unscaled, applies cumulative
scale to translations and the emitted matrix, and retains that unscaled state
for descendants. Maya optionally compensates matrix rows using the previous
joint's scale, does not compensate the translation column, and updates parent
scale after emitting its matrix. The original scale-compensation check is
exactly `== 1`.

## Reciprocal instruction boundary

Retail `JMath::fastReciprocal`, `0x8001B140`, contains `fres f1,f1; blr`.
The root `__fres` implementation produces these exact eight bytes with
GC/3.0a3. The native implementation handles float input bits directly and uses
the measured 32-entry Gekko estimate table also recorded by the local Dolphin
`Common/FloatUtils.cpp::fres_expected` oracle. It is a fresh float-input
implementation, not a mathematical reciprocal or Newton refinement.

For example, input `1.0f` produces bits `0x3F7FF800`, exactly
`0.9998779296875f`. Parent scale compensation must preserve that estimate even
when the scale component is one. The implementation also preserves input sign
for zero/infinity, quiets NaNs while keeping their payload, saturates very small
nonzero inputs, and flushes small estimates from input magnitudes at least
`2^126` to signed zero. FPSCR exception flags are not emulated.

`math_oracle.py` compares a source-equivalent Python model with the local
Dolphin double-input oracle. The 1,310,720 cases cover both signs, every table
step with low-bit endpoints, normal exponent representatives, subnormal input
normalization, the overflow/flush thresholds, and NaNs/infinities. This is
source-level numerical evidence; it is separate from the parent's compiled
native regression execution.

## Original compiler and retail evidence

The supplied DOL is RMGK01, SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`.

| Function | Original compiler comparison |
| --- | ---: |
| Basic calculator | 100% |
| Softimage calculator | 100% |
| `appendChild` | 100% |
| `JMAMTXApplyScale` | 100% |
| Recovered `fastReciprocal` intrinsic | Exact 8-byte match |
| Recovered `recursiveCalc` | 93.451220% |
| Existing Maya calculator | 83.135925% |
| Existing joint constructor | 83.239130% |

The recovered traversal retains all original branches and callback/traversal
ordering. Its remaining raw differences are register allocation and extra
member reloads around the local calculator. The existing Maya source calls
the reciprocal helper out of line while retail inlines its estimate, changing
register saves/scheduling. No matching-only source changes were made.

Existing mode-initializer and translation/rotation helper raw scores are lower
because of scheduling, register allocation, and materialization of vectors.
The complete scores and sizes are retained in `evidence.json`; they are not
presented as matching functions. To verify the matrix helpers independently,
`math_oracle.py` decodes actual table loads, integer angle indexing, scalar
floating-point operations, and all matrix stores from retail
`0x80423CD4` / `0xF0` and `0x80423DC4` / `0xB0`. Every one of their 12 output
expression graphs equals the corresponding original C++ expression, including
separate multiply/add/subtract ordering. The source mirror gate separately
checks those root bodies against the native provider.

## Reproduce

With the supplied DOL already extracted into ignored
`build/compat-math-oracle/main.dol` and the existing pinned compiler/tool setup,
run from the repository root:

```sh
python3 pc-port/notes/original-j3d-joint-traversal-20260903/verify.py
```

The script checks source correspondence, verifies retail output graphs and
reciprocal cases, compiles the three original PowerPC units, and uses DTK /
objdiff for comparison. It does not invoke a native build. A prior verified DTK
split can be reused with `--target-jsystem path/to/retail/obj/JSystem`.

All objects, extracted binary code, full comparisons, and generated DOL split
files remain under ignored `build/original-j3d-joint-traversal-20260903`.
The checked-in compact evidence records the successful source/original-compiler
verification. Native traversal/renderer validation belongs to the parent
checkpoint and should be reported separately.
