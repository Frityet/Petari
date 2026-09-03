# Retained original J3D TEX1 resources

This component creates actual `J3DTexture`, `ResTIMG`, and `JUTNameTab` objects from an authored TEX1 block. It is a native endian/address ownership boundary; it does not replace a Game texture algorithm or activate the full model loader.

## Original correspondence

- `libs/JSystem/include/JSystem/JUtility/JUTTexture.hpp`: the complete 32-byte `ResTIMG` record, including three bool bytes, opaque byte 0x19, signed LOD bias, and relative image/palette offsets.
- `libs/JSystem/include/JSystem/J3DGraphBase/J3DTexture.hpp`: actual inline constructor, getters, and empty virtual destructor. The existing PC header is byte-identical; no missing constructor provider was needed.
- `src/JSystem/J3DGraphLoader/J3DModelLoader.cpp`, `J3DModelLoader::readTexture`: construct a JUTNameTab when the name offset is nonzero, then J3DTexture(count, records); attach both to the material table. The new component follows this ownership result using retained native records. Name count is not artificially required to equal texture count.
- `src/JSystem/J3DGraphBase/J3DTevs.cpp`, `loadTexNo`: preserve every authored field; the actual writer consumes format & 0x0f, min/max LOD times 0.125, bias times 0.01, and original palette-size selection. Validation computes the corresponding low-nibble image layout but retains the complete format byte.
- `src/JSystem/J3DGraphBase/J3DGD.cpp`, `J3DGDSetTexImgPtr` and `J3DGDLoadTlut`: actual mapped pointers pass through OSCachedToPhysical and right shift by five. Original command lengths/patch offsets are unchanged.

`verify-source.py` checks these existing SDK implementations against their current root sources and records hashes. This is source correspondence, not a new retail binary-match claim. No root decompilation or Game edit was needed in this tranche.

## Memory and lifetime

`Mem1ResourceHeap::create(budget)` is an explicit, single process-startup decision after completed OSInit. The caller chooses the budget and passes the same shared owner to all texture components. It rejects an already initialized OS allocator through the narrow AuroraOSIsAllocatorInitialized query; it never silently replaces another allocator.

The owner reserves a 32-aligned region from the top of the actual MEM1 OS arena using OSAllocFromArenaHi. OSInitAlloc creates its descriptor table in this reservation; OSCreateHeap uses the remainder. Existing arena allocations below the new high watermark remain separate. Setup does not choose a global current heap. Available payload is smaller than the requested reservation because original heap metadata also occupies it. Future users of the arena must observe the updated arena high address; they must not initialize an independent allocator over the reservation.

Each move-only allocation retains the shared heap. The heap destroys its original OS heap only after its final allocation dies. It does not restore the arena high watermark: the process-global OS descriptor table remains in the reserved region, so repeated setup remains rejected even after the C++ owner is destroyed. The intended runtime holds this owner until all retained resources have retired. This is deliberately one authoritative setup, not one heap setup per model.

For N textures, one real OS allocation contains N native ResTIMG records (32 bytes each), followed by an unchanged complete raw TEX1 copy. Both the allocation and raw-copy base are 32-aligned. Each relative offset is rebased forward into that copy; shared images, shared palettes, and image/palette cross-type aliases remain the same address. All offsets remain representable in the original u32 records. The unmodified raw copy also supports provenance/inspection. Names use the existing retained J3dNameData component, which preserves name offset aliases.

Before returning mapped storage to the heap, Allocation drains queued FIFO CPU processing through AuroraDrainGXCommands. The GX command processor resolves/uploads sampled texture bytes before recording retained GPU texture/bind-group handles (`gx/command_processor.cpp::push_gx_draw`), so later GPU encoding no longer reads this TEX1 backing. The barrier emits no command or draw-done callback and is valid with an empty FIFO before GXInit or after renderer shutdown. Existing AuroraGXSync additionally flushes producer __gx state, so it is not used for this narrower resource destructor boundary. Resource destruction must run on the game/producer thread, consistent with existing GX producers. A complete model owner must still retain this component for every future use of its table or reusable display lists; draining already queued work cannot authorize later use of retired pointers.

## Validation boundary

The component checks the exact block extent, record/name ranges, every image mip extent, palette extent, and original BP alignment before publishing objects. It supports all eleven original image formats handled by Aurora: I4, I8, IA4, IA8, RGB565, RGB5A3, RGBA8, C4, C8, C14X2, and CMPR. It retains all metadata and computes image reads from both the declared image count and the actual masked max-LOD/filter state. Invalid native bool representations, unsupported low format nibbles, invalid GX dimensions/sampler enums, and out-of-range/unrepresentable offsets fail visibly.

Palette storage retains all authored entries, including C14X2 palettes. Actual original loadTexNo still chooses a 16- or 256-entry load from mPaletteNum; this existing source behavior is not changed or represented as full C14 rendering validation. J3DTexture::setResTIMG can involve wrapped original relative offsets for cross-owner copies; this tranche guarantees its own forward-owned construction, not arbitrary future cross-owner texture mutations. Native GX host-pointer ownership is unchanged. No address registry or display-list sidecar is introduced.

## Verification

Isolated compilation with the current PC compile database succeeds for Mem1ResourceHeap.cpp, J3dTextureData.cpp, and OriginalJ3DTextureResourceTests.cpp. Logs/objects are under ignored `build/original-j3d-texture-resource-20260903/`; `compile-isolated.py` reproduces this check without xmake.

The focused fixture uses real OSInit/OS heap owners and original J3DTexture/J3DMaterialTable/GD writers. It covers all eleven formats, opaque metadata, masked format upper bits, native physical-address round trips, aliases after input-source retirement, exact original paletted command size/address words, attachment ownership, malformed resources, mip bounds, allocation exhaustion, reclamation, and heap lifetime. Optional SMGPC_REAL_DISC reads Mario's authored mario.bdl TEX1 and verifies every retained record after archive ownership retires.

At source freeze only isolated compilation has run; parent owns linked runtime tests and shared build wiring. This note makes no successful GPU/full model publication claim.
