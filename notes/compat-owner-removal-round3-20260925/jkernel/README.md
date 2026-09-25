# Canonical JKernel heap ownership, 2026-09-25

Baseline: `8490a01dfe1ca84d5348059c6133d3eb6cbcd75e5`. Ten compatibility files removed. Four complete existing heap implementations now live in their canonical JSystem/JKernel owners. This agent ran source checks only; no build, runtime test, index operation, xmake edit or commit.

## Result

| Retired provider | Canonical owner |
| --- | --- |
| JKRHeapCompat.cpp | JSystem/JKernel/JKRHeap.cpp |
| JKRExpHeapCompat.cpp | JSystem/JKernel/JKRExpHeap.cpp |
| JKRSolidHeapCompat.cpp | JSystem/JKernel/JKRSolidHeap.cpp |
| JKRUnitHeapCompat.cpp | JSystem/JKernel/JKRUnitHeap.cpp |
| JkrAllocationProvenance.cpp/.hpp | JKRHeap allocation/free/bulk retirement and native releaseAllocation entrypoint |
| JkrHeapFinalizer.cpp/.hpp | JKRHeap registerFinalizer/unregisterFinalizer and private heap/range retirement |
| JkrDiagnostics.cpp/.hpp | Existing SDK OSReport/OSPanic calls directly; no replacement wrapper |

The four canonical heap implementation files have no compatibility or Game includes. The general native metadata is integrated into the actual base-heap lifecycle, not moved to another sidecar directory. SDK-facing finalizer consumers in JPAResourceManager, JutTexture, the disabled audio wrapper and the finalizer test now call JKRHeap directly. The three existing JKernel ARAM users also call OSPanic directly.

## Native adaptations retained and audited

The original decomp references are `decomp/src/JSystem/JKernel/JKR{Heap,ExpHeap,SolidHeap,UnitHeap}.cpp` and Unit allocation/free plus Solid individual-free overrides in `decomp/src/Game/System/Overwrite.cpp:60`. Existing port behavior was retained where native architecture/lifetime differs:

- Pointer arithmetic and masks remain pointer width, and ExpHeap block headers keep native pointer alignment. Heap creation still reserves actual native class/header sizes. The original head/tail block search, splitting, coalescing, resize and group accounting remain intact.
- SolidHeap retains native-width alignment masks and pointer-sized allocation rounding. Individual free stays the original no-op; bulk/tail reclamation remains original ownership behavior.
- UnitHeap preserves the original MSB-first occupancy bitmap, head/tail and first/best-fit scan behavior, zero-count longest-run query, and Galaxy's one-unit free override. Native bitmap scanning is byte-order independent; calcHeapSize includes the actual native header, bitmap and alignment. No caller-specific allocation policy was added.
- Native allocation provenance remains malloc-backed and independent from ordinary global new. Allocation and bulk retirement keep the original heap mutex while changing provenance; global delete consumes its exact record before calling the heap. Registration failure frees the allocation before throwing. Native free/tail/all/destructor hook order is unchanged.
- Finalizers still run after original JKRDisposers and before heap/range reuse. Each record is removed before invoking its callback; the registry lock is released across callbacks and GX drains. Callbacks may unregister sibling records without stale iterator use. Host/stack resources remain unregistered. All metadata and synchronization remain independent from Game heap storage.
- Root creation still binds the explicitly retained native root. Root release still permits a parentless native arena after descendants retire. No native arena budget, MEM2 reserve, stage policy or Game owner moved into JKernel.

The retained `JkrAllocationDomain` continues to own caller-sized host arenas and retained native heap lifetimes; that higher-level service is deliberately not renamed here. Its existing MEM2/reserve policy remains outside JKernel for a later focused cleanup. Wii boot initArena/initArena2 and other previously absent methods were not fabricated.

## Shared current-heap synchronization

`JKRHeap::sCurrentHeapMutex` now owns the same recursive synchronization identity used by host allocation domains and original Game/J3D heap selection. `MR::MutexHolder<1>` is a reference alias to that SDK storage, preserving all existing GameSystem/app boot initializers and J3D lock ordering without duplicate locks. Other MutexHolder specializations are unchanged.

`JKRHeap::CurrentHeapScope` locks before selecting/snapshotting the current heap and restores before unlocking. JkrAllocationDomain uses this SDK scope and no longer includes or calls Game/Util/MemoryUtil or MutexHolder. Its previous-domain retention, host/guest routing, teardown ownership and rollback ordering are unchanged. The three MemoryUtil lock sites use the SDK mutex; MR::CurrentHeapRestorer now takes that mutex before reading its previous-heap snapshot, closing the pre-lock read race on native threads.

Existing JkrAllocationDomainTests gained two checks: the MR/J3D alias has the exact SDK mutex address, and a nested SDK scope inside MR::CurrentHeapRestorer allocates from the selected real heap and restores the enclosing heap during exception unwinding. Existing multithread selection, allocator-failure rollback, provenance and disposer-allocation tests remain. FinalizerTests migrated to the actual JKRHeap API without changing its host/explicit/bulk/tail/range/sibling-mutation coverage.

## Static evidence and build handoff

`validate-source.py` and `source-validation.json` compare C++ tokens after known diagnostic/hook renaming: all 129 existing heap method bodies are preserved (37 base, 43 Exp, 21 Solid, 28 Unit), as are all six original provenance/finalizer implementation routines. Added base methods are the scoped selection and ownership APIs. Scoped `git diff --check` passed. No retired header/symbol references remain under src/ or tests/ (the test filename itself is retained).

Root must add these four sources to smg-pc-game, relative to src/Game:

```lua
add_files({"../JSystem/JKernel/JKRHeap.cpp", "../JSystem/JKernel/JKRExpHeap.cpp",
           "../JSystem/JKernel/JKRSolidHeap.cpp", "../JSystem/JKernel/JKRUnitHeap.cpp"})
```

Regenerate the source graph after deleted compatibility glob entries. `build-wiring.json` lists existing focused targets: original JKR heap, finalizer, allocation-domain, J3D command scheduling, ARAM, GameSystem startup and file loader. Build and runtime validation remain pending the root checkpoint.

`before/` and `before.json` save every touched existing file and its pre-edit status/hash. All were clean at capture. `owned-patches/` provides exact per-file modifications, and `source-changes.patch` includes deletions and the four new sources. `audit-update.json` records every removed provider's destination. No unrelated dirty files were overwritten.
