# Original J3D vertex and shape construction

This checkpoint adds retained native VTX1/SHP1 construction data and executes the original `J3DShapeFactory`, `J3DShapeTable::initShapeNodes`, and `sortVcdVatCmd`. It also extracts the shared original name-table backing from `J3dJointData` into `J3dNameData` for JNT/MAT/SHP consumers.

`J3dGeometryData` is a component for the complete typed model owner. It does not publish a partially decoded BMD/BDL as a complete model, create empty substitute material tables, link a hierarchy, change gameplay, or replace the current renderer owner. Complete material/texture/vertex/joint cross-validation and original finalization remain the complete owner's responsibility.

## Source correspondence

- `compat/J3DShapeFactoryCompat.cpp` is an exact copy of root `src/JSystem/J3DGraphLoader/J3DShapeFactory.cpp`, and the PC factory header is identical to its root header. The only new root changes replace PPC allocation-size literals with `sizeof` for the actual shape, matrix subclasses, draw objects, and pointer arrays. Original PPC sizes are unchanged. Existing native `J3DShape::kVcdVatDLSize` remains responsible for Aurora's wider array-binding commands.
- `compat/J3DShapeTableCompat.cpp` contains both unchanged root bodies, with one additional complete-type include required by the PC header's dependency narrowing. A `TARGET_PC`-guarded friend declaration in root and PC `J3DShapeTable.hpp` allows the construction component to populate actual private table fields without layout changes or fake loader objects.
- `resource/J3dGeometryData.cpp` is an endian/alignment/ownership boundary outside `Game/`. It reads authoritative raw INF1/VTX1/SHP1 sections. It does not derive native records from renderer summaries. It retains actual `J3DShapeInitData`, `J3DShapeMtxInitData`, `J3DShapeDrawInitData`, GX format/descriptor records, native matrix indices, and unchanged big-endian draw bytes.
- Shapes are constructed in INF shape-command order by the actual factory. Repeated shape commands retain all created allocations and replace the logical table slot as original `readShape` does. Caller load flags select matrix classes; INF flags are not silently merged into this factory argument.
- The retail `create` function at `0x80440838` never uses its input third argument (`vtxDesc`, incoming r6). Its original body retains each authored byte-offset descriptor. The native owner still passes the previous descriptor as original `readShape` does.

`verify-source.py` recompiles the two root files with GC/3.0a3 and the repository's `cflags_jsys`, verifies exact imports, and checks selected metadata instructions in the verified RMGK01 DOL (`25c5959534b3c21246c6c7e42021b916b41fb578`). Results are recorded in `source-evidence.json`.

The two ShapeTable bodies, factory constructor, command allocation/size, matrix-size query, and group-count getter match 100%. Other existing factory bodies are reported honestly: create 80.07921%, newShapeMtx 63.634922%, newShapeDraw 90.15151%, calcSize 94.84375%. These are not new decompilation claims. Retail disassembly was inspected for constructor/allocation flow, selected matrix classes, preserved fields, group traversal and logical numbering. Native tests independently exercise every original matrix-class choice and retained authored data.

Original compiler logs, comparison JSON, raw extracted `create`/`newShapeMtx` disassembly and native command lists are under ignored `build/original-j3d-geometry-resource-20260903/`. Retail `readVertex` disassembly is under `build/original-j3d-joint-resource-20260903/retail-read-vertex.asm`.

## Vertex contract

Retail `readVertex` (`0x8043e784`, size `0x254`) uses specific count formulas, not a generic next-table record count:

| Field | Original computation |
| --- | --- |
| normal count | Distance to NBT, else color0, else tex0, else block end; divide by 12 for F32 or 6 otherwise, then add 1 |
| color0 count | Distance to color1, else tex0, else block end; divide by 4, then add 1 |
| tex0 count | Distance to block end; divide by 8 regardless of component type, then add 1 |

Absent arrays produce zero metadata counts. INF supplies the position and packet counts. Native allocation changes physical pointer distances, so the decoder computes these fields from source offsets before relocation. The native VTX header is not passed back into original `readVertex` to recompute distances using the expanded layout.

Every present array retains the complete physical pool plus the original count-based footprint when it is larger. Actual readable bytes beyond a source table/block are copied when needed; padding is not synthesized and counts are not clamped. Numeric F32/U16/S16 components become native endian. Packed color numeric16/numeric24 values are converted at the original fixed four-byte binding stride; RGB/RGBA byte-channel formats remain byte-identical. Independent native views permit normal lookahead to interpret bytes numerically while an overlapping color pool retains byte channels.

