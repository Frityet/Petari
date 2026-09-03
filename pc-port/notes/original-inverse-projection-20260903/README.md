# Original TDDraw inverse projection recovery

Recovered only root `TDDraw::invProject` in `src/Game/Util/DirectDraw.cpp`, retail `0x80404078`, size `0x234` (564 bytes / 141 instructions). The existing declaration in `include/Game/Util/DirectDraw.hpp` is unchanged. Added `Game/Util/SystemUtil.hpp` to declare the actual `MR::isScreen16Per9` call. There are no native production imports, Game owner substitutions, SDK changes, shared builds, or GPU/runtime claims in this tranche.

## Callers and data types

The function takes an output `TVec3f*`, screen/depth `const TVec3f&`, original 3x4 view matrix (`MtxPtr`), the seven floats returned by `GXGetProjectionv`, the six floats returned by `GXGetViewportv`, and a boolean controlling whether input Z is already normalized.

- `StarPointerController::storeDataFromCallback` (`src/Game/Screen/StarPointerController.cpp:44`) constructs `(mInfo.mPos.x, mInfo.mPos.y, float(mInfo.mZDepth))` and calls the function with `false` after a ready original PeekZ callback. Its output is the actual controller's retained `mWorldPos`.
- `TalkPeekZ::drawSyncCallback` (`src/Game/NPC/TalkDirector.cpp:57`) reads actual `GXPeekZ` at converted framebuffer coordinates, then calls the same function with its screen coordinates, original camera view matrix, and `false`. Its header's `_20` plus 24 bytes of padding holds seven projection floats; `_3C` plus 20 bytes holds six viewport floats. No fabricated struct or expanded header layout is needed.
- `GXGetProjectionv` / `GXGetViewportv` (`src/RVL_SDK/gx/GXTransform.c:99,285`) establish the array layouts. Projection is `[type, P00, P02/P03, P11, P12/P13, P22, P23]`, with type zero for perspective. Viewport is `[left, top, width, height, nearZ, farZ]`. The original StarPointerPeekZ header declares those exact 7/6-element arrays.

The current callers both pass false. The true branch is nevertheless recovered and verified: it consumes input Z directly instead of dividing by `16777215.0f` (`0x4B7FFFFF`). The binary's other constants are `0.5f`, positive zero, and the signed-integer-to-float conversion bias `0x4330000080000000`; every referenced constant was checked in the current DOL.

## Preserved original behavior

The source retains the exact scalar arithmetic order visible in the binary:

1. Normalize raw 24-bit depth only when requested by the boolean.
2. Compute the original projected-Z and inverse-W intermediates from projection entries 5/6 and viewport near/far. This common computation executes for **both** projection types; it is not replaced with a mathematically redesigned unprojection formula.
3. In the wide-screen branch, call `getScreenWidth` followed by `getFrameBufferWidth` and scale only screen X by framebuffer-width/screen-width before removing viewport center. Standard-screen X is used directly. Y uses the original sign inversion around viewport center. Both divisions retain their original order and intermediate float rounding.
4. The zero projection-type branch subtracts Z-dependent projection offsets; the nonzero branch subtracts the literal translation offsets. Both compute the original view-space Z.
5. Call actual `PSMTXInverse(view, localInverse)` followed by `PSMTXMultVec(localInverse, viewPoint, output)`. As in retail, the inverse return status is ignored. No clamping, fallback matrix, guessed plane, epsilon, or revised projection behavior is added.

No new allocation or field layout is involved. The actual Star Pointer owner, its update/depth callback scheduling, and publication through GameSystem remain a separate prerequisite for activating CameraDPD. This recovery closes one real source dependency; it does not install that owner.

## Original compiler and complete instruction evidence

Reproduce from the repository root:

```sh
python3 pc-port/notes/original-inverse-projection-20260903/verify-original.py
```

The script compiles the actual root `DirectDraw.cpp` against the real root include hierarchy with GC3.0a3, `configure.py` Game flags, and the Shift-JIS wrapper. No generated header overlay or analytic host replacement is used. It compares the original object and the current RMGK01 DOL (SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`).

Result: **93.61702% objdiff**, with identical compiled/retail function size, `0x234`. `compiler-differences.txt` records the actual differing assembly. Differences are the arrangement of the depth-normalization branch, FPR allocation (notably the two retained intermediates), constant-symbol names, and independent load scheduling.

For stronger functional evidence, the verifier relocates every actual compiled call and constant to its verified retail address, then interprets **every instruction** in both routines. It compares all eight complete control paths, covering both values of each of the three branches. The comparison preserves each float addition/subtraction/multiplication/division rounding node, sign inversion, original predicate, constant bit pattern, helper call identity/order/arguments, output write, stack balance, and saved-register restoration. Only operand order within a single finite add/multiply is canonicalized; no expression reassociation, contraction, division simplification, or algorithmic instruction is ignored. Every one of the 141 instruction positions is exercised in each routine across those paths.

All **eight symbolic paths match**. Original matrix inverse/multiply calls remain identical abstract helper nodes with the same matrix/point/output arguments; the verifier does not replace them with a new host algorithm or claim to test those SDK implementations here. Floating-point exception state and signaling-NaN payload details are outside the symbolic comparison's finite arithmetic scope.

`source-evidence.json` contains the command, exact hashes, relocation destinations and eight per-path graphs. Reproducible compiler outputs, original/relocated bytes and full objdiff remain under ignored `build/original-inverse-projection-20260903/`. The earlier camera registry audit predates this root-only recovery; its previous missing-invProject entry is closed by this note, while the native owner boundary remains unchanged.
