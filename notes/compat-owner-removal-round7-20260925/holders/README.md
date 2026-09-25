# Actual resource and layout holder ownership

ResourceHolder and LayoutHolder now own the native resource backing and destruction previously held by ResourceArchiveOwner and LayoutArchiveOwner. ResourceHolderCompat.cpp/.hpp are deleted after all production consumers migrated. No replacement resource registry or service was introduced. Existing original constructors, enumeration, table naming and lookup methods remain their actual entry points.

## Ownership and initialization

Each constructor retains the actual JKR archive borrow token, bounded parsed source, and its selected original heap domain before calling original initializeArc. ResourceHolder prepares the complete existing animation/model/BAS/BTI/CANM decoding closure first. ArchiveHolder owns bounded JMap/JPC registrations. Native host containers remain under Aurora HostAllocationScope; original Game allocations run in their actual JKR domain. The existing J3dCommandScope and exact recursive load-mutex exception recovery surround synchronous original initialization. No asynchronous wait is introduced inside those scopes.

The native ResourceHolder destructor retires loaded models before their borrowed MaterialAnmBuffer. It then frees material animations/diff flags, BckCtrl/control rows, effect-matrix backups, and all original resource tables and allocated names. LayoutHolder retires its original tables before releasing the archive/heap backing. Constructor failure invokes the same cleanup. Archive finders now have scoped unique ownership so a loader exception cannot leak the active enumerator.

Borrowers retain an opaque token from the actual holder. The original manager must call ensureNativeResourcesUnborrowed across every affected entry before deleting any, and retains the actual heap through delete. ArchiveHolder separately retains foreign heap domains until its entries retire. This keeps the holder-before-archive-before-heap order without a parallel registry.

## Original duplicate-holder semantics

The original manager's stationed/raw creation paths may create multiple holders for one archive; the retired service hid this by deduplicating. Two boundaries handle those original requests correctly:

- Each ResourceHolder owns independent J3D model/animation source copies and loaded objects. A local raw-source-to-native-input mapping in createAndRegisterObject supplies the holder's own registered SDK source to the existing original loader calls. BAS similarly owns an independent bounded source and actual native JAUSoundAnimation. Archive raw identities, sizes and IDs recorded by mount into ResFileInfo::_8/_4/_C remain unchanged. No competing archive-global J3D/BAS aliases remain.
- Actual JKRArchive override leases publish converted BTI/CANM records. The SDK restores the next live cached record or the original raw cache state when a lease retires, regardless of FIFO/reverse order. A later unrelated cache write is preserved. Each override retains one archive token, included by nativeArchiveReferenceCount in the manager's combined all-owners preflight. Handles clear before converted bytes or the archive token.

## Native public boundary

Both holders expose retainNativeResources() -> shared_ptr<const void>, ensureNativeResourcesUnborrowed(), nativeArchiveReferenceCount(), nativeResourceSource() -> const RarcArchive&, nativeResourcePath() -> const filesystem::path&, and heap() -> JKRHeap&. ResourceHolder additionally exposes retainNativeTexture(ResTIMG*) for independently retained converted BTI backing. The earlier LayoutHolder texture factory was removed after the review below.

Diagnostic archive paths resolve through the actual FileLoader ArchiveHolder entry by archive identity. A direct standalone JKR archive has only its actual internal loader name available. Texture copies retain their encoded mapped backing independently from the TexMap object, as before.

## Focused validation

OriginalResourceHolderTests now constructs actual ArchiveHolderArchiveEntry and ResourceHolder objects for synthetic bounded resources, and uses the actual process singleton managers/FileLoader for layout and retail assets. Every test executes inside one OriginalStageResourceProcessFixture callback. A bounded 4 MiB reclaiming child of the real scene heap supplies 1 MiB temporary test cohorts; there is no second OS allocator/runtime initialization or production budget increase.

The original fixture data generators and SDK texture checks remain unchanged. Existing checks still cover typed key/full animation, original BckCtrl rows, nested tables, zero-length BCK, exact BTI metadata/GX payload, TPL palette/LOD rules, CSV conversions, load failure mutex/GD/current heap restoration, real Mario/material animation and independent effect-matrix backups. New checks cover independent duplicate J3D/BAS state, BTI/CANM FIFO/reverse retirement, preservation of unrelated cache writes, duplicate actual stationed manager creation, and independent duplicate real model/material buffers.

Service-era tests that allowed removing original fixed bytes while borrowed were replaced with original holder/archive preflight rejection and unchanged identities after rejection, followed by retirement in the correct order. Attached JMap parser leases separately block fixed archive retirement until the parser is released. The layout font test now compares with the real original process font owner rather than expecting the former absent process error.

61 source checks pass in source-validation.json. This agent ran no build and made no commit/index changes. Root owns build/runtime verification. An initial root test attempt exposed that standalone GameResourceRuntime followed by actual process startup would replace an initialized OS allocator; the fixture was corrected to the single actual process described above. Current runtime results must be read from root's round7 validation logs, not inferred from these source checks.

### Actual-process thread probe wait

The existing built holder test hung after placement. A fresh run was attached with LLDB as PID 52461; `test-hang.txt` records the main thread in `test_failure_scope` at `std::thread::join()` and the probe thread in `OSTryLockMutex` -> `OSDisableInterrupts` -> Aurora `acquire_cpu`. The actual-process callback owns the guest CPU, so a bare native join cannot wait for an SDK worker. The test now uses the existing `aurora::os::GuestThreadWaitScope` only around `join()`, preserving the cross-thread load-mutex acquisition assertion and restoring callback CPU ownership afterward. No production code changed. LLDB detached and only the verified test PID was terminated. `before-hang-fix.cpp` and `hang-fixture-fix.patch` preserve the exact fixture delta; the parent will rebuild and rerun.

### Synthetic model test arena

The parent retry passed the factory, layout and manager lifetime cohorts, then reached the real Mario material fixture. The synthetic mixed Model.bdl/Color.bpk holder had incorrectly selected the already-sized retail Mario archive heap, which cannot accommodate another holder. It now uses a bounded 2 MiB child of the existing 4 MiB test arena. Production heap sizes and retail resources are unchanged. The shared-heap identity assertion compares the two actual synthetic duplicate holders; a separate assertion confirms their heap differs from the retail original. All model/material/name/diff/controller/effect-matrix and final MEM1 reclamation assertions remain. `before-model-arena-fixture.cpp` and `model-arena-fixture-fix.patch` isolate the correction.
