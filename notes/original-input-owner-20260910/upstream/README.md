# Upstream JKR alignment — 2026-09-10

Audited exactly the six files changed between upstream `73f5b40dc` and `d1ae0a05cc023d52ecdcbc7731c8c79f0cb84dc6`, including preceding `560585473` (`JKRDvdAramRipper`). Parent owns the decomp merge. The full upstream delta and per-file decisions are saved as `upstream-six-files.patch` and `native-dispositions.json`.

## Applied native alignment

After the parent authorized implementation, changed only `src/JSystem/JKernel/JKRHeap.hpp` and `src/compat/JKRHeapCompat.cpp`:

- Mirrored `NO_INLINE` on instance `alloc(u32,int)` and static `free(void*,JKRHeap*)`.
- Replaced the invented class-static `JKRHeap::ARALT_AramStartAddr` with the actual global `ARALT_AramStartAdr`, keeping the original logical 32-bit ARAM address and initial 0x90000000 value. Original getters/setters now use that storage. No old alias remains. Before editing, native source search found only the declaration, definition, and these accessor uses.
- Adopted upstream's disposer iteration: start at the first link, retain the last surviving link, resume from that predecessor after a destructor removes itself. Native `uintptr_t` range parameters and native `finalize_jkr_heap_objects(this,start,end)` remain intact. The old and new loops have equivalent traversal semantics because `JSUList::getEnd()` is null and the iterator's default link is null.

Native already had the newly recovered default `byte_806B26D8 = true`, so no behavior change was needed there. Static declaration order and removed comments are Wii matching details. The native panic provider, recursive allocator lock, allocation provenance, parentless-root lifetime, finalization and global-new routing remain owned by their existing native systems. Replacing the whole native heap implementation with the reference TU would erase those necessary platform boundaries.

## Other upstream changes

Wii split/symbol/configuration changes remain reference-only. The two symbol-name corrections already agree with native names. `-ipa file` is a Metrowerks code generation option.

No native `JKRDvdAramRipper` source, header, or provider exists. Its recovered path is an original asynchronous DVD-to-ARAM graph with commands, ARAM blocks, buffered DVD retries, DMA flushes, and Yaz0 history. The active native archive path instead runs `ArchiveMountService -> RarcArchive::from_bytes -> decompress_yaz0`. The native decoder already reads the Yaz0 size explicitly big-endian and uses native-size source/output offsets. The upstream helper replacement in `callCommand_Async` preserves the existing no-ASR compression rule and reads its initial size with byte shifts; the later streaming decoder still uses a Wii-native `u32` header read and logical ARAM addresses. That complete owner graph needs its own future integration, not a raw source replacement into the active native decoder.

## Evidence and limits

Full native LLVM23 JKRHeapCompat translation-unit compile exits 0 (`native-compile.json`, `native-compile.log`). `native-alignment.patch` captures the exact two-file change; `source-manifest.json` records final hashes. No shared Xmake, merges, commits, or runtime tests were performed here. Parent can use existing `OriginalJkrHeapTests` range-disposal/lifetime coverage and `JkrHeapFinalizerTests` for linked validation; the actual range fixture verifies disposal of only a middle object and complete cleanup of the remaining links.

Production sources are frozen; only notes were saved after the compile/freeze notification.
