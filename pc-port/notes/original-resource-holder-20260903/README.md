# Original ResourceHolder ownership foundation — 2026-09-03

The global Game `ResourceHolder` already has its original declaration, while
`compat/ResourceHolderCompat.hpp` currently declares an unrelated global class
with an archive pointer and filesystem path. `MR::createAndAddResourceHolder`
currently returns that wrapper. It must not be renamed alone or reinterpreted as
the original table-owning class. The migration remains pending while its actual
constructor dependencies are completed.

## Atomic migration plan

1. Supply the complete native JKR archive catalog and original finder methods.
2. Close real constructor prerequisites: original resource tables, BckCtrl,
   typed animation loading, actual current-heap/allocation ownership, recursive
   OS mutexes, and typed model/material-table loading.
3. In one integration change, rename the archive-only type/service into an
   explicit native namespace, and have `ResourceHolderService` retain actual
   `ResourceHolder` instances constructed through the original constructor.
   Original MR signatures continue to return only that actual class. Existing
   native collision diagnostics obtain their archived bytes/path through an
   explicit service association, without adding archive-wrapper fields to Game.

`ResourceHolder` initializes twelve tables, categorizes every archived file,
loads model and animation objects, optionally constructs MaterialAnmBuffer and
BckCtrl, and snapshots eight effect matrices per material. A holder with only
hand-populated tables or a bypassed constructor does not close this lifecycle.
The parent is implementing the actual model components; loader dispatch must
retain all typed data owners behind the original borrowed-return API.

## Stage 1 completed: JKR archive catalog

The existing flattened RARC view remains available for native consumers.
`RarcArchive::bytes` additionally exposes a read-only view of its retained,
decompressed bytes. `JKRArchive` decodes the actual original RarcInfoBlock,
SDIDirEntry, and SDIFileEntry records into typed native storage. It retains all
file-table records, including dot entries, directory metadata, and empty
folders. Pointer fields are native pointers, not reinterpreted Wii offsets.
The authored root name initializes `mLoaderName` as in original JKRMemArchive.

The catalog validates record extents, directory ranges, name termination and
original name-buffer limits. It preserves file IDs separately from table
indices, flags, file/directory offsets, declared order, and root-relative string
offsets. `getFileAttribute` uses the original table-index contract. This is
important even when authored archives happen to assign matching IDs and indices.

Original `CArcName::store`, `isSameName`, `findIdxResource`, `countFile`,
`getDirEntry`, `countResource`, and `getFileAttribute` bodies are copied into the
native provider. `findDirectory` additionally rejects an invalid directory
index before forming a pointer; root `..` entries can hold 0xffffffff.
`getFirstFile` preserves its source algorithm and currently uses host `new`
instead of the still-unimplemented JKR game-heap placement allocator. This
allocation difference is explicit and will be removed with heap ownership.
The current-directory ID remains the original shared static.

Original JKRFileFinder/JKRArcFinder constructors, traversal, and destructors are
imported as one unchanged source file. The missing base destructor was recovered
first in root `src/JSystem/JKernel/JKRFileFinder.cpp`: retail 0x80277A00, 64 bytes,
contains only the null/positive-destruction-flag checks and the delete call at
0x8040B4A0. All five methods match the verified RMGK01 bytes after relocation.
The other four methods also compare 100% with the configured GC3.0a3 compiler.
No new ResourceHolder object is published by this stage.

`verify-archive.py` compiles with the configured original JSystem flags, uses
dtk/objdiff and verified DOL relocation targets, then independently builds/runs
the native fixture normally and under AddressSanitizer plus UndefinedBehavior-
Sanitizer. Both runs pass **6/6 groups**, with no sanitizer diagnostics:
full catalog/root name/counts; original finder order and folder flags;
non-index file IDs and byte identity; relative/absolute directory lookup;
retained moved-archive lifetime; and bounded declared string extents.

Run:

```sh
python3 pc-port/notes/original-resource-holder-20260903/verify-archive.py
```

The raw DOL and compiled objects remain under ignored build directories. JSON
and logs record actual source/binary hashes and verification, without claiming
that ResourceHolder construction or original Mario jumping is activated.

## Stage 2 completed prerequisites: BckCtrl and borrowed JMap names

The missing BckCtrl methods are now recovered in root and imported unchanged.
`bck-review.md` records the actual compiler comparison percentages, reviewed
instruction differences, setting order, and remaining runtime validation scope.
The native BckCtrl fixture is ready for the coordinated parent build; it has
not been counted as a passing runtime test here.

The original BckCtrl constructor obtains names from a local JMapInfo reader and
retains the returned pointers. The native reader previously stored its strings
in a per-reader cache, which freed those names when the local reader died even
though JMapResource still owned the table. Strings now belong to the shared
table metadata. Each entry is inserted once and never replaced on repeated
reads or attaches. A mutex serializes lazy cache insertion by concurrent readers.
The character pointers therefore remain valid for the retained table lifetime.

`OriginalJMapResourceTests.cpp` independently checks a long borrowed name and an
inline name after local-reader destruction, an attached reader outliving its
JMapResource, rebinding without clearing the old resource's strings, and six
concurrent readers sharing one stable pointer. All **4/4 groups pass** normally,
under AddressSanitizer plus UndefinedBehaviorSanitizer, and under ThreadSanitizer.
`verify-bck-jmap.py` reproduces these runs and records binary/source hashes.

## Remaining source-backed boundaries

Root ResourceHolder count/mount format a null root path through `%s`. Retail
count at 0x803A7920 confirms the null argument is passed unchanged. MSL printf
at `src/MSL_C/printf.c:1123` explicitly converts null strings to empty strings;
host libc does not promise that behavior. The native import needs that library
semantics preserved so nested directories resolve correctly.

Current native JKRHeap is empty. Heap-taking aligned new ignores its heap
argument, and normal new/delete use the host runtime. Simply adding a current
heap pointer cannot provide real allocation or bulk-lifetime semantics; this
requires a coordinated owner/allocator boundary, including normal deletion and
exception cleanup. It is not part of the completed archive result.

Original model loading derives material difference identities from pointer
values and reserves high bits for flags. Native pointers must not be truncated.
Aurora currently configures 24 MiB MEM1 and maps only that region through its
physical-address API. Forcing all widened native J3D allocations there would
introduce that finite capacity and require complete allocator integration.
An explicit SDK allocation-identity boundary can instead reserve stable,
16-byte-aligned original-width virtual ranges for the retained material owner,
with checked capacity and lifetime. The parent is coordinating its exact use;
no guessed identity or masked native pointer is introduced here.

The subsequent typed animation prerequisite is documented separately in
`../original-j3d-animation-loader-20260903/README.md`. It provides all twelve
original animation families and explicit source-identity ownership; it does
not independently complete the ResourceHolder class migration.
