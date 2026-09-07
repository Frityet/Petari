# PPC bitfield ABI boundary, 2026-09-07

Restoring original Mario exposed a real architecture mismatch. Native Clang allocated the first named unsigned bitfield at bit 0, while original Wii masks address bit 31. The prior native test using the actual header produced `MovementStates::jumping = 0x00000001` and `DrawStates::_5 = 0x00000020`; retail uses `0x80000000` and `0x04000000`. Mixed named/raw operations occur throughout Mario.cpp, MarioMove.cpp, MarioJump.cpp, MarioCollision.cpp, and MarioActor.cpp, including grounded/jump, previous-frame, and rendering state.

The shared `aurora/ppc_bitfield.hpp` declaration helper accepts each complete storage unit once, in original MSB-first field order. It emits forward declarations for the Wii compiler/big-endian hosts and reversed declarations for little-endian hosts. It preserves multi-bit field widths and numeric values; a bit-reversal operation on raw words would incorrectly reverse values within Mario's two-bit `_3E` field. Raw Game union members remain plain u32 and every Game cpp operation remains unchanged. The macro introduces only fields and a compile-time width check: no runtime proxy, allocation, alternate backing state, or mask-conversion code.

The native Mario.hpp replaces its three original storage groups with the shared helper. MovementStates has 63 named fields covering 64 bits: 32 single-bit fields in the first word, 30 single-bit fields plus `_3E:2` in the second. DrawStates has 32 single-bit fields. The same helper replaces J3DModelX.hpp's preexisting handwritten host-order branch; its 29 flags plus three padding bits occupy one word. Binder and StarPiece also declare bitfields, but have no named/raw union alias in these headers and are outside this bounded migration.

| Original Mario member | Original Wii offset | Canonical raw word aliases |
| --- | --- | --- |
| mMovementStates | 0x08 | mMovementStates_LOW_WORD, mMovementStates_HIGH_WORD |
| _10 | 0x10 | _10_LOW_WORD, _10_HIGH_WORD |
| mDrawStates | 0x18 | mDrawStates_WORD |
| _1C | 0x1C | _1C_WORD |
| _20 | 0x20 | _20_LOW_WORD, _20_HIGH_WORD |

`canonical-field-masks.json` enumerates every field, width, word, shift, and original numeric mask. The `_28`, `_2C`, and `_34` previous-frame snapshots remain plain raw words; they need no separate conversion once the declaration layout agrees.

Validation:

- `tests/PpcBitfieldAbiTests.cpp` checks all 124 named fields: 63 MovementStates, 32 DrawStates, and 29 J3DModelX fields. Each test checks named-to-raw and raw-to-named behavior, preserves unrelated bits/words, and checks every representable value including all four `_3E` values. Sizes and alignments remain 8/4, 4/4, and 4/4 respectively. Additional 8-bit and 16-bit storage groups check padding and multi-bit values.
- Native standalone Clang builds and runs pass at both `-O0` and `-O2`; no scene, renderer, or disc is involved. The regular Xmake target is `smg-pc-ppc-bitfield-abi-tests` (parent coordinates its build).
- The original GC 3.0a3 Wii compiler freshly compiled 248 getters/setters using unchanged reference declarations, then using a temporary reference-header overlay containing the shared helper. **All 248 function byte sequences are identical**, with exact hashes in `wii-equivalence.json`. No production decomp header is modified.
- Incomplete, overflowing, and zero-width groups are rejected by the shared helper. The group contract requires explicit full-width storage units and supports up to 32 fields per unit.

`verify.py` reproduces the proof and records commands/results in `verification.json`. `make-probes.py` creates the field fixtures and temporary Wii overlay; `generate-header.py` records the mechanically generated macro arities. All generated objects/binaries are local notes output. This proves the shared bitfield representation boundary; it does not by itself establish successful Mario gameplay.
