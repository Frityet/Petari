# Original v26 material-table construction

This component runs the original material-factory and reader sequences over
retained native MAT3/MDL3 records. It supplies the material portion of the complete
model/resource owner. It does not construct a partial J3DModelLoader, publish a
Game resource, render a model, or activate Mario's animator.

The production changes are `src/resource/J3dMaterialTableData.hpp/.cpp`, plus one
native SDK byte-copy correction in `src/JSystem/J3DGraphBase/J3DStruct.hpp`.
`tests/OriginalJ3DMaterialTableTests.cpp` contains six focused groups. Paths in
this paragraph are relative to `pc-port/`.

## Original source correspondence

| Native component method | Authoritative root method | RMGK01 address / size | Original compiler match |
| --- | --- | --- | --- |
| `Storage::read_material` | `J3DModelLoader_v26::readMaterial` | `0x8043EC04 / 0x27C` | 94.08805% |
| `Storage::read_material_table` | `J3DModelLoader_v26::readMaterialTable` | `0x8043F2CC / 0x13C` | 91.3924% |
| `Storage::read_patched_material` | `J3DModelLoader::readPatchedMaterial` | `0x8043F608 / 0x13C` | 91.77215% |
| `Storage::read_material_dl` | `J3DModelLoader::readMaterialDL` | `0x8043F744 / 0x1AC` | 89.252335% |
| `Storage::modify_material` | `J3DModelLoader::modifyMaterial` | `0x8043F8F0 / 0x7C` | 100% |

All five methods already exist in
`src/JSystem/J3DGraphLoader/J3DModelLoader.cpp`; this task does not change root
algorithms. The exact original `J3DMaterialFactory::create`, its actual normal,
patched and locked classes, and `modifyPatchedCurrentMtx` are supplied by the
previously imported root-identical `compat/J3DMaterialLoaderFactoryCompat.cpp`.

`verify-original.py` compiles the current root TU with configured
`cflags_jsys`, GC/3.0a3, VERSION=0. It compares against the retail object and
reconstructs each compared retail function from ELF relocations into the
verified current DOL bytes. DOL SHA1:
`25c5959534b3c21246c6c7e42021b916b41fb578`.
The eight checked functions include both material/table loop entrypoints and
the newly recovered binary dispatcher from the other worker. Exact hashes,
addresses, sizes, calls and relocation counts are in `compiler-evidence.json`.

The lowest match, `readMaterialDL`, has the same factory calls, loop bodies,
field writes, constants and conditions. This compilation leaves
`JSUConvertOffsetToPtr<ResNTAB>` out of line, adds moves to preserve its result,
and emits redundant zero-count entry checks before the loops. The retail
inlines the same null-or-base-plus-offset helper and enters the existing loop
condition directly. The BMT reader also saves one additional register.
The paired instruction listings preserve these discrepancies. We do not call
the native component binary matching, nor treat the percentages as validation
of native pointer widths.

## Preserved behavior

The component admits v26 resource headers `J3D2/bmd3`, `bdl3`, `bdl4`, and
`bmt3`; its explicit Mode chooses the reader. SDK format dispatch belongs to
the complete resource owner. In particular, the recovered original table
entrypoint only accepts bmt3, while binary loading accepts bdl3 or bdl4. This
separation does not remap blocks or invent material parameters.

- Model mode passes the supplied flags into original `readMaterial`.
- Binary mode processes blocks in authored order. MAT3 uses
  `0x50100000 | (flags & 0x03000000)`. Selection bits `flags & 0x3000` choose
  normal construction for zero, patched construction for `0x2000`, or skip
  construction for `0x1000`/`0x3000` while retaining the preceding MAT3 pointer.
  Each MDL3 executes original locked creation/patching followed by the original
  conditional MAT current-matrix reconstruction.
- MaterialTable mode uses fixed `0x51100000`, leaves unique-array/count fields
  at their original clear values, and ignores the caller's flags.
- Original `countUniqueMaterials` counts increases above the previous maximum
  remap ID. It is not the number of distinct IDs. The optional original unique
  array uses that count, and logical materials retain actual pointers into it.
- Each logical material is a separate actual factory allocation. Equal remap
  IDs share the original comparison identity, without collapsing allocations.
- Model/patched identities use original pointer-right-shift-by-four semantics.
  Unique identities preserve original `sizeof(J3DMaterial) == 0x4C` stride;
  the native class size does not affect comparison flags. BMT uses the original
  unshifted cached-address value plus the remap ID. The parent-owned
  `J3dAllocationIdentity` supplies retained disjoint original-width identities;
  they are not physical addresses or truncated native pointers.
