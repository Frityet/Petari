# MarioActorWipe RMGK02 recovery

This lane reconstructs the root `src/Game/Player/MarioActorWipe.cpp` unit from
the RMGK02 retail assembly. It intentionally changes no shared header or PC
port source.

## Recovered behavior

- `MarioActor::drawPreWipe()` preserves the retail two-gate wipe behavior. If
  the actor is outside the special pre-wipe state and its wipe flag is clear,
  drawing returns immediately. Otherwise it draws the model with the normal
  view, projection, and actor-light setup.
- When the overlay is active, the routine fills the EFB-sized screen rectangle
  before drawing Mario. The fill value is `0xE0` above the `0x80` threshold and
  is the retail `value + value / 2 + value / 4` ramp at or below it.
- The ten `NrvMarioActor` singleton instances are emitted in retail order.

## Objdiff result

The complete retail `.text` is 388 bytes. The final focused comparison reports
**99.896904%** for `.text`, **100%** for `.ctors`, and **100%** for `.sdata2`.

- `MarioActor::drawPreWipe() const`: 280 bytes, **99.85714%**
- `__sinit_\\MarioActorWipe_cpp`: 108 bytes, **100%**

Every generated `drawPreWipe` instruction aligns with retail. The only two
reported argument mismatches are relocations for the same signed-integer to
floating-point conversion constant: the extracted retail object references
the shared `lbl_80539CB8`, while a standalone recompilation names its identical
local compiler pool `@3230`. The emitted instruction sequence and constant
bytes are otherwise the same.

The initial reconstruction placed the alpha calculation in a named local,
which compiled to 92.78571% for `drawPreWipe`. Passing the conditional retail
expression directly as the third `drawFillBox` argument restores the original
register allocation and instruction schedule.

See `objdiff-summary.txt` and `verification.log` for the focused metrics and
reproducible commands.
