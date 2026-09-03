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
bit-exact PowerPC floating-point claim: scalar atan2, square-root, and
trigonometric table generation still use host libm, and platform
floating-point operation rounding can differ. Retail resource or
interactive rabbit gameplay verification belongs to the enclosing runtime
milestone.

## Follow-up quaternion and matrix recovery

The two-argument `TQuat4<f32>::slerp` was recovered from the verified supplied
RMGK01 binary at `0x80016428`, size `0x1e8`. The first implementation is
`src/JSystem/JGeometry/TQuat.cpp` at the root, copied verbatim to the PC
`src/JSystem/JGeometry/TQuat.cpp`. The accompanying asin table constructor was
likewise recovered first in root
`src/JSystem/JMath/JMATrigonometricTable.cpp`, then copied verbatim to the PC
tree. Detailed address and constant evidence is in `slerp-reconstruction.md`.

Both slerp overloads now use the recovered behavior: normalize input copies,
choose the short quaternion hemisphere, use the retail squared-epsilon linear
threshold, retain rates outside zero-to-one, and preserve the computed result
without an added final normalization. Quaternion normalization now uses the
retail `32 * FLT_EPSILON` squared-length cutoff. The shared JMath arccosine
lookup uses the recovered asin table grid and integer index truncation.

`TQuat4::makeMtx`, `TRotation3::setQuat`, and
`TPosition3::makeQuat`/`setQT` follow the existing recovered root headers.
`setQuat` and `makeMtx` preserve translation; `makeQuat` clears it. None silently
normalizes input quaternion components.

The focused tests now also cover nonunit slerp inputs, positive and negative
rate extrapolation, opposite quaternion signs, both sides of the small-angle
linear threshold, retained unnormalized results, source/copy alias ordering,
normalization epsilon, shared table entries, and matrix translation behavior.
These general primitives serve both actor movement and camera orientation;
the follow-up does not activate new NPC or demo behavior.

## Earlier vector-rotation checkpoint validation

Serialized LLVM 23 build and `smg-pc-game-math-rotation-tests`: PASS.
Adjacent gravity-math checks: PASS after correcting the old ideal-cosine
expectation to the existing retail 14-bit table cell. NPCActor: PASS 6/6.
Real-disc Mario stand/walk/release/recreate: PASS, 325.685 units,
`Wait -> Run -> Wait`. Game/Player source mirror gates also pass.

## Quaternion reconstruction validation

LLVM 23 rebuilt `smg-pc-game-math-rotation-tests` with both new root-mirrored
source files on macOS arm64. The expanded math test passed, including the
new slerp, table and matrix cases. New movement/camera integration results
are tracked separately in `../movement-camera-20260903T043000Z/`.
