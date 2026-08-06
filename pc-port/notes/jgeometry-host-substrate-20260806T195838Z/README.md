# JGeometry host substrate phase 1

Updated: 2026-08-06T19:58:38Z

## Outcome

The PC port now has a canonical source-only `JGeometry::TVec3<f32>` with the global `TVec3f` alias used by the original game headers. It inherits Aurora's `Vec` storage and is statically verified to remain standard-layout, trivially copyable, 12 bytes, and aligned exactly like `Vec`.

The prior standalone `TVec3f` implementation was removed from `Game/LiveActor/LiveActor.hpp`. Game headers that previously forward-declared that standalone struct now include the compatibility vector definition. No actor, stage, route, placement, or gameplay behavior was added to `Game`.

## General compatibility surface

- `TVec3f` preserves the current host construction, zero-default, set/add/subtract/scale, dot, length, distance, and scalar operator behavior.
- The source-close rail/collection surface includes `squared`, `epsilonEquals`, `scaleAdd`, `killElement`, unary negation, compound arithmetic, cross product, and finite zero-safe normalization.
- `TUtil<T>` provides host math primitives and the original clamp boundary without importing the root JMath/paired-single implementation.
- `TPos3f` retains the original 3x4/48-byte shape, writable `MtxPtr` view, affine point/vector multiplication, translation access, and Euler extraction.
- `multTranspose(const TVec3f&, TVec3f&)` uses the writable destination signature proven by the RMGK02 symbol/assembly and safely supports the same object as source and destination. The root header's second `const TVec3f&` declaration is stale relative to that binary ABI.
- Both two-input and in-place matrix concatenation compute through temporary storage, so `result.concat(result, rhs)`, `result.concat(lhs, result)`, and `result.concat(rhs)` do not overwrite an operand while it is still being read.
- Narrow `TBox3f`/`TDirBox3f` definitions preserve the original member topology and half-open point-containment boundary.

## Focused test coverage

`JGeometry host layout and math` verifies:

- global/canonical type identity and Vec/TPos/TBox byte layouts;
- squared magnitude/distance, safe normalization, aliased `scaleAdd`, `killElement`, and clamp boundaries;
- affine concat results and aliasing in either operand position;
- writable, in-place `multTranspose` round-trip behavior;
- `toMtxPtr()` storage identity; and
- `TBox3f`'s exclusive maximum boundary.

## Verification

```text
xmake build smg-pc
[100%]: build ok

xmake build smg-pc-aurora-native-tests
[100%]: build ok

xmake test
[ok] JGeometry host layout and math
13 Aurora-native test(s) passed
100% tests passed, 0 test(s) failed
```

The scoped `git diff --check` passes, and a source sweep finds no remaining `struct TVec3f;` declaration under `pc-port/src`. AreaObj itself is intentionally not part of this phase.
