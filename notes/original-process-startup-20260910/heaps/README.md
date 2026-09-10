# Original stationed loader and process heap prerequisites

The complete existing reference `GameSystemStationedArchiveLoader`, embedded `PlayerHeapHolder` and conditions, `HeapMemoryWatcher`, and `StationedArchiveLoader` are now native sources. Original loading nerves, actual async request completion, suspend/reset gates, per-player heap disposal, table filters, resource creation, and finalization order remain in Game. The former `OriginalScenarioPreload.cpp` extraction is removed because the full original StationedArchiveLoader now owns that symbol.

This is a compiled-source prerequisite checkpoint, **not a completed original process startup**. The parent owns the one combined build and runtime check. No compiler, Xmake, or commit was run by this agent; no new Wii-match score is claimed for these existing source imports.

## Native-only source and ABI deltas

- `GameSystemStationedArchiveLoader.cpp`: both `heap->alloc(0x10000, nullptr)` arguments become integer `0`. The reference's MW `nullptr` spelling represented the zero integer alignment; modern C++ rejects `nullptr_t` there. No branch or size changes.
- `HeapMemoryWatcher.cpp`: MEM2 pointer arithmetic uses `uintptr_t`, including subtraction before the bounded u32 heap-size argument. The WPad heap's fixed 208-byte budget adds the difference between the native rounded `JKRExpHeap` header and its rounded Wii 0xA0 header. This preserves usable space with native object size even when the SDK needs zero Bluetooth work bytes.
- `StationedArchiveLoader.hpp`: the unimplemented base `Condition::isExecute` is declared pure virtual. Every concrete original condition implements it; retail has no callable base implementation. This supplies the native abstract-vtable contract without inventing a base result.
- Native JKR ARAM address accessors/store use `uintptr_t` so the original reserved MEM2 prefix is not truncated to 32 bits.

The original `trySuspend` assignment `_C = true` was checked directly: retail 8039DB6C–8039DB8C does the same. It was not changed based on appearance.

## Boot ownership and allocation domains

`JkrHeapRuntime::prepare_mem2_arena(byte_budget)` retains a separate aligned real native MEM2 arena. Its budget includes the original 14MiB ARAM prefix. Aurora's `bind_mem2_arena`/`unbind_mem2_arena` only borrow that storage and expose bounded OS MEM2 watermarks. `JKRExpHeap::createRoot` binds the already owned actual native expandable root and fails if no valid root exists; it never creates an orphaned second root.

Before actual original boot, the process owner must prepare MEM2, then call original `HeapMemoryWatcher::createRootHeap` and construct the watcher exactly once. All of its original system/audio/stationed/WPad/home-button/game children are real JKR heaps. The process owner must reclaim them, including the external MEM2 GDDR root, before releasing JkrHeapRuntime. The API deliberately does not allocate a partial watcher or publish a fake singleton.

The original `createGameHeap(-1)` consumes remaining root space. Later native domains can use `JkrAllocationDomain::retain_heap(runtime, actualHeap, actualHeapOwner)` and `create(retainedParentDomain, byte_budget)`; children retain their real parent. `retain_heap(parentDomain, actualChildHeap)` binds an existing child, validates ancestry, and retains that owner's lifetime. Existing root-based callers are unchanged until actual boot integration selects the new APIs.

`current_jkr_allocation_domain` resolves the closest registered ancestor and recognizes actual SDK-worker guest routing, including host allocation escapes through `callbackGuest`. Its lookup takes the original heap mutex only briefly. **Do not hold `JkrAllocationScope`/`CurrentHeapRestorer` over an OS wait for another task that needs the same mutex.** The process entry uses `GuestThreadExecutionScope` with its actual heap already selected; root/audio own that integration. Actual owner destruction still must happen after resources and async workers have stopped.

## JKRUnitHeap SDK support

`JKRUnitHeapCompat.cpp` implements the actual bitmap heap with native header/alignment, head/tail search, first/best fit, free-space checks and snapshot checksum. `calcHeapSize(unitSize, unitCount, alignment)` lets the async executor preserve its 256 original records with native-sized message queues and pointers.

Evidence is retail `asm/JSystem/JKernel/JKRUnitHeap.s` plus the Game SDK overrides in `asm/Game/System/Overwrite.s`: create at8040D7FC, bitmap/index helpers8040DABC onward; Galaxy do_alloc803A41E0 and do_free803A432C. The occupancy bitmap remains MSB-first independently of host endian. Multi-unit allocation reserves all units; original free releases only the addressed unit. Free-tail/fill/resize and the zero-count max-block-query behavior retain their original contracts. This is a generalized native SDK implementation, not a newly recovered Game algorithm or a claimed Wii fuzzy match.

`WPADGetWorkMemorySize` reports zero because the native KB/M implementation does not allocate a Bluetooth/WUD work buffer in the Game allocator. It does not report fictional Wii hardware allocations.

## Shared resource boundary

The actual ResourceHolderManager class is constructible. Its model methods delegate to the existing ResourceHolderService and actual ResourceHolder/J3D owner graph; there is no second native model registry. Stationed methods require the actual mounted archive and its requested heap. ResourceHolderService can retain an original child heap and construct all model/animation resources within it. Heap eviction rejects still-borrowed ResourceArchiveOwners before any removal, then reclaims those actual resources. FileUtil removes models first, mounts and decompressed copies next, then raw loaded-file buffers associated with that heap. Raw loaded-file publication/queries/eviction are mutex protected and repeated loads retain published byte identity.

`ResourceHolderManager` is a native service bridge in this checkpoint, not a claim that its complete original implementation is active. The original manager depends on largely undecompiled `LayoutHolder` (resource tables, mount, GetResource/GetFont and related accessors) and has a TODO raw-layout construction routine. All three native layout-holder creation entry points therefore fail explicitly. The actual original stationed loader will stop at that missing capability rather than silently omit type5 records or claim completion.

## Original utility providers and remaining closure

`OriginalStationedArchiveUtil.cpp` supplies exact original GameSystemFunction request/isEnd and MR request/wait wrappers against `SingletonHolder<GameSystem>->mStationedArchiveLoader`, plus original heap watcher accessors/adjust. No synthetic sequence or archive-completion state is introduced. `getSceneHeapGDDR3` prefers the actual watcher once present; the existing retained scene-cohort path remains for the current bounded showcase until actual process startup is activated.

Known remaining process dependencies owned by the coordinator or later work: actual GameSystem singleton/children; audio initialization readiness; actual LayoutHolder; FileLoader request bookkeeping (`MR::clearFileLoaderRequestFileInfo` has only an excluded original provider); process graph destruction; coherent system/scene heap selection across async waits. No actual process owner was created by this agent. Current working scene/demo behavior is assessed by the parent's combined smoke, not by this source audit.

`source-manifest.json` records this source freeze. The Aurora OS umbrella header is shared with the async agent's OSFastCast include; coordinator final publication may refresh its hash.
