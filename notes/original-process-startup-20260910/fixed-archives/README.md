# Fixed-memory archives and the shared SDK volume list

`JKRMemArchive` now supports the original default constructor and `mountFixed(void*, JKRMemBreakFlag)`. It also provides an explicitly bounded span overload for native callers. These are genuine archive mounts: metadata is parsed from the supplied decompressed RARC bytes, native directory/file records are published only after successful validation, and resource pointers retain the caller's original buffer identity.

`RarcArchive::from_borrowed(span)` parses without copying resource bytes. Owned `from_bytes/from_file` archives retain their existing owning vector behavior. The selected byte span is derived at access time: ordinary copies of owning archives point into their own copied vectors; borrowed copies/moves continue borrowing the original external span. The caller must keep a borrowed buffer alive for the archive's lifetime. Invalid/compressed input is rejected rather than silently copied or reported mounted.

## Real SDK lifecycle

The archive now derives from the real native `JKRFileLoader` base, itself a `JKRDisposer`. This makes Game-heap archive objects participate in original disposer teardown and destroys their host metadata when a heap retires. A separate synthetic mount registry was not introduced.

The shared actual `JSUList<JKRFileLoader>` supports prepend/remove, global getGlbResource lookup, mounted state, current loader selection, original reference-counted unmount, and native pointer-width mount identities. Both original source spellings `sFileLoaderList` and `sVolumeList` refer to this one list. initializeVolumeList initializes its actual original OSMutex once; it does not erase existing mounted native archives when original GameSystemObjHolder later initializes DVD support. Mutations and lookup are protected by that recursive SDK mutex.

The supported virtual lookup surface uses the existing native const API and u32 resource sizes. This is SDK adaptation outside Game; no Game bodies were modified.

## Ownership and duplicate mounts

- Flag0 borrows the original supplied bytes. Destruction removes the volume and releases parsed metadata but leaves those bytes untouched.
- Flag1 transfers the supplied actual JKR buffer allocation on successful mount. Destruction frees it with JKRHeap::free and the buffer's owning heap. A flag1 buffer outside all actual JKR heaps is rejected explicitly; the native layer does not pretend to own/free a static or arbitrary host buffer.
- Duplicate buffer identities are found through the shared mounted-volume list. The existing loader's `_34` reference count increments, then mountFixed returns false without mounting or taking ownership on the new object. This unusual side effect is the exact retail behavior: JKRArchivePub.s80410DC8–80410E18, especially80410DF4–80410E00, followed by JKRMemArchive.cpp::mountFixed. Native remount of an already-mounted object using a different buffer is rejected rather than corrupting its intrusive list membership.
- The raw pointer SDK API has no byte-count argument. As on Wii, its caller supplies a valid RARC header and the full header-declared extent. The native span overload validates that declared extent against its actual supplied span. No claim is made that arbitrary invalid host pointers can be validated by the raw API.

Default/unmounted archive queries return no resources and zero counts; failed parsing does not partially publish directory pointers. Existing native archive constructors also publish their actual volumes and unregister on destruction. Native retained archive owners still own the loader objects they expose as borrowed pointers; callers must respect that existing owner contract rather than independently deleting them.

## Original references and validation

Reference sources inspected: `decomp/src/JSystem/JKernel/JKRMemArchive.cpp`, `decomp/src/JSystem/JKernel/JKRFileLoader.cpp`, SDK headers, and `notes/gateway-audit-20260907/restoration/retail/asm/JSystem/JKernel/JKRArchivePub.s` for duplicate reference-count behavior. GameSystemFontHolder uses flag0 for the decompressed embedded font buffer and deletes that buffer only after deleting its archive, so this path now retains the required byte identity and lifetime.

No isolated compile, test run, or commit was performed per coordinator instructions. The parent owns the one combined native build and any runtime verification. This does not claim complete original FileLoader process startup or completed original GameSystem initialization.

## Embedded archive registration and resource ownership

The previously empty `MR::createAndAddArchive` now publishes a real fixed-buffer mount through the existing ArchiveMountService. Its source borrows the original caller-owned decompressed RARC bytes, preserves resource pointer identity, and participates in the same named mount/resource registration and heap eviction path as disc archives. The pointer-only Game API trusts the caller-supplied RARC length, just like the original SDK; the shared service accepts a bounded span.

`MR::decompressFileFromArchive` now decodes Yaz0 through the existing general decoder and allocates the result on the requested/current JKR heap with the original signed alignment. The old private vector copy cache and its helper were removed. Callers can use normal deletion/heap retirement and pass the result to fixed archive mounting. Yay0 remains explicitly unsupported instead of returning compressed bytes as successful output. No new Game edits or standalone test run.
