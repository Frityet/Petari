# Original WPadHVSwing recovery — 2026-09-10

Recovered all three missing methods in `decomp/src/Game/System/WPadHVSwing.cpp` before copying the complete source and header unchanged into the native `src/Game/System/` tree. No Game-specific input substitute was added. Parent owns original WPad activation and the complete linked build; this task did not run Xmake or commit.

## Retail and compiler proof

The authoritative assembly/object is under `notes/gateway-audit-20260907/restoration/retail/{asm,obj}/Game/System/WPadHVSwing.*`. `retail-byte-proof.json` checks every instruction in all four functions directly against `decomp/build/compat-math-oracle/main.dol` (SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`).

| Function | Retail address | Retail / recovered bytes | Wii objdiff |
| --- | --- | --- | --- |
| Constructor | 0x803ACCA0 | 68 / 68 | 99.70588% |
| updateSwing | 0x803ACCE4 | 264 / 264 | 100% |
| updateCentrifugal | 0x803ACDEC | 408 / 408 | 99.95098% |
| update | 0x803ACF84 | 140 / 140 | 100% |

The remaining differences are only the constant pool relocation mapping: constructor `.4f` and centrifugal initial `0.0f`. The recovered constant pool contains exact float bits `3f800000 3ecccccd 00000000` (1, .4, 0); retail order is 1, 0, .4. `objdiff-summary.json` records the two changed relocation operands. No instruction, branch, register, timer threshold or vector arithmetic differs in the three recovered methods.

Both baseline and recovered full translation units compile with the reference Metrowerks recipe (exit 0); the exact command/result is in `wii-results.json`. Native LLVM23 full translation-unit compilation also exits 0 (`native-compile.json`). The source and header copies are byte-identical, with hashes in `source-manifest.json`. This is decompilation and isolated compile proof, not a claim of full WPad gameplay integration or physical input verification.

## Preserved behavior

- Swing compares current acceleration with history index 20 using distance **greater than or equal to** `_8`. Quiet duration greater than 6 clears the retained swing state; its duration advances while retained. Unavailable history/current clears only instantaneous `mIsSwing` and returns, preserving all retained state/timers.
- Centrifugal processing initially attempts history index 20 and current acceleration. Failure clears its four fields but **continues**, exactly as retail. With at least 15 history samples it sums 14 successive Y differences and retains the highest prefix sum. The threshold is strictly **greater than** `_1C`. Quiet duration greater than 8 clears retained centrifugal state. With insufficient history after initial failure, the continued quiet path leaves `_28` equal to 1.
- `update()` calls swing then centrifugal, clears trigger, and triggers on swing duration 1 or on centrifugal durations 30, 45, 60, … while quiet duration remains below 15.
- Four timer fields `_10`, `_14`, `_24`, `_28` are now `s32`, preserving their offsets and widths. Retail uses signed `cmpwi` and signed `divw` for the remainder operation; unsigned declarations misdescribe the original arithmetic.

## Activation dependencies

The native object requires only `WPad::getAcceleration`, `WPad::getPastAcceleration`, and `WPad::getEnableAccelPastCount`. These belong to the complete original WPad owner being integrated by the parent. No raw Aurora-stick/acceleration query was introduced inside Game, and no history-error sanitization was added.
