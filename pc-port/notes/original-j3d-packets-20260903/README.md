# Original J3D packet and shape pipeline restoration

Verified foundation checkpoint, 2026-09-03. This is the shared SDK closure for the actual
J3DModel and authored animation owner needed by original Mario movement.
No game-time multiplier, jump impulse, or stage-specific branch is added.

The full original Packet, DrawBuffer, Shape, ShapeMtx, ShapeDraw and GD bodies
are imported under compat. Joint::entryIn and the indexed J3DSys matrix helpers
are exact original extracts. Texture SRT and texture matrix paths preserve the
original methods. Root-first additions recover VertexBuffer::setArray, GD
command writers, matrix-load tables/globals, multi-matrix NBT calculation and
the game's matrix-load hook in Overwrite.cpp.

Native architecture boundaries:
- Original GD/CPU matrix command words retain big-endian byte order. ShapeDraw
  explicitly reads/writes unaligned big-endian primitive counts.
- Native array-base updates use Aurora GXSetArrayBase, preserving stride and
  full pointers. The twelve-array VCD/VAT buffer grows from 192 to 320 bytes to
  hold Aurora's 16-byte base command in place of the 6-byte original CP command.
- J3DFifoLoadIndx uses GX command writers instead of the Wii FIFO MMIO address.
- Native J3DPacket is explicitly abstract: no base draw implementation or base
  vtable is present in retail and only real concrete packet types are used.
  Concrete Draw/Mat/Shape packet virtuals are all original implementations.
- The paired depth-sort helper, projection concatenation and overlapping 3x3
  copy retain original operation and load/store order with portable C++.
- Native includes replace the old mem.h with cstring and SDK forwarding paths;
  original alignment macros are supplied at the shared compiler boundary.

The root inline 3x3 copy helper's arguments are spelled as flat float pointers,
with explicit call-site casts. This fixes the original compiler's pointer-type
diagnostic when the recovered multi-matrix method calls that existing helper;
the pointed-to bytes and assembly operation are unchanged.

`verify-original.py` compiles eleven actual root translation units using their
configured GC 3.0a3 flags and compares them with extracted retail objects.
Recovered VertexBuffer::setArray, the game matrix-load hook, BP/CP writers and
XF header writer match 100%, including sizes. Multi-matrix NBT scaling matches
99.86487%, with the same 148-byte size. The XF word writer has a 73.021736%
match: the compiler inlines the constant zero halfword while retail calls its
halfword writer. Its nine output bytes and original order are verified in the
native packet fixture. These percentages are recorded individually; inherited
packet/shape/material implementations are not claimed to be newly exact.
`verify-source.py` enforces 21 literal imports/extracts with the explicit native
substitutions above and checks the compiled root source hashes.

The integrated macOS debug build passed for the showcase and fifteen selected
test targets. All fourteen CPU test programs in `regressions.json` passed.
The new packet fixture exercises actual single/double display-list lifecycle,
BP/CP/XF bytes, unaligned big-endian primitive counts above 255, matrix-index
insertion, repeated full twelve-array shape recording with a canary beyond the
320-byte buffer, and indexed multi-matrix NBT scaling with a 0xFFFF sentinel.
The original matrix-buffer fixture passes six groups, including a real model's
constructor/calc/viewCalc chain. The texture-matrix fixture passes four groups.
Existing Core, vertex, Binder/KCL, Aurora, clock and camera fixtures also pass.

Title and Gateway GPU smokes and the real Gateway spin checkpoint pass, with
commands and executable hashes in `runtime-gates.json`. The pre-existing real
Mario walk test still fails its grounded assertion at the planet KCL seam;
this checkpoint does not mask or fix that failure. The live renderer still
needs complete typed model resources and the original animation owner before
the recovered Mario jump/floor pipeline can be activated. Jumping is not yet
working. The shared matrix helpers have separate raw-retail arithmetic evidence
under `../original-j3d-mtx-buffer-20260903/`.

Aurora's real interrupt/scheduler critical-section provider is required by this
group; see `../aurora-os-execution-20260903/` for its SDK contract and tests.
