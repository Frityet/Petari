# General actor rotation and vector interpolation compatibility

## What changed

`src/compat/GameMathCompat.cpp` now follows the recovered root implementations
for `blendQuatUpFront`, `vecBlendNormal`, `vecBlendSphere`, `vecRotAxis`, and
`turnRandomVector`. The new quaternion overload accepts a separate source and
destination. `TQuat4` supplies source-compatible copy and multiplication methods
with input/output aliasing and the original left-multiplication convention.

The vector-angle helper now uses the recovered 256-entry `acosEx` table close
to parallel or opposite directions. Table ownership is local and initialized
on first use; `initAcosTable` still exposes the original initialization API.
Out-of-domain/nonfinite `acosEx` values return NaN instead of an undefined host
floating-to-integer conversion.

No `Game/` source, actor identifier, stage identifier, route condition, or
progression state was changed. These are shared mathematical operations used
by player motion, gravity transitions, NPC orientation, and other actors.

## Why the old implementation was inaccurate

- `blendQuatUpFront` linearly blended axis vectors. The game rotates by a
  fraction of the angular distance to up, then rotates toward the desired
  front projected onto the new up plane. At the 0.1/0.2 rates used in
  `DemoRabbit::control`, a 90-degree up change should begin with a 9-degree
  rotation. Normalized linear interpolation instead rotates about 6.34 degrees.
- `vecBlendSphere` normalized both endpoints before its general spherical
  branch. The recovered function weights the original vectors and restores
  their interpolated length afterward. Different endpoint magnitudes therefore
  produce different directions. It also returns success for zero input vectors
  through its normalized-linear branch and rejects exactly opposite vectors
  without overwriting the destination.
- `vecRotAxis` always retained source length. The game copies the target vector,
  including its length, when it can reach that target in one rotation step or
  when either vector has zero length. The bounded case uses the normal SDK
  axis rotation and vector-matrix operations.
- The previous broad near-opposite cosine cutoff incorrectly selected
  `vecRotAxis` fallback for still well-defined spherical blends.

## Source evidence

- Root `src/Game/Util/MathUtil.cpp:43`: arccosine table initialization.
- Root `src/Game/Util/MathUtil.cpp:57`: table-indexed `acosEx`.
- Root `src/Game/Util/MathUtil.cpp:119`: random direction perturbation preserving length.
- Root `src/Game/Util/MathUtil.cpp:421`: both up/front quaternion blend overloads.
- Root `src/Game/Util/MathUtil.cpp:1217`: normalized vector blending.
- Root `src/Game/Util/MathUtil.cpp:1238`: spherical vector blending.
- Root `src/Game/Util/MathUtil.cpp:1270`: bounded axis rotation and target snapping.
- Root `src/Game/Util/MathUtil.cpp:1379`: vector-angle clamping and `acosEx` call.
- Root `libs/JSystem/include/JSystem/JGeometry/TVec.hpp:1016`: single-argument
  quaternion multiplication calls `PSQUATMultiply(&q, this, this)`.
- `aurora/lib/dolphin/mtx/quat.c:27`: host SDK multiplication component order.
- `src/Game/NPC/DemoRabbit.cpp:93`: the ordinary NPC gravity/facing blend call.
- `src/Game/Player/MarioMove.cpp:35`: spherical blend with bounded-axis fallback.

## Validation

The new `tests/GameMathRotationTests.cpp` exercises analytic rotations with
independent expected vectors: ordered quarter turns, the 9-degree/18-degree
actor blend, extrapolated rates, zero targets, opposite-direction recovery,
orthonormality, unequal vector magnitudes, zero inputs, exact and near opposite
vectors, aliasing, signed axis turns, target snapping, and a known arccosine
table cell. The parent task owns xmake target wiring and serialized build/run
verification.

Direct Clang syntax checking of `GameMathCompat.cpp` passed with `TARGET_PC=1`,
the existing source/Aurora include paths, and the forced
`compat/MetrowerksStdCompat.hpp` header. Only pre-existing missing-override
warnings from the game headers were emitted.

This restores the recovered algorithms and branch contracts. It is not a
bit-exact PowerPC floating-point claim: the existing JMath middle-range
arccosine/atan2 and scalar square-root providers still use host libm, and
platform floating-point operation rounding can differ. Retail resource or
interactive rabbit gameplay verification belongs to the enclosing runtime
milestone.

## Next source-facing methods found during the audit

`TPos3f::setQuat` can be brought over directly from the recovered
`libs/JSystem/include/JSystem/JGeometry/TMatrix.hpp:340`; it only writes the
rotation block and must preserve translation. The two-argument `TQuat4::slerp`
is declared but not defined in the root header. Its RMGK01 symbol is
`0x80016428`, size `0x1e8`; it needs reconstruction before the current host
three-argument approximation can be treated as source-equivalent.

## Current macOS validation

Serialized LLVM 23 build and `smg-pc-game-math-rotation-tests`: PASS.
Adjacent gravity-math checks: PASS after correcting the old ideal-cosine
expectation to the existing retail 14-bit table cell. NPCActor: PASS 6/6.
Real-disc Mario stand/walk/release/recreate: PASS, 325.685 units,
`Wait -> Run -> Wait`. Game/Player source mirror gates also pass.
