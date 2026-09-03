# Original camera vector compilation surface

Production SDK changes are limited to the shared PS dot/cross primitives, the native JMath scale/add provider, and original TVec forwarding/three missing camera operations. No camera controller algorithm, production registry owner, GameCameraCreator, DotCam reader, or Star Pointer provider was activated.

## Source correspondence

| Native surface | Original source / retail evidence | Adaptation |
| --- | --- | --- |
| `PSVECDotProduct` in `aurora/lib/dolphin/mtx/vec.c` | `src/RVL_SDK/mtx/vec.c:166`; `0x804B911C`, 32 bytes | Explicit float products, fused X-product plus rounded Y-product, then add rounded Z-product |
| `PSVECCrossProduct` in the same file | `src/RVL_SDK/mtx/vec.c:184`; `0x804B913C`, 60 bytes | Exact fused subtraction/lane permutation, separate post-rounded sign flips, actual GQR0 store conversion |
| `JMAVECScaleAdd` in `src/compat/JMathVectorCompat.cpp` | `src/JSystem/JMath/JMath.cpp:63`; `0x80442858`, 36 bytes | Three fused operations, all input reads before stores as scheduled in the actual retail code |
| TVec `dot`, `cross`, `length`, `squared` | `libs/JSystem/include/JSystem/JGeometry/TVec.hpp:623,711,784,721` | Calls actual PS primitives; squared uses original JMath square magnitude independently of dot's different accumulation order |
| TVec `orthogonalize`, `angle`, `orientation` | Same original header at 684,795,808 | Same original expressions and helper calls; explicit Vec pointer arguments and `std::fabs(float)` replace original implicit conversions/intrinsic spelling |
| PS names and `MTX_USE_PS` dot/cross selection | Original `libs/RVL_SDK/include/revolution/mtx.h` | Actual PS functions replace the former aliases to scalar C implementations; `MTX_USE_C` retains the C choice |

Original Game algorithms and root math implementations are unchanged. The native math members that call runtime SDK functions no longer claim `constexpr`. Existing other TVec methods are outside this bounded audit; this checkpoint does not assert the entire vector/math SDK is now identical to retail.

`ppc_psq_store_f32` implements the GQR0 float-store conversion that clears subnormal magnitude while preserving sign. `ppc_ps_neg_f32` flips the float sign bit after an observable rounded source. The volatile float boundary is intentional: both source `-fmaf(...)` and a plain integer sign XOR were recognized by the ARM64 compiler as a negated fused instruction, which gave positive zero where the original separately rounded `ps_msub` followed by `ps_neg` gives negative zero. The bit-sign operation after the observable value prevents that transformation. There is no epsilon or numerical threshold added to Game math.

The original compiler schedules the Z loads in `JMAVECScaleAdd` ahead of its XY store, although the inline assembly source text lists those loads later. The first native draft followed the text order; the direct retail oracle's partial-overlap case exposed the discrepancy. The final native implementation follows the actual compiled retail load/store order. The root compiler proof is still exact because GC3.0a3 performs that scheduling on the unchanged original source.

## Verification

```sh
python3 pc-port/notes/original-camera-registration-20260903/verify-vector-original.py
python3 pc-port/notes/original-camera-registration-20260903/verify-vector-native.py
python3 pc-port/notes/original-camera-registration-20260903/probe-native.py
```

The original-compiler script compiles the actual root JMath and SDK vec translation units using configured GC3.0a3 flags. **All three functions are 100% objdiff, with every compiled instruction byte identical to current verified RMGK01**: 32 instructions / 128 bytes total. No relocation masking or ignored instructions are needed. `vector-original-evidence.json` contains commands and source hashes. The actual TVec dot at `0x8001D2A8` uses different FPR allocation and load scheduling; the independent decoded operation-graph comparison proves the same dot accumulation.

The native verifier decodes the actual DOL instruction words instead of repeating the native formula. **35,043 calls match every output float bit**, covering 5,005 finite moderate-range vector pairs, signed zero and fused-cancellation witnesses, distinct/output-left/output-right modes, and eight partially overlapping input/output layouts. The script additionally compares the TVec dot's symbolic operation graph with the SDK dot.

`tests/OriginalCameraVectorMathTests.cpp` has **five groups passing both optimized and ASan/UBSan builds**, with all linked workspace sources instrumented and leak detection/error halting enabled. It checks analytically known fused cancellation (`-2^-46`), negative-zero cross output, aliasing, actual reciprocal-square-root magnitude, raw nonunit orthogonalization, handedness, exact original axis/antiparallel radian angles, zero cases, and signed subnormal store conversion. The test uses actual production functions and original table owners, without fake Game or SDK objects. Homebrew LLVM is used explicitly; the system Apple ASan runtime cannot enable leak detection.

These checks do not claim complete FPSCR state, signaling-NaN payload propagation, subnormal arithmetic emulation, or full scene/controller runtime. GQR0 **store** conversion is separately tested; it is not a blanket assertion of every floating-point input boundary. `vector-native-evidence.json` records this limit and all build commands/source hashes.

The complete catalog compile probe now passes **96/97 TUs** against native headers. CameraDPD still rejects the absent real Star Pointer depth getter. This is intentionally visible and recorded in the registry README. The probe is isolated and does not link or instantiate the complete production director.

## Deferred compiler-only imports

`staged/Game/Camera/` retains exact root GameCameraCreator and DotCamParams headers and source copies for the next complete registry import. Only two source differences exist:

- GameCameraCreator removes redundant `<mem.h>`; `<cstring>` already declares the only used operation, `memset`.
- DotCam uses `const JMapInfo::DataCompat* mapData = mMapInfo.mData.get()` in its existing iterator end test. The existing shared typed JMap ownership retains entry count and identity; all branches and iteration semantics remain unchanged.

These files live in notes, outside the production Game glob. Their eventual import must be atomic with the complete real registry/owner dependency closure. The probe explicitly records these two staged paths rather than pretending unchanged root source compiles directly.
