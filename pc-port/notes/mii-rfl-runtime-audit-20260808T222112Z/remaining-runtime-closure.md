# Remaining honest runtime closure

## 1. Retire the null/no-op holder (complete)

The `SceneObj_MiiFacePartsHolder` case was removed from
`SceneObjHolderCompat.cpp` and `MiiFacePartsHolderCompat.cpp` was deleted. The
exact holder, parts, recipe, `FileSelectItem`, and `FileSelector` remain
excluded. This makes the feature structurally absent instead of returning null
faces while pretending a holder exists. `SceneObjHolderRealOrAbsentTests` now
guards this boundary.

The old void RVLFace model/draw facades were removed at the same boundary.
Headers retain the retail declarations for exact-source parity, but there are
no PC definitions for matrix/expression mutation or draw setup/submission until
the renderer is real.

## 2. Port retail record semantics into Aurora

Use the recovered 100% RMGK02 RVLFaceLib implementation as the specification.
The smallest exact file-select path needs:

- the retail default database bytes and raw-to-info conversion for default
  index zero;
- the retail official `RFL_DB.dat` parser and validation for saved Mii IDs;
- `RFLiPickupCharInfo` source dispatch and validation;
- no private `SRFL` database and no synthesized character record.

The default path is mandatory: `FileSelectItem::createMii()` creates default
index zero for every new/non-Mii slot before any saved official recipe is
selected.

## 3. Decode real shape and texture resources

Build generalized Aurora loaders on top of `ResourceArchive` for the exact
RFL subarchive formats. Port the behavior represented by:

- `RFL_NANDLoader.c` file lookup;
- `RFL_Model.c` shape selection, transforms, display-list construction, model
  colors, and texture setup;
- `RFL_Texture.c` texture resource decoding;
- `RFL_DefaultDatabase.c` and `RFL_DataUtility.c` record selection.

Store native host ownership separately from the public `RFLCharModel` handle.
Do not embed host pointers into the retail PPC `RFLiCharModelRes` layout or
assume a host `GXTexObj` is the retail 0x20-byte object.

## 4. Generate expression masks for real

Port `RFL_MakeTex.c` composition for the requested expression flags. Exact
file select requests `RFLResolution_256` with normal and blink (flag value 33),
so one default actor needs a 0x482A0-byte retail model work allocation and two
256x256 RGB5A3 mask images.

Aurora may implement the composition with a generalized offscreen target, but
the resulting masks and sampling behavior must match the retail path. A blank
texture, procedural face, or placeholder is not acceptable.

## 5. Establish model registry and lifetime

The public host `RFLCharModel` should be an opaque stable handle into an
Aurora-owned registry. A model entry must retain:

- selected validated character info;
- decoded geometry and display lists;
- texture objects and expression masks;
- model and normal matrices;
- current expression and allowed expression flags;
- resource-generation identity.

Reject stale handles after `RFLExit` or resource reinitialization. Construction
must validate non-null 32-byte-aligned work storage and must not publish a
partially initialized entry.

## 6. Port actual drawing

Implement the retail `RFLLoadVertexSetting`, `RFLLoadMaterialSetting`,
`RFLDrawOpaCore`, and `RFLDrawXluCore` state/call sequences using Aurora GX.
This includes real indexed arrays, display-list execution, textures, matrices,
culling, TEV, lighting, alpha, blend, and depth behavior. `RFLSetMtx` must copy
the actor view/model matrix and compute its inverse transpose.

The exact recovered `MiiFacePartsHolder::drawExtra()` is now available as the
Game-side caller and must remain unchanged on PC.

## 7. Complete generalized Game dependencies

Add compatibility implementations or exact mirrors where appropriate for:

- J3DSys view-matrix access;
- JKR 32-byte-aligned heap allocation and solid-heap declarations;
- `MR::createDrawAdaptor` and its scheduled lifetime;
- `MR::isNoCalcView`;
- `MiiDatabase` header/destructor dependencies;
- exact holder scene connections and actor registration.

Keep those surfaces in Aurora/compat/JSystem, not local edits to Mii Game code.

## 8. Prove the closure before activation

Focused tests must demonstrate, in order:

1. retail resource copy, validation, lookup, and exit invalidation;
2. exact default index-zero record decoding;
3. deterministic geometry/texture/model creation with normal and blink masks;
4. 32-byte allocation and actor/holder ownership across async completion;
5. matrix and normal-matrix updates;
6. expression switch normal -> blink -> normal;
7. non-empty opaque and translucent Aurora draw submissions;
8. rebuild after an official recipe change;
9. teardown without stale handles or leaked owned resources;
10. a real file-select frame compared with RMGK02/Dolphin.

Only then remove the Mii exclusions from `Game/xmake.lua`, return the holder
factory case, and make `FileSelector` constructible.
