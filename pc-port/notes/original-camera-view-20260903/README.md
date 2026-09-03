# Original camera view interpolator

The PC `Game/Camera/CameraViewInterpolator.cpp` and `.hpp` are byte-identical
root source/header copies. No Game algorithm, field, or constant was changed.
The original source is marked NonMatching in `configure.py:1135` and retains
its existing FIXME comments. This import does not imply a new binary match.

The original class retains its interpolation timer, target-relative movement
correction, quaternion camera switching, repulsion adjustment, Binder
collision correction, anti-oscillation state, and final matrix/FOV publication.
Its owner must retain the real object across frames and provide the original
`MR::setCameraViewMtx`, `setFovy`, and `getFovy` context. The parent task owns
that runtime integration. TriangleFilter and repulsive-area dependencies are
owned by another agent. This source-import task edits neither owner nor
camera output providers.

## Missing shared math

The existing PC layer already has the original `CameraLocalUtil::slerpCamera`
body and `TQuat4<f32>::slerp`, plus the original inline `MR::tanDegree` ratio
through the JMath trig functions. The missing matrix/quat interface was:

- `TRotation3::getQuat`: only declared in root, recovered as described below.
- `TRotation3::getRotate`: exact existing root inline body, which gets a
  quaternion and returns its axis/angle result.
- `TRotation3::setRotate(from,to)`: exact existing root inline body, using
  the existing original quaternion rotation and matrix setter.
- `TQuat4::getRotate`: existing root body from `JGeometry/TVec.hpp:937`, with
  one explicit host-layout adaptation. The native quaternion stores its four
  scalar fields directly, so a typed xyz vector value replaces the original
  `toTvec()` alias. The squared length, epsilon branch, reciprocal square
  root, axis scaling, and angle expression are otherwise unchanged.

The returned angle is in radians: it is `TUtil<f32>::acos(w) * 2.0f`, with
`acos` calling `JMAAcosRadian`. Quaternion vector length squared at or below
the original epsilon returns zero axis and zero angle. No new normalization
or threshold is inserted. Consequently the interpolator's nearly-end angle
test is a strict `> 1.0f` radians comparison, independently of its strict
`> 1.0f` translation-distance comparison.

The new shared `pc-port/src/JSystem/JGeometry/TMatrix.cpp` needs explicit
inclusion beside TQuat.cpp in the PC Game target's shared-source list. This
was reported to the parent, which owns build wiring.

## Matrix-to-quaternion recovery

The missing `TRotation3<TMatrix34<SMatrix34C<f32>>>::getQuat` was recovered
first into root `src/JSystem/JGeometry/TMatrix.cpp`, then copied unchanged to
the PC shared JGeometry directory. `AGENT_DECOMP_GUIDE.md` was followed; no
inline assembly or new camera-specific math was introduced.

The retail RMGK01 function is at `0x800173A0`, size `0x22C`. Its source DOL
SHA1 is `25c5959534b3c21246c6c7e42021b916b41fb578`. It computes the trace as
`m22 + (m00 + m11)` and branches on trace greater than or equal to zero.
The trace branch uses `sqrt(trace + 1)`; the remaining branches select the
maximum diagonal with the retail comparison order, then use
`sqrt(1 + (selected - (other + other)))`. Ties choose X before Y before Z.
Each branch computes `0.5f / root`, writes the dominant component as
`0.5f * root`, and uses the recorded sums/differences for the other three
components. The original `TUtil<f32>::sqrt` call is retained.

| Retail range | Recovered behavior |
| --- | --- |
| `0x800173A8-0x800173DC` | Trace evaluation and nonnegative branch. |
| `0x800173E0-0x80017438` | Positive-trace quaternion, W dominant. |
| `0x8001743C-0x8001746C` | Maximum diagonal and X equality branch. |
| `0x80017470-0x800174D8` | X-dominant quaternion. |
| `0x800174DC-0x8001754C` | Y equality test and Y-dominant quaternion. |
| `0x80017550-0x800175B0` | Z-dominant quaternion. |

