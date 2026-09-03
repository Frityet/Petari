# Original JPA resource loading boundary — 2026-09-03

## Result and scope

The staged shared resource layer decodes the real JPAC2-10 archive into owned, aligned storage containing **the original JPA data structures**, with every numeric field and embedded animation table in host order. The staged SDK loader constructs the original JPAResource, block classes and JPATexture, calls the complete original JPAResource::init, and preserves each original user index and texture-table index. It does not create replacement emitters, map IDs to array positions, discard fields, replace dispatch callbacks, or add Game branches.

The CPU parser and actual complete block constructors pass normal and AddressSanitizer/UndefinedBehaviorSanitizer checks against real disc data. **Full JPAResourceManager construction is not yet linked or runtime-proven**: the complete Resource::init dispatch graph reaches twelve missing original methods (eleven unresolved linker entries). Full EffectKeeper/MultiEmitter/EffectSystem ownership and Game effect API activation remain gated by that original closure and the preceding construction report.

No native production source, production effect header, xmake file, shared target, GPU context or game scene was modified/run by this tranche. The parent committed the separately approved root pointer-width correction as `89430a7c1`.

## Files and review/apply boundary

- `native/resource/JpcResource.{hpp,cpp}`: new complete shared decoder, immutable typed block storage, retained bounded-source registration.
- `native/JSystem/JParticle/JPAResourceLoader.cpp`: SDK traversal uses decoded records and original allocation/construction/registration calls. Unknown structurally valid blocks retain the original ignore behavior.
- `native/JSystem/JParticle/JPAResourceManager.{hpp,cpp}`: original methods plus a native shared lease for decoded data and JKR heap finalization. The manager registers before construction so texture finalizers run before its backing lease releases. An exception unregisters the manager finalizer. Stack/host objects release the lease through their destructor.
- Remaining original `native/JSystem/JParticle/*.cpp`: literal original bodies, except the reviewed general native color-table bounds guard in JPABaseShape and the root-first pointer-sized JPAResource dispatch allocations.
- `native/JSystem/JGeometry/TVec.hpp`: restores the literal original `setLength(const TVec3&, f32)` overload. This is independently reviewable via `general-vector-header.patch`; no existing overload is replaced.
- `native.patch`: complete staged native package relative to repository root. `native-manifest.json` records every destination and SHA256. It intentionally contains no build selection or production activation.
- `root-pointer-width.patch`: exactly seven original JPAResource pointer-list size/alignment fixes. Already committed by parent; do not reapply.
- `native-color-guard.patch`: focused review diff against original JPABaseShape. Root JPABaseShape is unchanged.

After the missing original methods and owner graph are complete, check `git apply --check pc-port/notes/original-jpa-resource-loader-20260903/native.patch` before applying. It adds all SDK sources and replaces the native TVec header's one missing overload only. The equivalent exact-copy manifest is `native-manifest.json`; each `native/<relative>` maps to `pc-port/src/<relative>`. Do not publish only the new manager layout to already compiled consumers. All users must compile against the same native header priority (native, Aurora, original fallback).

## Data and lifetime contract

`register_jpc_source(span, shared_owner)` accepts the original immutable archive pointer and its exact bounded range. It validates/decodes while retaining the source owner, publishes a borrowed original identity, and returns an explicit registration handle. Duplicate registration of the same address requires the same complete range and owner. An unregistered pointer is rejected before an unsized read. The original `JPAResourceManager(void*, JKRHeap*)` entry point resolves that identity and retains the same decoded resource in `mNativeResource`; releasing archive registrations afterwards cannot invalidate its JPA block pointers.

Decoded blocks own aligned allocations, establish the lifetime of the real typed header and embedded scalar/key arrays, and preserve offsets within each block. No native pointer is read from JPC bytes. The resource record's user index is retained exactly; the CPU probe also changes one index to 65000 and verifies it remains 65000. The original manager's linear `getResource(userID)` algorithm is unchanged.

The source copy remains immutable; host block storage is separate. Color channel bytes, texture-index animation bytes, names, texture/palette tile payloads and unknown block payloads are kept as bytes. ResTIMG multibyte fields are decoded before the actual JUTTexture sees them. Mipmap image ranges are bounded according to GX tile format and authored image count; palette spans are bounded separately. There is no rendering or texture upload in the CPU test: GXInitTexObj initializes CPU metadata and no texture load/draw is issued.

The eventual ParticleData archive owner must register `particles.jpc` alongside its retained JMap tables before calling the actual ParticleResourceHolder constructor. That registration is an archive/resource service responsibility, not a change to ParticleResourceHolder or a particle-name special case. A real process/scene JKR domain owns the resource and emitter objects. The emitter manager and all live emitters must retire before their resource manager/heap. This tranche supplies the storage lease and manager heap finalizer, not the remaining Game service ownership.

## Conversion coverage and real-data counts

The Korean disc contains `ParticleData/Effect.arc`, not `Particles/Effect.arc`. The ignored extraction is 813,696 bytes; the archive parser produces a 2,074,304-byte particles.jpc. JPC SHA256: `c6d0e7208a98af0d8586d3eb1cdf691b4cde3701bef5f03f37c76f7d7732788c`.

| Kind | Count | Host conversion |
|---|---:|---|
| BEM1 | 3327 | flags/user data, all vectors/floats, rotation/frame/lifetime/volume halfwords |
| BSP1 | 3327 | flags/offsets/scales/blend/frame fields, texture-coordinate float tables, color-key indices |
| ESP1 | 3256 | flags, all scale/alpha/rotation floats, cycle halfwords |
| SSP1 | 302 | flags, all numeric child fields, lifetime/rate/rotation halfwords; colors unchanged |
| ETX1 | 401 | flags and the complete indirect matrix |
| FLD1 | 2097 | flags, position/direction/magnitude/timing floats |
| KFA1 | 388 | all authored time/value/in/out tangent float records |
| TDB1 | 3327 | all texture references as native u16, preserving texture identity |
| TEX1 | 225 | ResTIMG dimensions/counts/offsets/LOD bias; encoded payload unchanged |

