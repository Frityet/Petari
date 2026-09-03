# Original material projection controllers

This root-only tranche restores the real controller path required by DisplayListMaker. It does not activate native ModelManager, change Mario animation selection, or provide a substitute screen/shadow texture.

Changed root files:

- `include/Game/LiveActor/MaterialCtrl.hpp`: replaces ProjmapEffectMtxSetter's opaque 0x3C bytes with the original typed layout and declares its existing virtual update/base-position query. The type remains 0x48 bytes under the original compiler. UpdateEffectMtxInfo is the original 0x34-byte record: J3DTexMtxInfo pointer followed by a TPos3f. The controller owns its record pointer at 0xC, count at 0x10, base matrix at 0x14, and borrowed J3DModel pointer at 0x44.
- `src/Game/LiveActor/MaterialCtrl.cpp`: restores ViewProjmapEffectMtxSetter constructor/update, MarioShadowProjmapMtxSetter constructor/update, and the actual MaterialCtrl::updateMaterial base virtual. Existing ProjmapEffectMtxSetter source is now compileable with its real declaration and follows the verified original matrix-copy and field-access order.

The base virtual's body is empty because retail is exactly `blr` at 0x801685E8. This is an original implementation, not a native fallback. Every concrete controller in this group overrides update and owns/uses the actual original state.

## Verification

Run:

```sh
python3 pc-port/notes/original-material-controllers-20260903/verify-source.py
```

The script compiles the complete root MaterialCtrl.cpp with configured GC3.0a3 Game flags through sjiswrap, splits the verified RMGK01 DOL, and checks 15 functions plus all four relevant vtables. Five function bodies are newly restored; the other ten include the existing Projmap lifecycle and base/MatColor methods. Fourteen functions have identical retail instruction bytes after actual relocation. `updateMtxUseBaseMtxWithLocalOffset` is 97.70% objdiff with only a bijective floating-register exchange and the order of three independent scalar input loads; every store, arithmetic operation and call is retained. View projection and shadow updates differ in objdiff only through constant-label naming and have byte-identical resolved instructions.

`source-evidence.json` retains source hashes, function addresses/sizes, canonical instructions, relocation targets and vtable slots. Exact commands, compile logs, original and rebuilt objects, disassembly and objdiff JSON are under `build/original-material-controllers-20260903/`. DOL SHA1: `25c5959534b3c21246c6c7e42021b916b41fb578`.

No shared native build, native imports, runtime changes, commits or GPU tests were performed by this subtask.

## Ownership and exact behavior

ViewProjmapEffectMtxSetter scans every material and its eight texture-matrix slots. It retains pointers only for nonnull, actually used matrices with `(mInfo & 0x3f) == 9`. Its original constructor uses a local 64-pointer array, then allocates and copies the exact collected count. Native model-resource validation must establish that an input does not exceed this real 64-entry aggregate footprint before running the unchanged constructor. Do not clamp the count or silently omit matrices.

Each update reads the live camera projection, preserves its first two rows, sets row 2 to `(0, 0, -1, 0)` and row 3 to `(0, 0, 0, 1)`, then calls the original J3DTexMtxInfo::setEffectMtx for every retained matrix. Its pointers must remain valid for the controller lifetime, and camera projection must come from the actual active camera owner.

ProjmapEffectMtxSetter separately selects used mode-8 matrices. It counts them, allocates the original record array, retains each actual TexMtxInfo pointer, and copies the first three rows of the ResourceHolder's original backup effect matrix into the corresponding record. The backup and the live material matrix are intentionally different objects: later updates concatenate that copied baseline with the controller's current base matrix and publish the result into the live material. Base-matrix updates use the original model's raw base TR; the local-offset variant first concatenates the authored local translation and then inverts. No actor-name special case is involved.

MarioShadowProjmapMtxSetter owns a real ProjmapEffectMtxSetter created on the same actual model and resource holder. Its update performs this original sequence:

1. Read the actual player position and the model's base translation, then compute model minus player displacement.
2. Read the published Mario shadow vector. Start the projection-side distance at -1. Only when `abs(length - 1) < 0.001` does the original vecKillElement calculate the signed component along that vector and remove it from the temporary displacement. It does not normalize an arbitrary vector or replace an absent owner.
3. For a strictly positive component, make translation by the negative player position. Otherwise use the original `(1000000, 1000000, 1000000)` translation. This includes zero distance, non-unit vectors, and unordered length tests.
4. Read and negate the actual player shadow Euler rotation, call the original makeMtxRotate, concatenate rotation times translation, store the inner controller's base matrix, and invoke its virtual update.

This controller does not allocate or fake the shadow image. The actual CollisionShadow texture owner and original DrawUtil published shadow globals remain required, as described in `../original-model-manager-owner-20260903/README.md`. The ResourceHolder migration agent confirmed that its exact backupInitMaterialData creates a separate retained Mtx44 array and offers a shared archive-owner lease; a future native controller owner must hold that lease together with its actual model/allocation owner.

## Remaining native/source boundary

These projection controllers and their exact vtables are source-ready. Full material-controller import still needs original FogCtrl constructor/update, TexMtxCtrl constructor/updateMaterial/setTexMtx, and optional MirrorReflectionMtxSetter closure. Their absence must remain visible; do not substitute empty derived virtuals. AnmPlayer/material-player and J3DMaterialTable attachment imports remain part of the separately planned ModelManager integration.

When the original controllers are eventually enabled, validate actual model/material pointer identity, the independent initial-effect backup, camera/shadow provider ordering, multiple models sharing a holder, and retirement of the model/holder after every controller. Direct original J3D packet/GX submission can consume these mutated materials without a second parsed animation evaluator.
