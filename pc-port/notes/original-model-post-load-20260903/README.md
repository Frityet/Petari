# Original model post-load utilities, 2026-09-03

`compat/ModelPostLoadCompat.cpp` supplies the exact existing root bodies for
`setShapeVcdVatCmdSelf`, `initEnvelopeAndEnvMapOrProjMapModelData`, `downFracVtx`
and `isUseFur`. Their original declarations were already present in the native
ModelUtil header. The four texture-mode predicates needed by the first routine
were already imported in MaterialTextureModeCompat.cpp and were left unchanged.
No new Game algorithm, model owner, fallback material or ResourceHolder instance
is introduced.

The original post-load behavior is retained:

- Materials without a position-matrix index are skipped. An envelope using an
  environment/projection texture matrix gets its own copied VCD/VAT command
  buffer, environment-map load flags, and the original texture-matrix index
  insertion calls. The original shape-table deduplication runs afterward.
- Vertex fraction reduction first returns when the position fraction is zero.
  Otherwise it decrements the authored position format fraction, arithmetically
  shifts all packed signed-16 positions by one, stores their range and rebuilds
  the model's original shape command buffers. Under C++23, negative signed right
  shifts have the required arithmetic semantics. No position type conversion,
  clamp, or alternate vertex count is added by this helper.
- Fur detection searches material names for the original case-sensitive `Fur`
  substring. The material name table and count come from the actual model.

The native command copy uses the existing shared J3DShape::kVcdVatDLSize constant.
It is 320 bytes in the native architecture because the general display-list
format can carry 64-bit array bases; the root Wii value is 192 bytes. The helper
body itself is identical and uses the real aligned allocation provider. Its
new allocations belong to the active original Game allocation domain, so the
complete resource owner must retain that heap and mutable geometry backing.

`verify.py` compiles the complete root ModelUtil unit with its configured
GC3.0a3 flags. Six relevant methods compare 100% with objdiff. `isUseFur` compares
99.82758% only because its SDA string label differs. The verifier then resolves
every actual function relocation for all seven methods and checks every byte
against the verified RMGK01 DOL:

| Method | Retail address | Bytes | Relocated bytes |
| --- | --- | ---: | --- |
| isEnvelope | 0x803E9A20 | 12 | exact |
| isUseTexMtx | 0x803E9D38 | 260 | exact |
| isUseTexMtxEnvMap | 0x803E9ED0 | 164 | exact |
| isUseTexMtxProjMap | 0x803E9F74 | 144 | exact |
| initEnvelopeAndEnvMapOrProjMapModelData | 0x803EA004 | 348 | exact |
| downFracVtx | 0x803EA7A4 | 200 | exact |
| isUseFur | 0x803EB180 | 116 | exact |

The verifier checks the actual r2/r13 initialization. For the string relocation,
it derives the effective address from the real r4 SDA argument instruction and
verifies `Fur\0` at 0x806B2660 against the compiled local string. It does not merely
find the same string somewhere in the binary. The inline command-copy helper is
included in the exact envelope routine bytes, and its source body is checked
separately.

All eight imported source bodies are checked byte-for-byte against root. The new
provider passes an isolated native syntax check. Original compilation reports two
pre-existing uninitialized-variable warnings in unrelated ModelUtil routines;
the native header reports its existing constant-range comparison warning. This
checkpoint does not claim an executed complete post-load model path: integration
waits for the parent's full typed model/resource-owner tests and the atomic
ResourceHolder migration. The shared production build was left with the parent.

Run:

```sh
python3 pc-port/notes/original-model-post-load-20260903/verify.py
```

The script uses the prior archive proof's local dtk split target. Objects and the
supplied DOL remain ignored under build/. JSON records current source hashes,
compiler arguments and resolved relocation addresses.
