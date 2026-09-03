# Original Mario screen, fog, and alpha drawing recovery

Frozen root-first recovery for the original native model activation. Seven
methods cover 3,624 retail bytes. The original GC3.0a3 compiler comparison has
91.928% weighted objdiff match. This is source/compiler evidence, not a native
runtime or visual claim.

| Method | Retail bytes | Compiled bytes | Match |
| --- | ---: | ---: | ---: |
| calcScreenBoxRange | 1220 | 1256 | 84.702% |
| calcFogLighting | 736 | 728 | 91.973% |
| updateAlphaDL | 520 | 520 | 99.885% |
| updateSimpleAlphaDL | 208 | 208 | 100% |
| updateReflectAlphaDL | 312 | 312 | 99.808% |
| updateLightDL | 584 | 584 | 92.192% |
| DLchanger::swap | 44 | 44 | 100% |

The six methods other than screen bounds preserve the complete retail direct-call
order. Loaded float constants agree. The alpha RGB initializer bytes are zero in
both versions; the recovered compiler places the static zero color in NOBITS
storage, which the evidence reader correctly interprets as zero bytes.

Screen bounds preserve the actual 80-unit world-axis projections and derive the
radius from the three projected lengths. The recovered source preserves strict
min/max comparisons, signed-16-bit rectangle dimensions, even-pixel alignment,
256-pixel limits, zero-to-two dimension adjustment, and framebuffer intersection.
The lower score comes from current root header inlining of vector assignment and
2D addition, TBox construction dispatching to its literal set method, and resulting
register/stack choices. The evidence explicitly records these helper-call
differences; this method is not claimed as an exact instruction match. No
matching-only compiler pragmas or conditional native substitutes were introduced.

Fog preserves damage, star-piece, and item-dash branches; the original triangular
pulse and sine calculations; ambient/material/fog colors; and the final actual
updateLightDL call. Light and alpha routines issue original GD commands, round the
copy range to 32 bytes, copy into their real DL buffers, and store the cache range.
DLchanger::swap now returns its actual DLholder pointer and uses the original
modulo-buffer selection. No synthetic display-list wrapper is involved.

## Files and integration

Root files already changed:

- src/Game/Player/MarioActorSpecialDraw.cpp
- include/Game/Player/DLchanger.hpp (swap return declaration)
- libs/RVL_SDK/include/revolution/gd/GDPixel.h (two missing SDK declarations)
- include/Game/Player/MarioActor.hpp (screen coordinate pairs become TVec2f)
- src/Game/Player/MarioActorInit.cpp (matching x/y initialization)

The last two are shared with gateway's separate blur-bank and AreaObj pointer
corrections. `root.patch` contains only this task's screen-member hunks for those
files, preserving the independent changes. Existing initMember and initScreenBox
instruction bytes remain identical before/after the typed-field corrections,
including the concurrently applied equivalent gateway changes: 2,156 and 40 bytes.
See `initializer-proof.json` for the references and hashes.

`native.patch` is a minimal patch against native production at staging time.
Its full-file mirror is under build/original-mario-special-draw-20260903/staged.
Apply hunks rather than replacing the shared native MarioActor header wholesale.
The patch additionally imports the literal original TVec2 min/max/isAbove and
TBox2 helpers into the native geometry layer. It corrects the stale native
GXFrameBuf include to Aurora's existing GXFrameBuffer header. It does not modify
Aurora behavior. The two affected native translation units compile successfully;
`native-compile.json` records the exact commands. No xmake/shared build was run.

`verify-original.py` compiles the actual root source and compares the verified
retail object. DOL SHA-1 is 25c5959534b3c21246c6c7e42021b916b41fb578. Evidence includes
addresses, hashes, every actual relocation, constants and direct calls. The ROM
itself is never copied into notes or staged output.

## Remaining direct closure

The strongest small follow-up in this same path is actual hideBeeFur (160 retail
bytes) and showBeeFur (180 bytes), still missing and called by view/morph behavior.
Original initScreenBox also still has its 0x80000 aligned allocation commented
out: its existing 40-byte body is shorter than retail's 92 bytes. That allocation
and its retained scene lifetime need closure before claiming screen capture works.
Those routines were not included in the authorized seven-method recovery.
MR::calcFogStartEnd already has a root body and should be selected as an original
provider if the final native link requires it; it should not be fabricated here.

Reproduce:

```
python3 pc-port/notes/original-mario-special-draw-20260903/verify-original.py
python3 pc-port/notes/original-mario-special-draw-20260903/stage-native.py
```
