# Original particle draw executor — 2026-09-03

Frozen root-first recovery and staged native callback closure. No production native changes, shared build, or GPU calls.

## Root checkpoint

Exact files in `root.patch` and `root/`:

- `src/Game/Effect/ParticleDrawExecutor.cpp`: every actual method and the original scene-draw-adaptor helper.
- `include/Game/Effect/ParticleDrawExecutor.hpp`: `initDraw` is static, as confirmed by its plain function-pointer callback in the DOL.
- `libs/JSystem/include/JSystem/JParticle/JPADrawInfo.hpp`: missing one-matrix constructor copies the camera matrix and initializes the texture projection matrix to identity.
- `include/Game/Util/Functor.hpp`: missing generic const-object/const-method overload, preserving every prior overload.

All 20 corresponding retail text symbols agree with the SHA1-verified DOL after relocation normalization (`dol-evidence.json`). The entire newly compiled source text for those 20 functions is instruction-identical to retail after relocation normalization (`ParticleDrawExecutor-normalized-source.json`, `verify-source.py`). Objdiff accounts for referenced compiler-generated constant/string/table labels: draw2D 99.69072%, indirect 99.88636%, adaptor init 99.82143%; all remaining functions and actual callback thunks/vtables 100%. Original full function sizes are retained.

The original 2D code uses signed EFB-height conversion, 608-wide fallback, half-width/half-height orthographic projection, near/far -1000/1000, and 0.5 texture projection scaling. Indirect particles use actual camera FOV/aspect and J3DSys camera matrix. It preserves fog/cull/depth/clip calls and all original draw groups:

| Adaptor | Scene draw category | JPA groups |
|---|---:|---|
| 3D | 71 | 0, 1 |
| 2D | 74 | 6, 7 |
| Indirect | 72 | 2 |
| After indirect | 73 | 3 |
| 2D model | 75 | 8 |
| Bloom | 76 | 4 |
| After image effect | 77 | 5 |

## Native staging

`stage.py` copies the complete original executor/header and JPADrawInfo header into ignored `build/original-particle-draw-executor-20260903/staged`. It also copies the already complete, unchanged original `src/Game/NameObj/NameObjAdaptor.cpp` and header: constructor, destructor, movement/calc/draw dispatch, and clone-owning connection methods. Both complete native TUs compile; `native-compiles.json` records commands. Existing EffectSystem header is a read-only staging prerequisite.

`native-headers.patch` makes two minimal generic native header additions:

- Functor.hpp: retain native FunctorBase's virtual destructor ABI; add actual const-object and free-function callback families, and honor the requested original heap in both method/free-function clone operations through existing placement-new allocation. No closure/lambda wrapper or fake callback object is introduced.
- ObjUtil.hpp: declare the actual MR::registerPreDrawFunction entrypoint. **Its provider is not implemented by this package.** It must be supplied by the actual scene scheduler owner before activating executor construction with adaptors.

Exact staged native files are also retained under `native/`; `native-manifest.json` distinguishes source and adapted staged hashes. Parent owns provider selection and atomic activation. Do not overwrite native Functor.hpp with the root header: the staged adaptation deliberately preserves the existing native virtual-destructor ABI.

## CPU evidence and limits

`verify-native.py` compiles the real NameObjAdaptor TU and callback ownership fixture with ASan/UBSan, links current native libraries, and passes with no diagnostics: three actual adaptor dispatches, two cloned callback dispatches, five heap ownership checks. Explicit clone target heaps win over a different current Game heap; implicit adaptor clones use the active original heap; native virtual destruction runs before heap retirement. Previously built libraries are not sanitizer-instrumented.

This is callback/allocator CPU proof and full executor compilation. It does not run JPA drawing or claim full EffectSystem runtime construction. The remaining scene pre-draw callback owner is material: root MR::registerPreDrawFunction forwards through GameSystem/SceneController to NameObjListExecutor, whose native owner path is not yet supplied. Complete executor object-level external references are retained in `ParticleDrawExecutor-undefined.txt`; those are references, not a claim that every listed provider is missing. Scene-global camera/render mode providers are also required by the original draw paths and remain parent-owned.

Reproduce from repository root:

```
python3 pc-port/notes/original-particle-draw-executor-20260903/verify-root.py
python3 pc-port/notes/original-particle-draw-executor-20260903/verify-dol.py
python3 pc-port/notes/original-particle-draw-executor-20260903/verify-source.py
python3 pc-port/notes/original-particle-draw-executor-20260903/stage.py
python3 pc-port/notes/original-particle-draw-executor-20260903/verify-native.py
```

When the real pre-draw scheduler owner is ready, copy `native/Game/Effect/ParticleDrawExecutor.{cpp,hpp}`, `native/Game/NameObj/NameObjAdaptor.{cpp,hpp}`, and `native/JSystem/JParticle/JPADrawInfo.hpp` to corresponding pc-port/src paths; apply `native-headers.patch`. The earlier actual EffectSystem/ParticleCalcExecutor/AutoEffectGroupHolder stage and AutoEffectGroup/Info stage remain required. No Game effect API no-op is supplied.
