# Original effect ownership and portable hash payloads

The generic hash payload package is ready independently of effect activation. The full original effect code compiles in isolation, but the native runtime still lacks the actual EffectKeeper / MultiEmitter / JPA owner graph that its methods require. No native production source or shared build was modified by this tranche.

## Apply the bounded payload package

From the repository root:

```sh
git apply --check pc-port/notes/original-effect-owner-20260903/payload/native.patch
git apply pc-port/notes/original-effect-owner-20260903/payload/native.patch
python3 pc-port/notes/original-effect-owner-20260903/verify-payload.py
```

The native patch changes exactly nine existing files. It contains no effect API expansion, effect flag changes, original owner activation, encoding change, or build selection change. Its MarioEffect changes are only the actual table pointer insertion and two lookup result types. `payload/native/` contains frozen resulting files; `payload/manifest.json` records before/after SHA256 values. Review and apply the patch rather than copying stale whole files if parent work has advanced.

The root payload changes are already in the shared tree. `payload/root.patch` records the ten affected root paths and excludes effect flag changes. `payload/root/` is a frozen root snapshot. Root PlayerEvent and EffectKeeper have no native source mirrors; their future native builds use the updated originals. GroupChecker needs no source change because it inserts scalar zero and searches without a result pointer.

`HashSortTable::Value` is `uintptr_t`. Keys and hash arithmetic stay u32; bucket indices stay u16 and table counts stay u32. Only payload storage, method arguments/results and sort scratch storage widen. Pointer callers retain actual pointers; scalar/index callers use the same payload type. Native MarioAnimatorLifetime captures a `Value*` for the payload array so destruction matches allocation. The original table capacity and 0x400 sorting limit are unchanged.

Changing the header and provider requires recompiling every caller together. Active native sources synchronized here are XanimeResource, MarioAnimationEfx, MatrixControl, OriginalMarioSound and MarioAnimatorLifetime; inactive MarioSound and MarioEffect mirrors are also synchronized. Do not link old objects expecting u32 payload signatures to the new provider. This is an intentional portable data-model correction, with no ABI compatibility shim.

## Evidence

* `verification.json`: all nine HashSortTable functions have identical original-compiler code and relocations before/after the payload change. The Wii compile-time checks retain four-byte Value, table size 0x1c, and payload pointer offset 8. This is equivalence to the previous root implementation, not a new claim that all prior root code matches retail.
* `retail-compiles.json`: complete HashUtil, EffectKeeper, MarioEffect, MarioSound, MarioAnimationEfx, MatrixControl, PlayerEvent, XanimeResource and GroupChecker compile with configured GC/3.0a3 game flags.
* `native-compiles.json`: all ten staged Game/provider units compile with the complete original effect header; the additional MarioAnimatorLifetime unit also compiles. Only MarioEffect needs the expanded effect API header for this compile proof.
* `payload/verification.json`: the seven native payload source units that already have a complete native API compile using ordinary production headers. The independent payload package includes no staged EffectUtil header. Full MarioEffect remains gated on its pre-existing incomplete effect API.
* `payload/runtime.log`: a real constructed HashSortTable retains actual pointers above UINT32_MAX, a full-width scalar, zero and index values across unsorted insertion, sorting and lookup; duplicate skip and missing lookup behave correctly. Its arrays are released using their actual types. The probe links only the exact frozen table provider and probe, using normal dead stripping of uncalled string-key methods; it supplies no replacement Game methods or fake objects.

Reproduce the complete original/native compile and Wii equivalence checks with `verify.py`; reproduce the independently publishable package with `verify-payload.py`. `freeze.py` regenerates review packages against current production files and should only be rerun deliberately, because the recorded baseline then changes. All binary output stays in `build/original-effect-owner-20260903/`.

## Effect activation is separate

`effect-activation/` contains the complete original EffectUtil header, full original EffectKeeper and MarioEffect sources, and the literal complete LiveActor::initEffectKeeper body. The latter also compiles (`keeper-init-compile.json`). `root-flags.patch` contains only the numeric FlagWord extraction changes, separate from the generic payload change. No partial-object runtime probe was used.

MarioEffect initializes packed flags numerically. Its former mByte0/1/2 reads select host memory order; the staged source instead extracts bits 31..24, 23..16 and 15..8 from mWord, respectively. All original branches, loops, table entries and emitters are retained. This solves the concrete little-endian field interpretation while preserving the original meaning on Wii. It does not solve ownership or text encoding.

The owner graph needed before MarioActor::initEffect is called is:

1. A scene-owned EffectSystem with ParticleResourceHolder, AutoEffectGroupHolder, ParticleEmitterHolder, JPAEmitterManager and update/draw executors. The scene registers this at SceneObj_EffectSystem before actors initialize.
2. LiveActor::initEffectKeeper constructs the actual EffectKeeper with the actor model ResourceHolder, capacity and group, enables sorting, registers auto effects and binds the actual Binder. Native LiveActor currently only registers a host with RuntimeContext's EffectService, leaving mEffectKeeper null.
3. MarioActor::initEffect adds 0xB8 capacity plus auto effects, registers common and material variants against actual model joint matrices and actor transforms, renames auto effects, initializes the WaterColumn ModelObj, and finalizes sorting. Later methods require the original Mario state graph including mSwim, actual Binder/Triangle data, and the MarioEffect callback owner.
4. EffectKeeper owns actual MultiEmitter objects. Each MultiEmitter owns its callback, particle callback and SingleEmitter collection; SingleEmitter links ParticleEmitter, whose mEmitter is an actual JPABaseEmitter. MarioEffect's callback compares these emitter identities, reads lifetime/status/particle lists and changes transforms. Scalar IDs or facade pointers cannot replace these objects.
5. LiveActor movement updates its actual keeper; clipping and death call the original keeper transitions. Scene retirement removes emitter callbacks and references before model matrices, actor state and allocation heaps are released. Existing OriginalEffectBckNotification overlaps EffectKeeper::changeBck and must be retired when the full original keeper TU is selected.

The native EffectService already parses resources, simulates JpcEffectEmitterInstance objects and draws them. It is more than an event logger, but its private native instances are not JPABaseEmitter or MultiEmitter objects. A general original JPA backend integration is required; returning those facade instances from an original Game pointer API would be invalid.

`effect-activation/direct-undefined.txt` records only undefined symbols from the three directly inspected compiled units, not a whole-project unresolved dump. The immediate original subsystem gaps include:

| Required source area | Present root code | Missing original implementation required for ownership |
| --- | --- | --- |
| EffectKeeper | Most methods compile | updateFloorCode(), initAfterPlacementForAttributeEffect, onDraw/offDraw |
| MultiEmitter | Constructors, many control/access methods | allocateEmitter, BCK sync initialization/control and scan methods |
| EffectSystem | delete/create-single helpers | constructor at 0x800C54FC (0xB8), createEmitter at 0x800C55B8 (0xAC), entry at 0x800C584C (0xC0) |
| Resource and auto effects | Headers | ParticleResourceHolder, AutoEffectInfo, AutoEffectGroup/holder and EffectSystemUtil source bodies |
| Emitter runtime | SingleEmitter and partial ParticleEmitter | ParticleEmitterHolder constructor/body closure, original JPA manager/resource/emitter lifecycle |

The full original EffectUtil header changes MR::emitEffect's return from the native facade's void to MultiEmitter*. Both have the same C++ mangled symbol. Publishing this header while retaining LiveActorUtilCompat's void implementation could link successfully and return garbage. Retire that native facade provider together with the real original helper/owner closure. The original EffectUtil.cpp already has complete LiveActor helper bodies, including emitEffect, callback emission, getEffect, rename, SRT and deletion; use those complete methods when the owners exist.

## Text encoding boundary before effect predicates

Native source literals are UTF-8; the original compiler wrapper emits Shift-JIS source literals. The existing general `resource/TextEncoding.hpp` contract decodes Nintendo CP932 resources into UTF-8 at the native resource boundary, but that conversion is currently applied by selected consumers, not universally to every JMap/effect string.

MarioActor::isCommonEffect checks the raw Shift-JIS prefix 8B A4 (共), and isMaterialEffect checks 91 AE (属). Native literals beginning 共通 or 属性 consequently fail these raw-byte predicates. Those methods were deliberately not patched here.

The coherent next step is to define and enforce the general Game-text contract for both source literals and decoded resource strings, including hash inputs, and provide an encoded-prefix comparison boundary shared by such byte predicates. With the existing UTF-8 contract, comparisons should express the logical source prefix and the resource boundary should decode exactly once. Keeping original encoded Game strings instead would require a general build/source literal encoding boundary and changes to the existing UTF-8 resource consumers. Do not add only a Mario UTF-8 byte alternative: it would leave resource/hash/prefix consistency unresolved. Also audit the original common-effect `name + materialIndex` alias keys when choosing the contract; those byte offsets are used on both registration and lookup paths and must remain consistent.

No native effect owner, active emitter graph, or full Mario gameplay success is claimed by this package.

## Integration checkpoint

The bounded payload patch is now applied to native production sources and rebuilt with LLVM23 for arm64 macOS. The original model and camera File Select fixture also passes with the widened table payloads. Full original effect ownership and playable Mario remain separate unfinished integration work.
