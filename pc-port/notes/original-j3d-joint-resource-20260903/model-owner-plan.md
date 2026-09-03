# Complete model owner boundary and renderer handoff

The existing summaries preserve enough data for current rendering, but not enough to reconstruct a faithful original ModelData. A complete owner should retain the authoritative BMD/BDL bytes, native table backing, actual factory-created objects, display-list allocations and every relative-address texture arena. It should publish the real ModelData only after construction and original finalization succeed. The joint component in this checkpoint is one private construction component of that owner.

| Data | Existing useful summary fields | Complete original ownership still needed |
| --- | --- | --- |
| INF/JNT | Full hierarchy, flags, joint remaps, SRT/bounds | Address-stable actual joints/calculator/native hierarchy and original names. Supplied by the new component; material/shape links remain deferred. |
| EVP/DRW | Envelope indices/weights, selected inverse matrices, boolean draw flags | Raw byte flags, source-order native count, all readable inverse matrices, actual important-index allocation and original finalization. Supplied by the new component. |
| VTX | Attribute formats, array offsets/strides/inferred counts, flattened renderer geometry | Exact original typed format table and terminator; stable CPU-native position/normal/NBT/color/texcoord arrays; original count calculations; raw BE GX display-list streams retained separately. |
| SHP | Logical matrix types/bounds, VCD descriptors, per-group matrix tables and display-list offsets | Original remap/VCD aliasing, actual Shape and ShapeMtx subclasses, native matrix tables, retained raw BE ShapeDraw bytes and command storage, original vertex/draw-table attachment and final sorting. |
| MAT2/MAT3 | Convenient render GX states and material associations | Full factory tables, indirect initialization, lights, secondary texcoord/effect fields, swap modes/tables, fog/dither/NBT and untouched raw initialization values. Parent owns factory/block closure and native backing. |
| MDL3 | Packet bytes and offsets | Patching/current-matrix/material-mode metadata and exact locked/patched material creation. Parent owns this with materials. |
| TEX1 | Image bytes/mips, decoded image, many texture parameters | Full original ResTIMG fields, palette bytes and semantics, names, contiguous stable header/payload arena retaining u32 relative image/palette offsets. Parallel texture/MEM1 work owns low-level address semantics. |

The original root loader finalizes in this order: `ModelData::makeHierarchy` (JointTree hierarchy plus ShapeTable initialization), `sortVcdVatCmd`, `findImportantMtxIndex`, and `setupBBoardInfo`. BDL additionally runs original `indexToPtr`. `setupBBoardInfo` uses each joint's actual first mesh and that mesh's remapped SHP matrix type; assigning a joint billboard mode from a convenient summary is not equivalent. Load flags and BMD/BDL model data type also affect actual factory class choices and matrix buffers.

`J3DShapeTable` currently grants mutation access only to the original `J3DModelLoader`. A coherent complete owner needs the actual loader interface or an explicit narrowly scoped native ownership friend/accessor. It must not fabricate a tiny alternative class named ModelLoader or cast private table storage. Raw BE block headers contain 32-bit file offsets where the native headers have widened pointers, so whole-header casts are invalid.

Original `readVertex` has specific normal/color/texcoord count formulas (including apparent `+1` rounding). These need retail checking before converting arrays; the summary's generic `inferred_count` must not silently replace them. Original shape creation selects matrix subclasses from the global flags and shape matrix type, retains group display lists and matrix tables, then allocates/sorts VCD/VAT command storage. Those ownership paths are the next independent component.

## Sharing actual model matrices

`J3dModelRenderer::load` currently creates its own `OriginalJ3dJointTree`. `calculate_joint_matrices` executes that private one-track Core again from draw, joint queries and bound queries; `LiveActorModel::refresh_resolved_joint_matrices` repeats joint queries. Once a real XanimePlayer owns an actual model, these calls must consume the already calculated model buffer. Recalculating with the renderer's private clock/Core would replace the original update/calculation phase and blend state.

A coherent first bridge exposes the actual model's animation matrices and original `mpWeightEvlpMtx` as read-only matrix views from the retained model owner. Current world-space geometry can consume them directly, and joint queries can use the actual animation matrices. This also retires the renderer's separate `weighted_envelope_matrix`, which currently skips invalid influences, omits missing inverse transforms and accumulates its own scalar math.

Actual animation/envelope matrices already include model base transform/scale. Actual draw and normal matrices after `viewCalc` are camera-space. The current renderer applies the camera projection separately, so it must not accidentally feed those camera-space draw matrices into the old world-space path, apply model scale twice, or overwrite the model by recalculating during queries. Moving the full original draw/normal path into rendering requires an explicit corresponding projection/interface change.

Full original ResourceHolder ownership still must replace the conflicting global native archive-only ResourceHolder layout before production XanimeResource lookups can safely use real model/motion resource tables. This component neither changes that boundary nor exposes an empty-material model as a loaded BMD.