- MDL3 applied to an existing material preserves its dynamic class, material
  mode and comparison identity. Its shared display-list object is installed
  only if absent. Later MDL3 blocks update matrix and patch offsets while
  retaining the earlier command image. Original cross-material command aliases
  and 32-byte native alignment remain intact.
- A later MAT3 replaces material fields in file order; original `readMaterial`
  does not clear an already set table lock flag. This unusual behavior is
  retained and tested.
- Empty BMT is valid. TEX1 attachment and the original empty J3DTexture fallback
  are the complete owner's responsibility. `attach_to` copies only the six
  material fields once and leaves both texture fields untouched.

## Lifetime and validation boundary

A nonnull retained `JkrAllocationDomain` is required. Actual SDK material,
name-table and pointer-array construction runs under `JkrAllocationScope`.
Native block storage, STL ownership metadata and comparison-address metadata
run under `JkrHostAllocationScope`. Original partial, replaced and post-load
subsidiary allocations remain inside the actual Solid heap until the domain's
last owner releases it. The component does not attempt to individually free
potentially aliased or replaced material graphs.

Successful top-level factory values use their actual constructed class when
running destructors; J3DMaterial has no virtual destructor. The optional unique
array runs its actual element destructors. Actual names and SDK pointer arrays
are destroyed before retained MAT/MDL backing and before the final domain
reference. The original material/table and block destructors do not own their
subsidiary allocation graphs. Typed SDK objects and original heap bulk release
therefore have separate, explicit duties.

Source input bytes may leave immediately after construction. Every encountered
MAT/MDL backing remains retained in authored order, including earlier command
images used after later MDL patches. Attachment is borrowed: the complete owner
must retain this component while the attached table is used. The construction
API does not detach or mutate a live table during component destruction.

Native validation rejects truncated blocks, unsupported MAT2 model/table
construction, unique-array remaps outside the original counted allocation, MDL
material indices beyond their decoded records, and patched MDL without a valid
preceding MAT. These checks happen before the original unchecked accesses.
All native validation exceptions construct their messages under host escape,
including checks executed inside the Game allocation scope, so exceptions
remain readable after a failed constructor releases its last domain reference.
Factory out-of-memory remains the original allocation failure; its abandoned
SDK allocations are reclaimed by the actual heap domain.

`tex_no_patch_offset(u16)` exposes the last actual MDL setter input for each
final material. MAT-only material offsets are zero. This allows the complete
owner to validate texture patch bounds before original `indexToPtr` runs.
It does not change `J3DTevBlockNull`'s inherited virtual getter returning zero,
and it is deliberately construction metadata rather than an observation of
later Game mutations.

## Native SDK alignment correction

The expanded sanitizer fixture reached the genuine default TEV-order resource
through the actual material factory. `J3DTevOrderInfo` has byte alignment, but
its inherited native assignment used a `u32*` load/store. The constant's actual
address was unaligned; UBSan reported the load at native `J3DStruct.hpp:166`.
The native-only correction uses `__memcpy(..., sizeof(J3DTevOrderInfo))`, exactly
four bytes including the opaque fourth byte. Root source remains unchanged.
An explicit fixture constructs both actual value objects at offset one from
four-byte-aligned storage and verifies all bytes. No material values or
packing rules are changed to work around the alignment.

## Validation

`verify-native.py` compiles the new component, parent identity helper and fixture
using the existing configured game/test flags, then links the frozen native
SDK/Aurora archives. Six groups pass, checking normal/unique/BMT construction,
all binary selection branches, MDL order/alias behavior, empty and invalid
input, attachment ownership, final-domain exception lifetime and partial
original factory failure. Every group returns the original root heap to its
exact initial free capacity. One `allocFromHead` diagnostic is expected from the
512-byte failure fixture.

`verify-native.py --sanitize` additionally instruments the component, decoded
backing, actual material factories, heap/lifecycle providers and tests with
ASan/UBSan. Other existing SDK/Aurora archives remain uninstrumented; this is
not a whole-program sanitizer claim. See the separate `native-asan.log` and
`native-asan-evidence.json` for the result.

This isolated fixture uses synthetic serialized J3D records and actual SDK
constructors; it does not fabricate virtual objects. Real-disc complete-model
coverage and shared xmake/GPU gates belong to the parent's integration run.
`verify-source.py` checks the reviewed source correspondence hashes and is
separate from original-compiler proof.