Raw formats/fractions and descriptor aliases remain original. Original `makeVtxArrayCmd` determines the live position/normal type fields and NBT/PN-matrix flags during `initShapeNodes`, rather than the owner advancing that state prematurely. The shape command allocation is retained separately from sorted pointers so aliases do not cause duplicate deletion. Retiring its allocation invalidates the original shared VCD cache if it points inside the allocation.

## Name-table contract

`J3dNameData` owns converted native-endian ResNTAB records and an actual `JUTNameTab`. Its default constructor represents an absent table. A present table with zero entries remains present. Stored count is independent of the number of joints/materials/shapes; original guarded `getName` returns null past that stored count. Authored key codes, relative string offsets, opaque header field and strings are preserved.

`bytes()` exposes converted header/records plus raw strings, suitable for appending into `J3dNativeBlock` at `alignof(ResNTAB)`. A present four-byte empty header receives only unused native object-tail padding; its count is unchanged. The common helper replaces the former joint-private parser. The shared arena owns a copied representation before the temporary name owner is retired.

## Validation and limits

The component checks declared file/block extents, required/unique construction blocks, format/descriptor termination and readable types, remap/initializer/matrix/draw extents, original count arithmetic without underflow, bounded names, descriptor alignment, recognized factory matrix types, and complete bounded INF shape coverage before original pointer-based construction.

This is metadata construction, not complete display-list admission. Raw display-list vertex references still require cross-validation against vertex pools, and matrix indices against the retained joint/envelope/draw data, before the complete owner publishes a model. Tests call original shape initialization and sorting, not draw submission with a deliberately incomplete material/joint fixture.

Original multi-matrix loops skip `0xffff` slots before indexing their ten-slot load cache. Validation therefore rejects a live matrix at slot 10 or later while preserving a readable all-carry suffix and its declared count. The regression covers both ordinary and concat-view matrix owners with eleven declared entries and the original carry suffix.

Initial factory allocations are explicitly owned. Later `addTexMtxIndexInVcd` / `addTexMtxIndexInDL` can allocate replacement arrays and have empty original destructors. A complete resource/heap owner must retain those replacement allocations, including superseded replacements; this initial component does not claim to solve that later mutation lifecycle. Exceptional allocation failure inside an original factory call likewise needs a complete original heap/allocation scope to recover partially created allocations.

## Native evidence

`python3 pc-port/notes/original-j3d-geometry-resource-20260903/verify-native.py` compiles current component/test/SDK extraction objects and links them before the prior frozen native archives. It checks archive hashes before/after and runs with the actual RMGK01 disc. No shared xmake configuration/build, renderer or GPU process is used. Current result: **5/5 groups pass**, including actual Mario.bdl. `geometry-evidence.json` records source/binary/archive hashes; `geometry-native.log` is the compact run output.

The groups cover:

1. Actual factory class identity for all four shape modes under both caller flags, remap/byte-offset descriptors, original size queries, matrix carry entries, draw bytes/countVertex, names, source retirement, move, reattachment rejection and cache retirement.
2. Native scalar arrays, original +1 count footprints crossing tables and into the following resource block, full texture pools, opaque format bytes, original shape initialization and command sorting.
3. All six packed/byte-channel color formats, absent arrays/names, and source-backed matrix carry suffixes beyond the ten live load slots.
4. Malformed file/array/remap/format/descriptor/matrix/draw/name bounds and missing/out-of-range INF shape slots.
5. Actual Mario.bdl: nine original shapes, twelve groups, every referenced matrix and draw byte, original VCD generation/sorting and archive-retirement lifetime.

Independent extracted Mario metadata: normals 2742, colors 41, tex count 977, **1952 stored S16/ST texture records**. The full texture pool remains available despite the smaller original metadata count. Original sorting shares the first eight shapes' commands and retains the ninth shape's distinct layout.

`verify-name.py` separately compiles the shared-name extraction with the current joint component and original helper against frozen archives: **4/4 joint-resource groups pass**, including actual Mario. Added assertions cover absent/present-empty names, count smaller than joint count, and copying converted names into the shared arena before retiring the source owner. See `name-evidence.json` and `name-native.log`.
