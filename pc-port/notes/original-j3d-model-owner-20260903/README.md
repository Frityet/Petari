# Original J3DModel provider closure

This tranche supplies the real SDK model and base-material implementations. It does not activate Mario's ModelManager, MarioAnimator, material factory, or resource ownership. No fake model, forged vtable, or replacement model virtual is used.

## Source ownership

- `pc-port/src/compat/J3DModelCompat.cpp`: complete `src/JSystem/J3DGraphAnimator/J3DModel.cpp`, plus the exact empty destructor from `src/Game/Player/J3DModelX.cpp:571`. The actual embedded VertexBuffer destructor runs. The original destructor does not free the separately allocated matrix/packet arrays; owners must preserve the original arena/allocation lifetime instead of assuming that `delete model` frees them.
- `pc-port/src/compat/J3DModelDataCompat.cpp`: original clear/constructor/destructor, plus original shared-display-list creation, `indexToPtr`, and J3DSys flag synchronization. Existing table/vertex lifecycle providers remain there.
- `pc-port/src/compat/J3DMaterialHelpersCompat.cpp`: all ten base-material virtual methods, original initialization, current-matrix/copy/DL helpers, and exact `loadNBTScale` from J3DTevs. Block factories and patched/locked material subclasses are a subsequent real resource-owner dependency, not fabricated here.
- `pc-port/src/compat/J3DClusterCompat.cpp`: complete original Cluster TU, with only the explicit PPCSync declaration include added.
- `pc-port/src/compat/J3DSkinDeformCompat.cpp`: exact original skin-deform and vertex-color dispatch wrappers. They preserve the real virtual dispatch into an attached object; they do not construct a synthetic deformer. Skin-deformer construction/CPU kernels and cluster-resource construction remain separate SDK closure work.
- `pc-port/src/compat/PPCArchCompat.cpp`: native CPU memory-ordering provider, described below.
- Full original declarations restored for J3DMaterial, MaterialAnm, MatBlock, Tevs, Texture, JUTNameTab, and Animation. The native Animation header now includes the original material/color/cluster animation classes and FrameCtrl.reset, rather than an incomplete transform-only definition.

The matrix-buffer owner separately provides actual allocation, envelope/draw/normal/billboard calculation, view-base transform and default constants. The parent owns Packet, DisplayList, DrawBuffer, Shape/ShapeMtx/ShapeDraw, Joint::entryIn, GD command writers, and VertexBuffer::setArray. This tranche initially copied the missing GD header; its subsequent writer recovery belongs to the parent's group. No stale GD hash is pinned by this verifier.

## Root-first missing inline recovery

`J3DModel::calcNrmMtx` was declared without a definition. Both original `viewCalc` paths load `mMtxBuffer` from model +0x84 and call the actual buffer `calcNrmMtx` at 0x80432654. The recovered class-inline body simply delegates to that buffer. It is in the real root Model header and the exact PC mirror.

`verify-original.py` compiles the actual root TUs with configured GC/3.0a3 flags and normal root includes. It verifies the two buffer loads and relocation targets independently in retail and compiled `viewCalc`. There is no generated substitute header.

## Evidence and limits

Verified RMGK01 main.dol SHA1: `25c5959534b3c21246c6c7e42021b916b41fb578`.

The original compiler succeeds for all seven real TUs: Model, ModelX, Cluster, SkinDeform, Material, ModelData, and Tevs. `compiler-evidence.json` records every imported method's retail address, size, original-compiler size, measured objdiff percentage, and original byte hash. `original-compiler-results.txt` is the compact result list.

- Model constructor entry, shape-packet creation, update, entry, matrix dispatch, most material/pose methods, and the destructor match 100%. Model's whole original text object is approximately 92%.
- Model's existing `createMatPacket` is 84.96%, `calcMaterial` 89.84%, `viewCalc` 74.22%; getter matches are lower because the compiler outlines their inline buffer helpers. `viewCalc` also outlines buffer swapping/getters; both normal-calculation branches retain the correct load/call target.
- Material shared-DL creation, count, flags, several other virtuals and NBT helper match 100%. Its `setCurrentMtx` score is 35.83% because the two-word implicit base assignment is outlined. The actual assigned words and typed fields agree.
- Cluster's position blending, track-weight dispatch, weighted deformation dispatch and normalization match 100%. Its buffer wrapper is 78.4% because swaps are outlined. Existing normal blending is 67.77%; the compiler outlines `acos_` instead of the retail inlined table lookup/conversion. The original source loops, sign flags, averaging, normalization, angular thresholds and interpolation are retained. This is not a claim of a newly perfect binary match or runtime validation of cluster assets.

These lower historical matches are disclosed rather than hidden by local compiler-flag tuning. The new inline recovery has direct instruction/call evidence. Exact source correspondence is separately enforced by `verify-source.py`.

Isolated native compilation passes for all six providers with only the production PC include hierarchy. The existing native XanimeCore, J3dAnimation, and J3dTransformAnimation consumers also compile with the restored complete Animation header. Commands and raw logs are under ignored `build/original-j3d-model-owner-20260903`. No shared xmake build or runtime test was run by this worker; parent integration gates remain required.

## Native architecture boundaries

1. Full Animation declarations remove redundant `_GXColor`/`_GXColorS10` typedef redeclarations and spell the existing real `GXColorS10` parameter alias. Data/class members and function behavior are unchanged.
2. Retail Material::getMaterialAnm rejects unsigned pointer values >=0xC0000000. Native retains rejection of that original 32-bit range (0xC0000000..0xFFFFFFFF), while accepting actual host addresses above UINT32_MAX. It does not translate an encoded Wii address into a native object. The original high-range values' producer is not recovered here; no undocumented tag meaning is assumed.
3. Retail PPCSync at 0x804A2BA4 is `sc; blr`, not a literal standalone sync instruction. The installed OSSync handler at 0x804AC21C temporarily sets HID0 bit8, executes isync/sync, restores HID0 and returns. The native provider uses `std::atomic_thread_fence(memory_order_seq_cst)` for coherent host-memory ordering; cache-range handling and GPU command publication belong to their existing separate providers. The native CPU does not emulate privileged PPC control-register writes. Both wrapper and handler were inspected from the verified DOL.

## Reproduction

Run from root:

```
python3 pc-port/notes/original-j3d-model-owner-20260903/verify-original.py
python3 pc-port/notes/original-j3d-model-owner-20260903/verify-source.py
```

The compiler script writes fresh evidence under build/. If deliberately changing an authoritative root source, review and replace the checked-in evidence snapshot after verifying the new source; the correspondence verifier intentionally rejects stale compiled-source hashes.
