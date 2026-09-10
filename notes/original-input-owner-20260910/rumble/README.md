# Complete original WPadRumble source, 2026-09-10

Recovered in `decomp/src/Game/System/WPadRumble.cpp` first, following `decomp/AGENT_DECOMP_GUIDE.md`, then copied byte-for-byte into new native `src/Game/System/WPadRumble.cpp`. The existing reference/native headers already contain the complete declarations and were not changed. Parent owns removal of old native providers and callback-array lifetime integration. No Xmake, commit, or push was performed here.

Four retail-proven changes complete the source:

- `RumbleChannel::update` reads `mPattern[_C]` during progression, rather than replaying byte zero every frame. The looping restart still reads byte zero and sets the cursor to one. Retail `0x803AD73C–0x803AD754` adds the signed cursor to the pattern address and normalizes the selected byte to bool.
- `WPadRumble::updateRumble` uses the existing original `MR::clamp(_B0 - 1, -9, 5)`. Retail `0x803ADAC0–0x803ADAEC` clamps the decremented value. This allows the idle counter to become negative, reset accumulated rumble duration, and retain the original 1800-frame cooldown behavior.
- Both insertion branches wrap `_C` to 31 bits using `(_C + 1) & 0x7FFFFFFF`, matching `clrlwi ...,1` at `0x803ADC20` and `0x803ADC58`.
- `findRubmlePattern` is restored completely from `0x803ADC90–0x803ADD48`. It intentionally does not use the requester argument. It checks active pattern hashes, immediately reports an existing match, otherwise remembers the first empty channel and the lowest unsigned sequence among occupied channels. Equal sequence values preserve the earlier channel. A free slot takes precedence; only its selected output pointer is written, leaving the caller's other sentinels untouched.

The rest of the original source was retained. In particular, pause/stop distinctions, cooldown state transitions, registration, static callback dispatch, and native motor support remain governed by the original methods and their SDK boundary, not a new host rumble state machine.

## Evidence

Both baseline and recovered complete RMGK01 translation units compile with exit 0 using GC/3.0a3 through wibo/sjiswrap, the established Game flags, and original headers. Exact commands, source snapshots, full object diffs, and compact results are recorded by `compile-original.py` and `*-compile.json` / `*-proof.json`.

| Original function | Before | Recovered |
| --- | ---: | ---: |
| RumbleChannel::update | 87.11539% | 100% |
| WPadRumble::updateRumble | 98.655914% | 100% |
| WPadRumble::setRumblePatternIfNotExist | 96.44068% | 100% |
| WPadRumble::findRubmlePattern | absent | 99.3617% |
| Whole .text, 1656 retail bytes | 87.028984% | 99.927536% |

All 14 original functions are present. The other ten functions remain 100%; data and BSS match 100%. The six remaining findRubmlePattern instruction differences are register-operand assignments (`r9`/`r10` roles), with the same instruction sequence and size (188 bytes); no behavior was changed to chase the final register allocation.

The exact native mirror compiles with LLVM 23, exit 0 (`native-compile.json`). `RumbleChannelProbe.cpp` links the actual original full source object with unused WPadRumble methods discarded, so it needs no fake WPad owner, SDK callbacks, or Game helper definitions. Its original field/sequence/pattern lifecycle checks run under ASan/UBSan: baseline fails three cursor progression cases (9/12), recovered passes 12/12 with exit 0. It checks nonzero-byte normalization, intervening zero frames, later activation after a leading zero, declared duration, looping restart, stable requester/pattern identity, and clear behavior.

This isolated runtime proof covers the pure original RumbleChannel. Whole WPad construction, motor routing, and asynchronous callback lifetime are parent integration work and are not claimed tested by this fixture.