The constants were verified from DOL bytes: `1.0f` at `0x806B7D30`, `0.0f`
at `0x806B7D34`, and `0.5f` at `0x806B7D40`. The square-root callee is
`TUtil<f32>::sqrt` at `0x80017320`. No extra guards, unit-quaternion fallback,
or post-normalization were added for degenerate matrices.

Retail `lookAtCenter` at `0x800B3F54` was also inspected. It uses the existing
quaternion `setRotate` and matrix `setQuat`, then concatenates after removing
the target translation and restores its position. The rotation setter
writes the nine rotation elements only. The imported shared overload keeps
that original contract and does not initialize translation as an added
camera policy. Raw disassembly and slices are under ignored
`build/compat-camera-view/`.

## Numerical boundary and verification

PC `TUtil<f32>::sqrt` currently uses native `std::sqrt`, and `inv_sqrt` uses
`1 / std::sqrt`. Original versions use a PowerPC reciprocal-square-root
estimate plus refinement. This pre-existing shared numerical boundary was
reported to the parent and remains outside these imports. The original
controller methods and scalar branch/order are preserved, but this work
does not claim instruction-level or bit-for-bit math equivalence for those
native primitives.

## Original compiler validation

The recovered root `TMatrix.cpp` was compiled with GC/3.0a3 through the
repository's existing macOS wibo, using the exact `cflags_game` list from
`configure.py` with `RMGK01` and `VERSION=0`. Compilation succeeded with no
diagnostics. The generated `getQuat` is `0x230` bytes / 140 instructions;
retail is `0x22C` bytes / 139 instructions. Its differences are one extra
diagonal reload plus register allocation and load/store scheduling. The
source was not changed to chase those differences.

The included `verify-getquat-object.py` resolves all thirteen relocations:
four calls to original `TUtil<f32>::sqrt` at `0x80017320` and nine loads of
the verified zero/one/half SDA2 constants. It verifies the retail SDA2 base
from startup instructions at `0x80004224`. It then decodes both functions'
floating operations and branches, tracking input matrix fields, the square-
root call's argument/result, and each quaternion component store. All
thirteen symbolic control paths agree, including the maximum-diagonal
comparisons and ties; those paths produce four identical quaternion output
formulas. The path enumeration includes branch combinations that may be
infeasible for particular matrix values rather than omitting them.

Every `f32` arithmetic operation remains a separate expression node. Only
the operands of an individual add or multiply may commute for finite matrix
values; expressions are never reassociated. This establishes functional
compiler correspondence for the recovered control flow and formulas. It
does not claim byte-identical instructions, a measured object-diff match
percentage, or equivalence of the separate native square-root primitive.

```sh
python3 pc-port/notes/original-camera-view-20260903/verify-getquat-object.py --compile
```

`getQuat-compiler-evidence.json` records compiler/object hashes, sizes,
relocation targets, output formulas, and compared control-path hashes. Exact
compiler commands, compiler output, objects, and disassembly remain under
ignored `build/compat-camera-view/`.

The new root TU's direct `JSystem/JGeometry/TMatrix.hpp` include resolves to
the tracked file `libs/JSystem/include/JSystem/JGeometry/TMatrix.hpp` through
the configured `-i libs/JSystem/include`. It does not require an umbrella
include or generated wrapper. Neither `include/JSystem` nor
`build/RMGK01/include` existed during verification. An additional original-
compiler run explicitly removed `-i build/RMGK01/include` and succeeded
without diagnostics; its relocated `getQuat` bytes are identical to the
configured-flags object and all thirteen control paths still agree with
retail. `include-resolution-evidence.json` records the tracked header/hash,
the exact command, and this result. Source includes and their hashes remain
unchanged because the existing include hierarchy is valid.

`source-correspondence.json` records the three whole-file hashes, the two
unchanged inline helper bodies, the exact two substitutions in quaternion
`getRotate`, and the retail instruction/constant ranges. `verify-source.py`
checks these records without building. The source agent performed these
source/hash checks, whitespace checks, and the original-compiler comparison
above. The parent owns native builds and runtime tests. No runtime, service,
event-camera, or test files were edited by this task.
