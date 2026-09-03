# Shared Gekko arithmetic boundary for JPA/J3D — 2026-09-03

The staged full original JPA resource/emitter simulation now completes cleanly under ASan and UBSan: 3,327 authored resource IDs, 225 textures, 45,386 original function-list entries, 52,571 CPU frames, and 352,896 particle-frame observations. Emitter/particle pool reuse, exhaustion, and heap retirement also pass. Counts are unchanged from the prior normal fixture. This is CPU evidence, not a rendering or full gameplay claim.

No production or root source is changed by this checkpoint. The frozen `original-jpa-draw-20260903` and `original-jpa-resource-loader-20260903` packages remain unchanged. Parent owns effect owner/API activation, ParticleEmitterHolder, EffectSystem, xmake and shared builds.

## Exact problem and arithmetic result

The native C++ direct float-to-halfword casts invoke undefined behavior on authored negative angles and positive 32768 rotations. Retail `JPABaseParticle::init_p` instead executes `fctiwz`, stores/loads the resulting 32-bit integer word, and then `sth` keeps its low halfword. Signed angular speed subsequently uses signed interpretation of those same bits.

Retail sphere emission intentionally visits poles where `mVolumeAngleNum == 0` and `mVolumeAngleMax == 1`. Its signed `divw` therefore evaluates 0/0. This is an ordinary original sphere algorithm state, not malformed authored data. The local Dolphin Gekko interpreter models the exceptional quotient as zero for nonnegative dividend and -1 for negative dividend (also -1 for INT_MIN/-1). The native C++ division must explicitly preserve that result instead of relying on host undefined behavior.

Instruction evidence from DOL SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`:

- `JPABaseParticle::init_p` at 8044A90C, angle conversion 8044ADD8–8044ADE4, speed conversion 8044AE0C–8044AE18: `fctiwz`, `stfd`, `lwz`, `sth`.
- `JPAVolumeSphere` at 8044844C: signed division at 804484A0 and 804484C0, low-halfword extraction, `fctiwz` at 804484F0.
- `VolumeSphere.asm` and `ParticleInit.asm` retain the full actual method disassembly.

The generic PowerPC divide-by-zero quotient is not claimed to be a portable C++ or architectural guarantee. The chosen exceptional value is the Gekko behavior implemented in the local Dolphin reference; `dolphin-reference.json` records its exact commit, paths, hashes, and line ranges. This work does not claim a new physical-console measurement.

## General native implementation

Four destination files, all copied under `native/` and mapped with SHA256 in `native-manifest.json`:

- `pc-port/aurora/include/aurora/ppc_math.hpp`: shared numeric result helpers for `fctiwz`, unsigned/signed low-halfword narrowing, signed `divw`, and word shift. No Game/actor/resource predicate is present. FPSCR/CR emulation is explicitly outside this value-only API.
- `pc-port/src/JSystem/JParticle/JPAParticle.cpp`: original source with the two floating rotation conversions routed through the shared halfword helpers.
- `pc-port/src/JSystem/JParticle/JPADynamicsBlock.cpp`: original source with the same float-angle conversion rule applied across volume types, signed division/word shifts for fixed circle/sphere sampling, and explicit low-halfword narrowing. Original counts, random calls, branches, and sampling arithmetic remain intact.
- `pc-port/src/compat/J3DAnimationInterpolation.hpp`: existing shared J3D sampling conversion/shift helpers delegate to this same Aurora boundary, eliminating separate implementations.

`native-only.patch` applies in the Petari root after the frozen JPA/draw package; `aurora.patch` applies inside `pc-port/aurora`. `native.patch` is the combined review diff. Alternatively copy the four concrete `native/` files to their corresponding destinations. No build-selector changes are bundled.

## Independent evidence

`oracle.py` executes the actual affected retail instruction slices, including load/store widths, rotation/masks, signed division, f32 arithmetic and fctiwz, with the exceptional instruction semantics above. Its native comparator passes:

- 266,257 fctiwz/sth inputs, covering every half-step through both halfword wrap boundaries, additional random float bit patterns, NaN, infinities, subnormals, and signed-32 overflow edges.
- 100 signed divw cases, including positive/negative zero divisors and INT_MIN/-1.
- 4,099 complete affected sphere arithmetic slices, including both ordinary poles. For a unit sweep the poles produce phi +16384/-16384 and theta -32768.

`arithmetic-verification.json` contains the sanitizer-enabled optimized native comparison and compile results for all three existing J3D animation callers (transform, material, additional). The narrow numeric oracle disables FP contraction so the original separate PPC single-precision arithmetic steps remain explicit; this is not a whole-engine floating-point parity claim.

`native-verification-asan.json` and `full-probe-asan-runtime.log` contain the actual manager simulation result. All 20 newly staged JPA/probe translation units are compiled with ASan/UBSan. Existing read-only Aurora/project libraries are linked; those prebuilt libraries are not newly instrumented by this fixture. No sanitizer diagnostics are emitted. The normal prior fixture result is retained in the frozen draw notes for comparison.

## Reproduction

```
python3 pc-port/notes/original-jpa-arithmetic-20260903/stage.py
python3 pc-port/notes/original-jpa-arithmetic-20260903/oracle.py
python3 pc-port/notes/original-jpa-arithmetic-20260903/verify-arithmetic.py
python3 pc-port/notes/original-jpa-arithmetic-20260903/verify-native.py --sanitize
```

Generated test vectors, compiler objects, and executables remain in ignored `build/original-jpa-arithmetic-20260903`. The archived disc resource remains in the earlier ignored loader build folder. No shared build or GPU is used.
