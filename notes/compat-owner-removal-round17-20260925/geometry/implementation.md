# Shared geometry compilation closure

Restored the missing existing donor API surface from `decomp/libs/JSystem/include/JSystem/JGeometry/{TVec,TMatrix}.hpp` in the actual native JGeometry headers. This extends the shared SDK layer rather than changing the affected Game call sites.

- `TVec.hpp`: donor TVec2 zero/normalize; TVec3 `(xz,y)` constructor, component multiplication, multInLine/2, orthogonalize2, cubic Hermite interpolation, copy, turnRate; signed-byte TVec3Sc alias; donor TVec4 scale body.
- `TQuat.hpp`: donor two-value constructor, Quaternion copy constructor, axis Euler setters and scalar axis-angle overload. Restored original inheritance through TVec4 to SDK Quaternion so DinoPackun/QuestionCoin value conversions and BegomanBase PSQUATMultiply pointer conversions use their actual original type relationship. Existing native default identity initialization and existing quaternion math bodies are retained. SDK size/alignment, standard layout, and trivial-copy invariants remain explicit static assertions.
- `TMatrix.hpp`: donor 12-value set, distinct basis-only and entire-affine scale operations, scalar direction setters, getYDir2, row-oriented setXYZDir2, setEuler, zeroTransInline2, setRT, normalizeBasis, and capped-angle makeRotate. Existing native bodies and root's already-added setEulerX/Y are untouched. SIN/COS trivial donor wrappers are expressed directly with the same global sin/cos calls as the surrounding native file. The donor's reported nonmatching setRT implementation and unused normalizeBasis aggregate remain preserved rather than replaced by a speculative algorithm.
- Factory includes the real MercatorTransformCube header. Root's prior NameObjFactoryStubs include is retained.

Native adaptations are limited to ordinary template dependent-base lookup, existing native vector component multiplication, an explicit pointer argument to the existing JMAVECScaleAdd SDK function, and the established native constexpr vector constructor style. Existing PPC integer conversion helpers, native alias-safe math, and matrix Euler implementations remain unchanged.

Coordinated removal of the misplaced MapPartsBreaker TVec2::squared body with the map agent because the native generic TVec2 already owns that method. The map agent also owns Note's mixed floating-literal call fix.

`before/`, `after/`, `owned-manifest.json`, and `geometry-only.patch` isolate these changes from the earlier factory/root edits. No builds, tests, or new test cases were run or added. `git diff --check` passed for owned source paths. Root owns the integrated compiler pass and runtime smoke.
