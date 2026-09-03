# Original material factory and retained MAT3/MDL3 input

This checkpoint restores the original material factories needed by the complete
model/resource pipeline. It does not activate Mario jumping or publish a partial
model through the original ResourceHolder interface.

The two disabled factory bodies in root `J3DMaterialFactory.cpp` now compile with
the configured original compiler. The complete native factory is byte-identical
source. Original patched/locked material virtual methods are copied from root.
The missing base J3DMaterial constructor is restored in the root/native SDK
headers: it initializes current-matrix state and calls initialize(). Its84 bytes
and relocations match retail exactly. The native fixture constructs it in reused,
nonzero storage and checks the original initial fields. No Game code changes
belong to this component.

`verify-original.py` compiles the root source, compares the original retail split
object, and checks source correspondence. Results for the restored normal and
patched functions are98.457146% and89.872826%; the existing locked function is
79.89865%. These are instruction-similarity scores, not a claim of exact retail
machine-code equivalence. The principal differences include constructor inlining
and register scheduling. The complete per-function results are recorded in
`compiler-evidence.json`.

`J3dMaterialBlockData` retains one complete raw MAT3 or MDL3 and constructs native
typed metadata records with the original header-relative offset interface.
Declared halfwords, words, floats and signed colors are decoded; byte fields,
sentinels and opaque fields are retained. Material remap indices and table extents
are checked before the original factory can access them. Indirect records use
logical material indices independently of the remap. Their enable byte follows
retail's exact comparison with1, confirmed in the original instructions. Native
layout also preserves the factory's name-to-indirect-offset distance branch when
the original name table is present but empty.

MDL3 descriptors retain a single unchanged command image. Their relative offsets
are rebased after native layout is fixed, preserving shared command bytes and
original patch offsets. Patching/current-matrix/mode tables have checked counts.
The metadata component does not own factory-created materials; the forthcoming
complete resource owner must retain those allocations and validate the live
patch operations and cross-block material/texture/shape indices before model
publication. Input indirect records with more than three enabled matrix slots
are rejected because the current original structure provides three matrix slots.

`J3dNativeBlock` is a shared aligned owner for pointer-bearing native headers and
typed tables. Arrays have C++ lifetimes, offsets are stable before publication,
and relative fields can be fixed before the owner is finalized. Command bytes
can retain their original format independently of native pointer width.

`verify-native.py` compiles the changed source with the configured game flags and
links against hashed, unchanged native archives. Four groups pass:

- Remapped MAT3 values, defaults, enable-byte semantics, empty names, typed SRT,
  RGBA and signed TEV values through actual normal and patched factories.
- Aliased aligned MDL3 commands through the actual locked-material factory.
- Six malformed extent/index fixtures rejected at the native data boundary.
- All nine materials in the supplied Mario BDL through normal, patched, locked,
  and existing-material display-list attachment paths after the source archive
  has been retired.

The tests explicitly free standalone factory allocations because the original
SDK delegates their lifetime to the resource heap. Runtime activation and the
complete model owner's allocation tracking remain separate work.
