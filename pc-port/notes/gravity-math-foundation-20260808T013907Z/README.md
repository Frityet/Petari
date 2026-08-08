# Gravity math foundation audit

## Scope

This change restores `pc-port/src/Game/Util/MathUtil.hpp` and
`pc-port/src/Game/Util/MathUtil.cpp` verbatim from the decomp tree and moves the
previous host-only provider subset into `pc-port/src/compat/GameMathCompat.cpp`.
The retail translation unit is intentionally excluded from the host target with
`remove_files("Util/MathUtil.cpp")` until its full dependency closure is
available. This is the only new xmake exclusion introduced by this work.

General host surfaces used by concrete gravity implementations now live in the
compatibility/JSystem boundary:

- canonical constants in `pc-port/src/math_types.hpp`;
- host compiler intrinsics in the already-forced
  `pc-port/src/compat/MetrowerksStdCompat.hpp` boundary;
- vector `isZero`, `killElement2`, and `TVec3s` support in the host TVec;
- rotation and pre-scale matrix providers in `pc-port/src/compat/MtxCompat.cpp`;
- MathUtil providers in `pc-port/src/compat/GameMathCompat.cpp`.

The excluded retail translation unit's canonical `gZeroVec` definition is also
provided there unchanged so `math_types.hpp` retains its real external object.

No gravity-stage special case, permissive fallback, or fabricated gravity object
was added.

## Source identity

`cmp -s` returned zero for both pairs:

```text
include/Game/Util/MathUtil.hpp == pc-port/src/Game/Util/MathUtil.hpp
src/Game/Util/MathUtil.cpp     == pc-port/src/Game/Util/MathUtil.cpp
```

SHA-256 evidence:

```text
44536d510a8506329817c50a13d230e2bb4c816d4554380c54ec2a4b29efb0  include/Game/Util/MathUtil.hpp
44536d510a8506329817c50a13d230e2bb4c816d4554380c54ec2a4b29efb0  pc-port/src/Game/Util/MathUtil.hpp
938e278b162b862d79f433c752cbf7d1803abad9196e14cdfe43393838a07bc5  src/Game/Util/MathUtil.cpp
938e278b162b862d79f433c752cbf7d1803abad9196e14cdfe43393838a07bc5  pc-port/src/Game/Util/MathUtil.cpp
```

## Retail projection evidence

RMGK02 symbol evidence:

```text
803e4d64 000000c8 T calcPerpendicFootToLineInside__2MRFPQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>
```

The disassembly in `projection-disassembly.txt` constructs `tail - tip`, computes
`(point.dot(line) - tip.dot(line)) / line.squared()`, clamps the normalized
parameter to `[0, 1]`, scales the line, writes `tip + line`, and returns the
parameter. `GameMathCompat.cpp` follows that operation order.

RMGK02's clamp compares `< min` and then `> max`. Unordered comparisons therefore
return the NaN input unchanged. A degenerate segment produces `0 / 0`, returns
NaN, and propagates NaN through all three output components; the focused suite
locks down that non-fallback behavior.

`max-element-disassembly.txt` records that `getMaxElement` calls
`getMaxElementIndex` and then loads the selected component. Its comparison tree
is exactly `x > y && x > z ? x : y > z ? y : z`; the focused suite covers
negative values, the x/y signed-zero tie (which selects y), and NaN in every
component position.

## Verification

Before the exact gravity/BaseMatrix integration began, both xmake targets and
their binaries passed:

```text
xmake -vD smg-pc-gravity-math-foundation-tests
build/linux/x86_64/debug/smg-pc-gravity-math-foundation-tests
gravity math foundation tests passed

xmake -vD smg-pc-aurora-native-tests
build/linux/x86_64/debug/smg-pc-aurora-native-tests
26 Aurora-native test(s) passed
```

After adding the RMGK02 edge cases, the focused test was rebuilt directly from
the owned provider translation units while the separate BaseMatrix closure was
still in progress:

```text
g++ -m64 -std=c++23 -Isrc/render -Isrc -Isrc/common -Iaurora/include \
  -DAURORA -DTARGET_PC \
  -include /workspaces/pcport/pc-port/src/compat/MetrowerksStdCompat.hpp \
  tests/GravityMathFoundationTests.cpp \
  src/compat/GameMathCompat.cpp src/compat/MtxCompat.cpp \
  -o /tmp/gravity-math-audit.HBeqag/gravity-math-foundation-tests
/tmp/gravity-math-audit.HBeqag/gravity-math-foundation-tests
gravity math foundation tests passed
```

All 13 exact concrete gravity translation units currently present also pass a
single host `g++ -fsyntax-only` audit under the production include, define, and
forced-compatibility flags: Cone, Cube, Disk, DiskTorus, GlobalGravityObj,
GraviryFollower, GravityCreator, GravityInfo, Parallel, Planet, Point, Segment,
and Wire.

The current full xmake link is owned/blocked by the concurrently integrated
BaseMatrix closure (`TMatrix34::invert`, `LiveActor::getBaseMtx`, and
`MR::getJMapInfoMatrixFromRT` at the time of this audit), not by a gravity/math
diagnostic.

The focused suite covers alias-safe scalar/direction separation, interior,
clamped, and degenerate segment projection, signed/tied/NaN maximum-element
selection, degree cosine and host square root behavior, canonical constants,
TVec zero threshold and projection removal, short-angle vector rotation, and all
pre-scale semantics used by the gravity sources.
