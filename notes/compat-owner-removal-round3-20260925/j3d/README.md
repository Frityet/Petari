# J3D shape, matrix-buffer and vertex owner restoration — 2026-09-25

Six complete canonical JSystem translation units replace six compat providers. This is source consolidation with explicit native adaptations retained; it does not claim new visual/gameplay progress or invent changed behavior where the fragments already matched the original.

## Ownership

| Canonical owner | Removed provider / displaced original methods |
| --- | --- |
| J3DGraphBase/J3DShape.cpp | J3DShapeCompat.cpp |
| J3DGraphBase/J3DShapeDraw.cpp | J3DShapeDrawCompat.cpp |
| J3DGraphBase/J3DShapeMtx.cpp | J3DShapeMtxCompat.cpp; J3DPSMtx33CopyFrom34 from J3DTransformMtxCompat |
| J3DGraphLoader/J3DShapeFactory.cpp | J3DShapeFactoryCompat.cpp |
| J3DGraphAnimator/J3DMtxBuffer.cpp | J3DMtxBufferCompat.cpp |
| J3DGraphBase/J3DVertex.cpp | J3DVertexBufferCompat.cpp; J3DVertexData construction from J3DModelDataCompat; J3DDrawMtxData lifecycle from J3DJointTreeCompat |

The full current decomp files are the base. The three remaining mixed providers only lose the original functions whose actual owner is restored here. No headers, Game files, JKernel files, resource lifetime logic, renderer or Aurora implementation was edited.

## Preserved native boundaries

- Shape array-base upload uses GXSetArrayBase, allowing Aurora to retain the full native pointer. Reintroducing the Wii CP-command address mask would truncate it.
- ShapeDraw reads unaligned primitive counts as big-endian and writes expanded counts in that byte order. Original fan/strip-only acceptance, command stopping, allocation and padding behavior remain unchanged. Final display-list extent uses uintptr_t subtraction.
- ShapeFactory uses sizeof for native shapes, draw records, matrix subclasses and pointer tables. Existing enlarged native VCD/VAT command reservation remains untouched in J3DShape.hpp.
- ShapeMtx retains complete original MW assembly under its original compiler guard. Native J3DPSMtx33Copy snapshots all nine values before any store. J3DPSMtx33CopyFrom34 retains the already-established scalar implementation with every source row loaded before writing an overlapping destination.
- MtxBuffer retains the native envelope branch byte-for-byte, including explicit std::fma, accumulation/store order, scale flags and the original do/while behavior. Current original MW assembly remains under its compiler guard. The prior floating-point contraction-off directive is preserved. Its private copy of getMdlDataFlag_MtxLoadType is gone; the complete original source includes the existing J3DModelLoader header owner.
- Borrowed array/display-list ownership, original empty destructors, draw matrix sentinel objects, scheduler/interrupt operations and shape matrix cache globals are retained. No generic success fallback or special-stage path was added.

## Regression and validation

OriginalJ3DPacketTests.cpp now exercises overlapping 3x4-to-3x3 matrix copies and the actual ShapeMtx normal-matrix scaling path for destination buffers before, at and after the source. It compares the entire storage buffer, including guard values and signed zero. This guards the native all-loads-before-stores behavior when the helpers are reunited with their proper owner.

Existing packet tests cover unaligned counts above 255, inserted matrix indices, original padding and full native array pointers. Existing geometry-resource tests check every native shape-matrix allocation size/type. Matrix-buffer tests retain the envelope math checks; vertex-buffer tests cover original borrowing and swapping. This lane did not execute any of them.

`python3 notes/compat-owner-removal-round3-20260925/j3d/validate-source.py` passes **41 source checks**. All **71 detected original functions** are present with a single CPP provider. Only nine function bodies differ from donor, for the documented full-pointer/endian/native-size/scalar-MW boundaries and typed raw-zero argument adaptation. Original MW copy bodies and the prior scalar envelope branch are checked independently. These lexical checks are not compilation or runtime proof.

All 10 touched existing files were clean at batch start. `before.json` records status and hashes; `baseline/` contains their copies. `source-changes.patch` is the exact change set. `git diff --check` and added-line whitespace checks pass. No initially dirty files were edited.

## Parent build handoff

`build-wiring.json` lists the six canonical source additions. The six deleted providers leave the compat wildcard automatically. No additional source flags or test target changes are needed; MtxBuffer contains its preserved FP directive. Run the full native build and existing original-j3d packet, mtx-buffer, geometry-resource, vertex-buffer and texture-mtx tests.

No build, Git index, staging, commit or push operation was performed in this lane.

## Deferred coherent owners

The original SMG loadMtxIndx_PNGP override remains unchanged in J3DShapeMtxGameCompat. A native Game/System/Overwrite.cpp does not yet exist; restoring only this one method would create another incomplete owner while the related JPA overrides remain fragmented. Complete Overwrite restoration should migrate them together.

Whole Joint/JointTree restoration spans additional J3DTransform, JMath, J3DSys traversal-state and native hierarchy ownership methods. This batch leaves those systems intact rather than mixing their fragments into the shape owners.

## Build integration follow-up

The native build exposed two donor spelling differences: J3DShapeDraw now includes Aurora's existing revolution/gx.h umbrella instead of the unavailable revolution/gx/GXDispList.h leaf, and J3DShape passes 0U to GDSetArrayRaw's raw u32 address parameter instead of nullptr. Both retain the original clear-slot semantics. Root reports the full native app build passed after these fixes; gameplay/runtime validation remains separate.
