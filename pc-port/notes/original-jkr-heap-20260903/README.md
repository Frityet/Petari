# Original JKR heap allocation domains

This tranche supplies an actual JKRExpHeap root and JKRSolidHeap resource domains so unchanged original constructors and factories can own their ordinary, array, aligned and explicit-heap allocations. It does not activate ResourceHolder or Mario animation ownership by itself.

## Original source recovery and proof

The authoritative source is `src/JSystem/JKernel/JKRExpHeap.cpp`, with the original header in `libs/JSystem/include/JSystem/JKernel/JKRExpHeap.hpp`. Missing allocation, free-list, accounting, inspection, state and destructor bodies were reconstructed root-first. The small game override `adjustSize` at `0x803A43CC` was recovered directly from the current RMGK01 executable and placed with its class implementation. Its header return type was corrected from `void` to `s32`.

A related JSystem source was useful as an initial reference: [zeldaret/tp JKRExpHeap.cpp, pinned dabd6961a674f23ec0df8f2b4d16ba12e1e090e5](https://raw.githubusercontent.com/zeldaret/tp/dabd6961a674f23ec0df8f2b4d16ba12e1e090e5/libs/JSystem/src/JKernel/JKRExpHeap.cpp), under that project's CC0-1.0 license. The downloaded reference and license remain in ignored `build/original-jkr-heap-20260903/`; its URL/hash is recorded in `original-evidence.json`. The reference was not accepted as proof of Galaxy behavior. Four reference-only diagnostic calls were removed after inspecting retail, and the actual Galaxy panic strings/lines and release control flow were retained.

`verify-original.py` compiles the real root TU using GC/3.0a3 and the configured `cflags_jsys`. It compares all 40 ExpHeap object functions plus the game `adjustSize` override to retail. All 41 have the original instruction count and score 96.100914–100% in objdiff. It independently relocates and compares all **1,842 target instruction words** to the verified live DOL, including both `r2` and `r13` SDA bases. The DOL SHA-1 is `25c5959534b3c21246c6c7e42021b916b41fb578`.

The constructor/create/destroy, ordinary head/tail allocation, free/freeAll/freeTail, group access, free-list links, coalescing entry, CMemBlock methods and state methods compile to 100% where listed in the evidence. The remaining differences are:

- Aligned head allocation (96.62745%) and resize (96.100914%): equivalent address arithmetic association changes registers and scheduling; all original branch cases, size comparisons and helper order remain.
- `adjustSize` (98.20225%): register allocation only. The original parent type check, unusual free-list successor skip, zero-size free-block fallback and resize sequence are retained literally.
- Diagnostic-bearing functions (98.625–99.849%): constant symbols/string layout and argument scheduling. Strings and panic line numbers were checked against the live literal pool; no extra reference diagnostics remain.

The compiler also exposed pre-existing root layout/lifecycle errors that a high fuzzy score alone would obscure. Retail accesses allocation mode at `0x6A`, group at `0x6B`, and the external-storage flag at `0x6C`. The first two are the existing base tail bytes, now named in JKRHeap; duplicate derived declarations were removed. Create/destroy now use the real external-storage flag: zero means return storage to the parent, nonzero means only destroy the heap object. The full lifecycle functions now match exactly. Parent-owned Base/Disposer/Solid/List recovery is documented separately in `../original-jkr-base-20260903/`.

## Native SDK boundary

`JKRExpHeapCompat.cpp` supplies the original class methods. `native-exp-architecture.patch` is the complete reviewed difference from root; `native-exp-edits.json` and `verify-source.py` reproduce/check those edits. Changes are limited to native concerns:

- Preserve full pointer width in offsets, alignment masks, address temporaries and the sorted-dump sentinel.
- Use native `sizeof(CMemBlock)`/heap header sizes and the native block alignment (8 here). Pointer-containing free-list headers cannot follow a four-byte payload stride on this host.
- Check allocation failure before native pointer arithmetic.
- Send diagnostics through the shared native JKR diagnostic boundary, with `%p` and correct pointer arguments.
- Omit Wii `createRoot`/boot-arena setup. The explicit host owner below constructs the actual ExpHeap with retained aligned storage and sets its original external-storage flag. It never calls `OSInitAlloc`, changes the Wii arena or consumes the independently reserved MEM1 texture heap.

All original virtual methods are supplied; there are no partial heap objects or substitute virtual tables. The native Base destructor has the separately documented null-parent unlink adaptation because retail keeps its root for process lifetime while a native owner can release its arena. Original no-op methods such as Solid individual free and Exp fillFreeArea remain no-ops because retail establishes that behavior.

`MemoryHeapScopeCompat.cpp` copies five existing original MemoryUtil methods, including CurrentHeapRestorer and the recursive MutexHolder<1> selection calls. `verify-source.py` compares their bodies with root. The MutexHolder header is an exact copy.

## Retained ownership and routing

`JkrHeapRuntime::create(budget)` creates one authoritative original ExpHeap on a 32-byte-aligned host arena. A second active root and budgets outside the original signed-size range are rejected. `JkrAllocationDomain::create(runtime,budget)` creates an actual Solid child and retains its root owner. Original typed destructors must run before the last domain reference releases; remaining original registered disposers and discarded allocations then follow actual Solid destruction and Exp parent reclamation.

`JkrAllocationScope(domain)` retains the selected domain and any registered domain that the original CurrentHeapRestorer will later restore. An outer acquisition of the original Game mutex precedes the original restorer's snapshot, preventing stale cross-thread snapshots. Ordinary new/new[] then use the actual current JKR heap. A direct original CurrentHeapRestorer inside the scope still controls allocation selection. `current_jkr_allocation_domain()` observes the registered domain of that selected heap, including inside a host escape; outside an allocation scope it returns null. It does not invent ownership for an unregistered original heap.

`JkrHostAllocationScope` keeps native vectors, strings, name tables, retained bytes, registries, caches and control blocks off the Game heap. A nested Game scope can re-enter its original domain. The new/default/aligned/nothrow/sized-delete families use one allocator boundary. Explicit JKRHeap arguments always select that actual heap, including external-buffer child heaps. Negative alignment retains the original tail-allocation direction. Native ordinary new uses `__STDCPP_DEFAULT_NEW_ALIGNMENT__` (16 here); `alignof(max_align_t)` is only 8 on this macOS ABI and was caught by UBSan during implementation.

Every native Base `alloc` records its returned pointer and actual heap in malloc-owned intrusive provenance. This includes objects constructed with placement new after a direct heap allocation. Individual/bulk/tail free and heap destruction retire the corresponding records; same-address reallocation replaces a prior record safely. Base holds its actual recursive heap mutex until the original operation and its provenance update are complete. The metadata does not retain a domain, avoiding a self-owning allocation cycle. Registry allocation failure frees through the actual heap before throwing; Solid retains the consumed bytes until bulk release, as its original free contract requires.

Global delete removes provenance under a short native mutex, releases that mutex, then invokes the original owning heap's free. It does not acquire the global Game selection mutex, avoiding the CPU-gate/Game-mutex lock inversion. The provenance mutex is a tiny intentionally retained malloc/placement-new object so global deletion remains usable through process static destruction; record nodes themselves are reclaimed. There is no address truncation, pointer-header probe on arbitrary host memory, MEM1 virtual mapping or display-list sidecar.

Final heap teardown keeps metadata on the host but enables original allocation routing during actual registered disposer destruction. Original heap destruction does not select a new current heap, so any destructor allocations use the existing original selection; the Base destructor may then update it while unlinking. Acquiring a new retained resource in a heap whose final domain reference is already releasing fails visibly. A still-live different current domain remains retainable.

Callers retain heap owners for live objects and do not free/reset a heap concurrently with use of its objects, as required by the original allocation contract. Direct low-level virtual `do_alloc` bypasses the public allocator/provenance boundary; original Game callers use `alloc` (no direct Game `do_alloc` call was found). Wii process-global heap startup is outside this native explicit-owner API.

## Validation

`verify-native.py --sanitizers` runs an isolated executable with actual Base/Exp/Solid/Disposer/List, OS execution/mutex and native allocation providers. It does not use xmake or GPU services. The original MSL printf provider is linked as a separate host object, as required by that boundary.

Seven groups pass normally, under ASan+UBSan with `halt_on_error=1`, and under TSan, with no sanitizer diagnostics:

1. Exp allocation in both directions, full-width alignment, resize, nonsequential free/coalescing, child `adjustSize` and parent reclamation.
2. Original disposer registration, explicit typed destruction outside scope, discarded allocations, over-aligned new, domain/root retention and final release.
3. Nested original heap selectors and Game/host scopes; native metadata survives original heap teardown.
4. Explicit external-buffer heap new, tail alignment, manual placement construction followed by ordinary delete, and bulk reuse.
5. Allocating disposer destruction follows the actual current heap; scopes retain the original heap they must restore.
6. Rejected duplicate roots and allocation exhaustion with restored scope/routing state.
7. Two native threads sharing original current-heap synchronization.

The expected exhaustion test prints one `cannot alloc` message per executable. Final logs and source hashes are in `native-evidence.json` and `native-tests*.log`. An earlier alignment diagnostic was fixed before these final runs. `verify-source.py` checks the exact original scope methods, reviewed native Exp edits, frozen source hashes and all three clean logs.

This worker did not run a shared port build, activate Game resource owners, or claim a full gameplay result. Parent integration supplies the shared build/runtime checkpoint.
