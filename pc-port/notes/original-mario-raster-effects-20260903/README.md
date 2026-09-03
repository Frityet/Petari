# Original Mario raster and cold-water draw effects

Root-first recovery after `7a2a682c3`, covering both remaining raster loops:

| Method | Retail / compiled bytes | Original compiler objdiff |
|---|---:|---:|
| drawColdWaterDamage | 848 / 848 | 99.363205% |
| drawRasterScroll | 804 / 804 | 98.631840% |

All relocation offsets, kinds, addends and resolved call/constant targets are identical for both methods. The original external f64 conversion constants at 0x80539A80 and 0x80539A88 are read from the verified RMGK01 DOL and checked against their exact values. Unlike the preceding screen checkpoint, these are **high-fuzzy matches, not exact instruction matches**: the remaining differences are register assignment and operand ordering within commutative floating additions/multiplications. Every non-relocation instruction difference is preserved in compiler-evidence.json, with the full original object comparison under build/. No matching-only annotations or native substitutes were used.

Both methods keep the original four-line RGB565 capture strips, alternating buffer index, GX synchronization, per-line position/texture emission and final GXDrawDone. Cold-water displacement uses the original swim timer, 89*sin(2π*timer/120) negative phase clamp and nested screen-height expression. Raster scroll retains the separate zero guards, period/wavelength divisions and nested sine wave. The method arguments and floating constants preserve the observed original operation order.

Retail indexed loads establish that the adjacent FBO pointers at 0x1D8 and 0x1DC form one actual array. Root now declares `FBO* mRasterBuffers[2]`; the existing allocations and initializers use indexes 0/1. Reconstructing the previous two-pointer representation and compiling both versions proves **init2 (1,688 bytes) and initMember (2,156 bytes) have identical raw instructions and identical relocations**. Thus the storage correction fixes C++ indexing without changing the Wii allocation/initialization algorithm.

The minimal four-file native patch copies SpecialDraw unchanged and changes only the buffer pointer declaration and existing allocation/initialization field names in native MarioActor.hpp/.cpp and MarioActorInit.cpp. All three affected native TUs compile with current native Game flags. No native production, xmake/source selection, shared build or GPU runtime was changed by this work.

Root checkpoint files: src/Game/Player/MarioActorSpecialDraw.cpp; include/Game/Player/MarioActor.hpp; src/Game/Player/MarioActor.cpp; src/Game/Player/MarioActorInit.cpp; this notes directory. `verify-original.py` reproduces source and architecture proof; `stage-native.py` regenerates the staged compile and reviewable patches under build/original-mario-raster-effects-20260903/staged.

The actual array allocation lifetime still needs the parent's external native scene ownership. drawWallShade is owned by the separate shadow recovery. Native full-model linking/runtime is a separate integration gate; these compiler results do not establish playable Mario movement or complete rendering.
