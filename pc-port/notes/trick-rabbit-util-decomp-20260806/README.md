# `TrickRabbitUtil` RMGK02 reconstruction

Date: 2026-08-06

## Result

Reconstructed the previously assembly-only
`TrickRabbitUtil::createRabbitFootPrint(LiveActor*)` in
`src/Game/NPC/TrickRabbitUtil.cpp`.

The helper is shared by `TrickRabbit`, `TrickRabbitFreeRun`, and
`RunawayRabbit`. It creates the original 64-entry `FootPrint`, loads
`RabbitFootprint.bti` from the owning actor archive, and preserves the target
parameters:

| Field | Value |
|---|---:|
| name | `ウサギ足跡` |
| `_2C` | `0.0` |
| `_30` | `30.0` |
| `_34` | `30.0` |
| `_38` | `100.0` |

## Target evidence

The source was reconstructed directly from
`build/RMGK02/asm/Game/NPC/TrickRabbitUtil.s`. The target function is one
`0x94`-byte routine and contains the same allocation, constructor, texture
lookup, member stores, and return sequence.

## Verification

```text
python3 configure.py -v RMGK02 --build-dir build
ninja build/RMGK02/src/Game/NPC/TrickRabbitUtil.o
[1/1] MWCC build/RMGK02/src/Game/NPC/TrickRabbitUtil.o

objdiff unit: main/Game/NPC/TrickRabbitUtil
.text: 148 / 148 bytes
function similarity: 99.72973%
.data similarity: 100.0%
```

The only code difference reported by objdiff is a constant-pool relocation
identity; the emitted operation and value are unchanged. No inline assembly
or unrelated engine changes were used.
