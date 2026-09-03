# Original Mario shadow view recovery

Recovered all eight missing `MarioActorShadow.cpp` methods from the verified RMGK01 rev0 executable. No native source or build wiring was changed in this task. These are original source/lifecycle dependencies for actual Mario model drawing, not a claim that the complete shadow renderer is active on PC.

Run the reproducible original-compiler check from the repository root:

```sh
python3 pc-port/notes/mario-shadow-view-20260903/verify-original.py
```

It uses `configure.py`'s `cflags_game`, GC/3.0a3, the Shift-JIS wrapper, and the existing verified retail split objects. DOL SHA1: `25c5959534b3c21246c6c7e42021b916b41fb578`. Generated objects, full diffs, compiler logs, and isolated baseline sources remain in `build/mario-shadow-view-20260903/`. `compiler-evidence.json` retains hashes, exact compiler commands, function sizes and relocations.

| Function | Retail address | Bytes | Objdiff |
|---|---:|---:|---:|
| calcViewReflectionModel | 802BFC5C | 1824 | 99.496% |
| calcViewWallShadowModel | 802C037C | 968 | 99.339% |
| drawShadow | 802C0744 | 560 | 99.036% |
| decideShadowMode | 802C09E0 | 576 | 98.889% |
| calcViewSilhouetteModel | 802C0C20 | 404 | 99.931% |
| calcViewBlurModel | 802C0EA4 | 472 | 96.975% |
| calcViewFootPrint | 802C107C | 304 | 96.961% |
| drawSilhouette | 802C11AC | 384 | 96.563% |
| MarioSwim::getWaterEdgeDist | 802C132C | 56 | 99.643% |
| J3DModel::getDrawMtx | 802C1364 | 32 | 100% |
| MR::calcCubeAxisZ | 80400830 | 92 | 100% |

Every newly recovered body's direct call sequence matches the retail object. The entire pooled string table matches the DOL byte-for-byte, including the Shift-JIS turn-jump animation name and all joint/area names. The signed/unsigned integer conversion double constants are independently checked against the DOL. Remaining differences are register choices, instruction scheduling/cached actor loads, function-local constant labels, and branch arrangement; the two exact helpers match all instructions.

## Source semantics retained

- Reflection modes 2, 3, 6 and 7 retain the original distinct plane/edge transforms, water-distance response, alpha changes by one per call, carried turtle shadow calculation, and view-matrix composition. The original call is `viewCalc3(3, nullptr)`, followed by `MR::loadViewMtx`; the previously documented nonnull ModelX oddity is not rewritten here. Unsupported/default modes do not initialize the local plane position in retail; callers must preserve the original mode-selection precondition. No invented fallback is added.
- Wall shading queries the actual named area types, stores the actual `AreaObj` pointer, traces a real map polygon, and uses its actual normal/hit position. Leaving an area retains the original 10-unit fade and prior area orientation. `updateRandomTexture` is the independently proven preceding recovery.
- Silhouette axes use the original `vecKillElement` projection. Footprints preserve the exact movement/draw flag checks, floor codes 13/26, 0.1 speed threshold, and water invalidation.
- Blur history preserves both eight-entry banks, demo suppression/countdown, every-third-tick sampling, bank toggle, and the original draw flags.
- Shadow mode selection preserves the original 500/0.75 constants, four single-step alpha adjustments, water camera tests, and actual `CollisionShadow::setUpdateFlag` call.
- `calcCubeAxisZ` calls the actual `AreaFormCube::calcWorldRotate`, builds its Euler rotation with `MR::makeMtxRotate`, and selects column Z. It does not substitute the local rotation or a normalized area matrix.

## Narrow existing declaration/index corrections

`MarioActor::_A70[8]` and `_A90[8]` were already indexed as one contiguous bank array by `initBlur`. They are now the actual sixteen-pointer `_A70[16]` storage. `initMember` retains the original eight iterations with interleaved bank writes (`i`, `i+8`), so the isolated before/after original-compiler comparison is **100%**. The independent concurrent screen-box `TVec2f` correction is held identical on both sides of that proof.

`drawModelBlur` previously indexed `_A70[idx + (_B10 << 5)]`. Retail shifts the bank by five **bytes**, or eight pointer elements on PPC. The corrected typed index is `_A70[idx + (_B10 << 3)]`. This prevents selecting an unrelated field when bank 1 is active. The pre-existing recovered `initBlur` and corrected `drawModelBlur` compare at 91.742% and 91.733%; the original real allocation/copy/draw behavior remains unchanged.

`_20C` is now `const AreaObj*`, preserving its original 32-bit layout and exact initializer instructions. Its only new consumers store/return the real area owner, not integer-address casts. `MarioSwim::calcWaterEdgeDist` had no callers or retail symbol; its existing body was renamed to the actual `getWaterEdgeDist` symbol verified at 802C132C.

## Parent integration boundary

Root source/header files for this recovery:

- `src/Game/Player/MarioActorShadow.cpp`
- `src/Game/Player/MarioActorDraw.cpp` (one blur-bank index)
- `src/Game/Player/MarioActorInit.cpp` (interleaved blur initialization; typed area null)
- `include/Game/Player/MarioActor.hpp` (sixteen-pointer storage; typed area pointer)
- `include/Game/Player/MarioSwim.hpp` (actual getter name)
- `libs/JSystem/include/JSystem/J3DGraphAnimator/J3DModel.hpp` (exact accessor)
- `src/Game/Util/AreaObjUtil.cpp` and `include/Game/Util/AreaObjUtil.hpp` (exact cube-axis helper)

Native minimum sync is those specific original bodies/declarations into the appropriate existing original-source providers, including the blur draw index and area-pointer declaration. Parent already synchronized the shared actor header's flat blur array during actor activation; do not overwrite its concurrent screen-vector changes.

A native import must link the actual `CollisionShadow` lifecycle/draw/capture owner, FootPrint owner, AreaFormCube world rotation, real map line query, and the original Mario swim/state providers used here. These methods do not authorize fake scene/camera providers or null-owner triangle substitutes. `CollisionShadow`'s broader full-source closure and native shadow rendering remain follow-up work. No xmake build, GPU run, or gameplay gate was run for this root-only recovery.
