# Actual process particle-resource ownership

Frozen isolated proposal. No RuntimeContext, shared xmake, production Game/SDK source, EffectSystem activation, or legacy effect path was changed. The patch adds four native files and passes `git apply --check`.

## Owner and lifetime

`runtime/ParticleResourceOwnership` owns the unchanged, actual `ParticleResourceHolder` in an actual child `JKRSolidHeap`. It uses the original `/ParticleData/Effect.arc` path from `GameSystemObjHolder::initAfterStationedResourceLoaded`. The supplied ArchiveMountService must already be active and outlive this process owner, following ScenarioCatalogOwnership's lifecycle.

Construction first mounts and retains the actual archive, propagating archive errors and checking the three required resources before the original constructor remounts the same identity. The unchanged holder constructor allocates its actual JMapInfo objects, JPAResourceManager, original resources, and particle group records under the retained domain. Publication occurs only after the complete constructor succeeds. Another published owner is rejected before mounting or allocating a domain. Lifecycle installation/removal is serialized by the process caller, as with the existing scenario owner; the returned Game pointer is borrowed.

`compat/OriginalParticleResourceLookup.cpp` provides the sole native `MR::getParticleResourceHolder`: it returns the published owner's actual holder, or throws a construction-order error. It does not fabricate GameSystem/GameSystemObjHolder or use a NameObj registry. The existing root SystemUtil source remains untouched and unselected for this native entrypoint.

Teardown unpublishes first, deletes the holder's trivial outer object, removes mounts tagged with this owner's own heap, and retires the domain while retaining the MountedArchive. Actual `JKRHeap::dispose` invokes the JMapInfo disposer list before the JPA manager finalizer, which destroys its native decoded resource lease. The archive lease is released last. Partial constructor failure uses this same Storage teardown; the incomplete outer holder is not published. Existing mounts retain their first identity/heap tag and publication, and are borrowed rather than retagged or removed. The owner also keeps its archive alive if mount publication is removed before holder retirement.

Scene emitter/effect owners must retain this process owner until all their actual JPA managers and emitters have retired. The archive service and root heap service must be torn down afterward. This proposal does not own or activate a scene EffectSystem.

## Measured budget

Actual supplied disc, current macOS ARM64 ABI:

| Item | Bytes |
| --- | ---: |
| Original holder construction allocations | 1,692,720 |
| Native JKRSolidHeap header allocation | 240 |
| Exact successful domain budget | 1,692,960 |
| Remaining at exact minimum | 0 |
| Largest failing aligned budget tested | 1,692,928 |
| Approved default budget | 2,097,152 |
| Remaining at default | 404,192 |

The 240-byte heap allocation header is the SDK-aligned size of the 232-byte native class. The immediately smaller 32-byte-aligned budget fails on a late 16-byte Particle allocation, after the JPA manager and both JMap tables have been constructed. A 10,240-byte budget fails earlier on the 26,616-byte resource-pointer allocation. Both are expected `std::bad_alloc` cases, and both restore root heap free space.

`default_byte_budget` exposes the parent's approved 2 MiB choice. Construction still takes an explicit budget, preserving the ability to verify the exact measured minimum. These figures cover Game/SDK heap allocation; retained RARC bytes and decoded host-order JPC/JMap backing use the existing host resource layer and are not included in this child-heap budget. They do not claim a total process memory footprint or a portable minimum for another ABI/disc.

## Resource queries and source proof

`compat/OriginalParticleResourceQueries.cpp` contains four literal complete functions from root EffectSystemUtil:

- `MR::Effect::isExistInResource(u16*, const char*)`;
- `MR::Effect::isExistInResource(u16*, const char*, s32)`;
- `MR::Effect::getAutoEffectNum(const char*)`;
- `MR::Effect::getAutoEffectListBinary()`.

The native lookup is the owner boundary; these four Game query bodies remain unchanged. Their original-compiler instruction streams match retail after relocation normalization. Three score 100% in objdiff; the numbered overload scores 99.545% because its isolated format-string relocation differs. The existing literal root getGameSystemObjHolder/getParticleResourceHolder pair both score 100%.

The complete already-selected ParticleResourceHolder was also compiled unchanged: four methods are 100%, constructor 99.8667%, getUserIndex 97.5890%, and countAutoEffectNum 90.6596%. These are existing decompilation results, not a claim that every holder instruction matches retail. No root recovery or mutation was needed in this task. `root-evidence.json` records normalized source comparisons, and `dol-evidence.json` checks all 13 relevant retail text functions against DOL SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`.

## Actual-disc CPU verification

`verify-native.py` compiles the three new native provider TUs, complete original ParticleResourceHolder, and the fixture with LLVM 23 ASan/UBSan, then links existing prebuilt native libraries. Those prebuilt libraries are not rebuilt or newly instrumented by this task. The real disc is opened through Aurora DVD and the existing DvdFileSystemService/ArchiveMountService; no fake resource, emitter, archive, or Game provider is used.

The final fixture passes:

- fresh owner construction/destruction and reinitialization at both exact minimum and approved default budgets;
- all 3,327 name-to-user-ID queries and actual JPA resource identities, 225 textures, and 2,591 auto-effect rows;
- all four literal query wrappers, including a real numbered resource and missing-name output preservation;
- independent raw BCSV group counts: 614 case-sensitive groups merge into the original holder's 612 case-insensitive groups;
- duplicate publication rejection and missing-owner lookup rejection;
- actual holder, JPA manager, and JMap allocation provenance in the selected child heap;
- owner-retained archive lifetime after its mount publication is removed;
- preservation of a prior archive identity, heap tag, and publication after owner destruction;
- early and late constructor rollback and restoration of root heap free space;
- expiration of weak JPC/JMap backing observers after late failure, proving that the completed JPA manager finalizer and actual JMap disposers did release their retained host state;
- zero NameObj registrations added by the resource holder.

`probe-runtime.log` includes the three deliberately triggered allocator exhaustion diagnostics; these are expected failure cases, not sanitizer findings. The process exits successfully with no ASan/UBSan diagnostics. No emitter simulation or GPU rendering is claimed here.

Reproduce from the repository root:

```
python3 pc-port/notes/original-particle-resource-owner-20260903/verify-root.py
python3 pc-port/notes/original-particle-resource-owner-20260903/verify-native.py
```

The native script selects the supplied sole repository-root RVZ into `SMGPC_REAL_DISC`, stages under ignored build/, and invokes no shared build. `native-compiles.json` and `native-verification.json` contain exact commands and runtime output.

## Integration

`native.patch`, `native/`, and `native-manifest.json` are the frozen production proposal. Apply the patch and select the three new `.cpp` providers; the fourth file is the owner header. The existing ParticleResourceHolder provider stays selected. If full original EffectSystemUtil or SystemUtil is activated later, remove the corresponding extraction provider to avoid duplicate symbols.

```
git apply --check pc-port/notes/original-particle-resource-owner-20260903/native.patch
git apply pc-port/notes/original-particle-resource-owner-20260903/native.patch
```

The parent's isolated callback fixture can use the staged sources immediately:

```
ParticleResourceOwnership resources(
    process.host_heaps(), ParticleResourceOwnership::default_byte_budget, mounts);
```

Include `build/original-particle-resource-owner-20260903/staged` before production include directories and link its three provider sources. Keep `resources` alive throughout actual MultiEmitter/EffectSystem ownership. RuntimeContext publication/teardown ordering, full original scene effect construction, and retirement of legacy effects remain the parent's atomic integration work.
