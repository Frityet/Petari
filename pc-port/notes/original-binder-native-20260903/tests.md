# Original Binder test coverage

2026-09-03. After the complete original Binder import, both changed test
executables were built and run successfully:

- `smg-pc-binder-kcl-mario-walk-tests`: passed.
- `smg-pc-aurora-native-tests`: all 29 groups passed.

Logs are `worker-collision-build.log`, `worker-binder-tests.log`, and
`worker-aurora-tests.log` in this directory. The first Binder run exposed a
test fixture that attached sensors without retained registration ownership;
the fixture now uses actual `StageCollisionRegistrationState` and explicitly
retires the owner. No production guard was changed.

`tests/BinderKclMarioWalkTests.cpp` replaces the stage-traced minimum-norm
fixture with simple nonorthogonal and opposing contact planes. Expected
component-extrema reactions are `(-0.4, 1.8, 2.4)` and `(1.5, 0, 0)`.
All fixture faces overlap the actual query radius. Their stored reaction
depths include the original post-contact 1.2. It compares the geometry probe's
returned contacts and reaction to the actual original
`Binder::obtainMomentFixReaction` on the returned contact records. A separate
constructed Binder/HitInfo fixture proves start-index selection, use of
mPlaneNum rather than the unused argument, conditional moving-part reaction,
and replacement of an empty range's output with zero. Static point and sphere
strike queries check zero `_7C`; their normal belongs to the parent Triangle.

The convex-seam fixture now reaches each face with a true physical overlap.
A 0.1 overlap produces 1.3 reaction after the margin. At the resulting
radius-plus-1.2 position, a subsequent zero-motion bind has no ground contact.
Skip-initial persists while a later physical overlap selects the next face.
The one-shot `_1EC._5` fixture starts overlapped and requests an outward and
tangential move: it returns only the 0.1 penetration response, without margin
or retry, and clears the flag. The next call returns the full `(2,2.3,0)`
displacement including margin and remaining movement.

Original plane-copy tests use three genuine sensors and actual floor/wall/roof
KCL contacts. With `_24 == 0`, mPlaneNum still records three temporary hits
and copy returns the three retained category caches. With allocated storage,
it returns the actual plane entries. Both sort by descending sensor pointer.
The second argument is deliberately 1 while the actual output array holds
four pointers, proving the unused argument without an undersized buffer.

`tests/AuroraNativeTests.cpp` now distinguishes contact normal from the static
zero `moving_reaction`. A focused vector group covers raw direction
`(2,0,0)` with source `(1,2,0)`, returning dot 2 and output `(-3,2,0)`;
source/output and direction/output aliases; the near-zero direction guard;
and in-place/copied normalization of `(3e-8,4e-8,0)` to approximately
`(.6,.8,0)`. It deliberately adds no zero-input test for MR::normalize.

The prior shell-support and host-epsilon assertions have been removed.
Separation one large-coordinate ULP beyond mRadius remains separation through
the real Binder probe. A shallow 0.01 physical overlap produces 1.21 stored
penetration. The matrix-offset fixture supplies raw matrix Y `(0,-2,0)` with
offset 2 and proves the original unnormalized offset of -4. A model's separate
negative or zero scale still does not enter this raw base-TR calculation.

These tests exercise original Binder execution with the current native prism
provider. They do not establish full Wii collision-list ordering, moving-part
geometry, or all KCollision query equivalence.