The existing `render/effects/EffectResource` parser is reused as a real-data comparison oracle. It has a useful RARC/BCSV/text-decoding lifetime and metadata contract but omits complete ESP1/ETX1/FLD1 data and original inline color/coordinate tables, so it cannot supply original JPA block backing. Its own JpcEffectEmitterInstance graph is not reused as an original JPA emitter. No production duplicate simulation path is activated.

## Native safety findings

1. Original JPAResource::init used `count * 4, 4` for seven function-pointer arrays. Root-first `sizeof(EmitterFunc)`/`sizeof(ParticleFunc)` sizes and alignments preserve the exact Wii values. The original compiler comparison reports all 37 compared symbols at 100%, including data/relocations. See `root-evidence.json` and `verify-root.py`.
2. Retail `makeColorTable` reads the next key index even after consuming the last declared key. DOL `0x804472B8/0x280` confirms this behavior (`makeColorTable.asm`). Independent host block allocations make the read invalid. The native guard adds only `j < keyCount` before accessing the next index. The independent contiguous-source oracle covers **all 1,172 authored color tables**. Exactly 340 tables make 7,383 reads beyond their declared key counts; **zero output tables differ** through maxFrame. One additional table has a key after maxFrame and never overreads. `color-equivalence.json` lists every affected table and actual adjacent index values. The compiled original constructor with the guard produces exactly the oracle's 182,156 color bytes. This is a checked claim for this archive, not a claim about arbitrary adjacent-memory behavior in other assets.
3. A native TVec3 source/length overload was absent. Copying its full original body closes JPAFieldBlock and JPAParticle compilation without editing either algorithm.

## Verification and current full-link frontier

- All sixteen staged resource/JPA source TUs compile with production include priority and the staged native header additions. `native-compiles.json` records commands/hashes.
- `probe-runtime.log` / `probe-asan-runtime.log`: 3,327 records, 225 textures, all eight block kinds, 83,175 numeric dynamics-word comparisons, texture payload equality, five malformed-range/header rejections, preserved user ID, and source/registration release with a retained data lease.
- `block-probe-runtime.log` / `block-probe-asan-runtime.log`: 11,226 actual original block constructors, 1,172 color tables, all 182,156 bytes equal to the contiguous-retail oracle, 1,294 original key interpolation results equal to authored values, actual JKR heap creation/retirement. No sanitizer diagnostics.
- `full-probe.cpp` invokes the actual complete manager constructor and actual emitter manager/create/delete API, but its link is blocked. `full-probe-link.log` and `full-probe-link.json` retain the exact frontier and command. No replacement provider was written to force the probe to pass.

| Missing original method in Game/System/Overwrite | Retail address | Size |
|---|---|---:|
| JPADrawDirection | 0x803A514C | 0x1AC |
| JPADrawRotDirection | 0x803A52F8 | 0x224 |
| JPADrawDBillboard | 0x803A551C | 0x174 |
| JPADrawStripe | 0x803A57DC | 0x3EC |
| JPADrawStripeX | 0x803A5BC8 | 0x6F8 |
| JPAFieldAir::prepare | 0x803A62C0 | 0x8C |
| JPAFieldVortex::prepare | 0x803A634C | 0x64 |
| JPAFieldVortex::calc | 0x803A63B0 | 0xEC |
| JPAFieldConvection::prepare | 0x803A649C | 0xA0 |
| JPAFieldSpin::prepare | 0x803A653C | 0x98 |
| JPADrawYBillboard | 0x803A66B8 | 0x128 |
| JPADrawRotYBillboard | 0x803A67E0 | 0x15C |

The four missing prepare key functions surface as four undefined vtables; Vortex also needs its missing calc when its vtable is emitted. Existing Air/Convection/Spin calc bodies remain complete in JPAFieldBlock.cpp. This is the next bounded original SDK closure, before a full manager/emitter CPU test can run.

## Reproduction

Keep all disc-derived bytes in ignored `build/original-jpa-resource-loader-20260903`. The durable notes contain only source, scripts, hashes, summaries and disassembly; no archive/JPC/color asset binaries.

1. Restore `native/` to `build/original-jpa-resource-loader-20260903/staged/`; copy the three probes, `extract_file.c`, and `JPAResource-before.cpp` from these notes into that build directory.
2. Compile `extract_file.c` with the installed encounter-nod include path and libnod.a recorded in the root task tools, then run `extract_file <root-rvz> ParticleData/Effect.arc <build>/Effect.arc`. This is the existing nod disc API, not an alternate asset downloader.
3. Run `python3 pc-port/notes/original-jpa-resource-loader-20260903/compile.py` and `python3 pc-port/notes/original-jpa-resource-loader-20260903/link.py`. The parser probe writes `<build>/Effect.arc.jpc` solely for the independent oracle.
4. Run `verify-colors.py`, then `verify-native.py` and `verify-native.py --sanitize` from this notes directory. All are standalone commands; none invokes xmake or a GPU.
5. `verify-root.py` checks the original SDK correction. `link.py --full` uses `full-probe.cpp` and the literal prior `OriginalJPAEmitterInit.cpp` compiled by `compile.py`, and currently records the expected missing-original-provider frontier.

The provided link scripts reuse the already built static dependency archives. They do not imply that the complete showcase or original particle rendering was built or tested.
