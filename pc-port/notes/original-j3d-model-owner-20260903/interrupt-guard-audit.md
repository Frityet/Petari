# Retail static interrupt guards

Read-only verification on 2026-09-03 of a suspected source-decompilation bug in
`J3DModelData::indexToPtr` and `J3DShape::makeVcdVatCmd`. Both root functions have
`static BOOL sInterruptFlag = OSDisableInterrupts();`. The verified retail DOL
contains exactly that first-call guard, so no root or PC source change is made.

DOL SHA1: `25c5959534b3c21246c6c7e42021b916b41fb578`.

| Function | Retail range | Guard and stored status | Scheduler/restore |
| --- | --- | --- | --- |
| `J3DModelData::indexToPtr` | `0x80431AF0`, `0xCC` bytes | `80431B04` loads guard from `r13-9084`; `80431B20` skips initialization if nonzero; `80431B24` calls `OSDisableInterrupts`; `80431B2C` stores returned status at `r13-9088`; `80431B30` stores guard1 | `80431B34` always calls `OSDisableScheduler`; `80431B98` calls `OSEnableScheduler`; `80431B9C` loads the saved first-call status; `80431BA0` calls `OSRestoreInterrupts` |
| `J3DShape::makeVcdVatCmd` | `0x80426DEC`, `0xA0` bytes | `80426E00` loads guard from `r13-9116`; `80426E08` skips initialization if nonzero; `80426E0C` calls `OSDisableInterrupts`; `80426E14` stores returned status at `r13-9120`; `80426E18` stores guard1 | `80426E1C` always calls `OSDisableScheduler`; `80426E6C` calls `OSEnableScheduler`; `80426E70` loads the saved first-call status; `80426E74` calls `OSRestoreInterrupts` |

Call destinations are actual `OSDisableInterrupts=804A9778`,
`OSRestoreInterrupts=804A97A0`, `OSDisableScheduler=804AC580`, and
`OSEnableScheduler=804AC5BC`. The two methods have separate static guards and
saved status variables. The existing ModelData original-compiler comparison
also reports100% for the entire `indexToPtr` function.

This means subsequent invocations restore the originally saved status without
performing a fresh interrupt disable; the scheduler calls still happen every
time. OS compatibility must preserve its actual enable/restore API, rather than
assume every restore has a paired disable in the same invocation. Replacing the
static local with an automatic local would change the original behavior.

Raw evidence is reproducible with:

```
python3 build/compat-math-oracle/disassemble_dol.py 0x80431AF0 0xCC build/original-shape-packet-user-data-20260903/ModelData-indexToPtr-retail
python3 build/compat-math-oracle/disassemble_dol.py 0x80426DEC 0xA0 build/original-shape-packet-user-data-20260903/Shape-makeVcdVatCmd-retail
```

The `.bin`, `.o`, and `.asm` files are ignored build artifacts. The raw integer,
branch, call, and load/store instructions here do not depend on LLVM's imperfect
paired-single decoding.
