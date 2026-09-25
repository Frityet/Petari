# Canonical J3D draw/record owners

Restored the complete current donor `J3DDrawBuffer.cpp`, `J3DStruct.cpp` and `J3DGD.cpp` under `src/JSystem/J3DGraphBase/`, deleting their three compat CPPs. Restored the two XF writers to their original inline definitions in canonical `J3DGD.hpp`. All four existing touched files were clean initially; baseline, snapshots, exact patch and manifest are adjacent. No Game, Aurora, animation, resource, build-wiring, index or commit changes made by this lane.

## Original behavior and native boundaries

- **DrawBuffer** is byte-exact current donor, including all six packet sort modes, batching by actual material/animation identity, original table order and depth calculation. Its donor already hashes through `uintptr_t`. The native header's existing explicit-FMA `J3DCalcZValue` was left untouched.
- **Struct** retains the complete donor and original paired-single branches. Native branches retain previous center copy and six-component load-before-store assignment, and extend a supplied 3x4 effect matrix with exactly `(0,0,0,1)`. Fog and NBT assignments are restored to their donor bodies. Semantic field/padding boundaries remain original. This adds no JMath header changes or duplicate helper owner.
- **GD** retains the full donor. Five GD includes use the Aurora Dolphin include spelling. The fog-adjust argument remains `u8` to match the original header and Wii byte semantics; Aurora's `TARGET_PC` `GXBool` is C++ `bool`, so using it here would change the current byte-packing interface. The original signed exponent cast is restored.
- **XF command emission** now comes from the original canonical header definitions. `J3DGDWriteXFCmd` still writes the same opcode, length, big-endian address and value; `J3DGDWriteXFCmdHdr` retains the original length-minus-one representation. The removed out-of-line duplicate functions are not retained anywhere.

Compile all three canonical TUs with `-ffp-contract=off`, matching the original JSystem build in `decomp/configure.py`. This prevents accidental scalar contraction while leaving explicit FMA in the existing depth helper intact. Exact additions/removals/flags are in `build-wiring.json`; root owns wiring and execution.

## Verification

`python3 notes/compat-owner-removal-round5-20260925/draw/validate-source.py` passes **110 checks across 49 uniquely owned definitions**. Checks compare complete function sets, byte-exact donor DrawBuffer, donor GD modulo explicit platform boundaries, original-architecture Struct branches, both exact original XF header bodies, single production providers and deletion snapshots. `git diff --check` passed for this source batch. No compilation or runtime pass claimed by this lane.

Existing focused regression targets require no fixture changes:

- `smg-pc-original-j3d-packet-tests`: six groups, including exact BP/CP/XF command bytes, actual display-list buffers, native arrays and paired copy boundaries.
- `smg-pc-original-j3d-texture-mtx-tests`: four groups, including original unwritten bytes, SRT rotation word, affine effect row and texture-matrix forwarding.
- `smg-pc-original-j3d-material-block-tests`: six groups, including real GD fog register words, disabled table reads, field/padding preservation and indirect self-assignment.

## Deferred override

`J3DShapeMtxGameCompat.cpp` implements one exact original Game override from `Game/System/Overwrite.cpp`. That complete donor also contains the Game's heap, JPA and audio overrides (already split among other providers/owners). Creating another partial Overwrite file would not restore the full owner and risks duplicate method ownership. Leave this provider for a coordinated complete Overwrite closure. No animation or loader surfaces were touched.
