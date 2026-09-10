# Original WPadLeaveWatcher recovery — 2026-09-10

Recovered the missing `WPadLeaveWatcher::update()` in the reference source first, then copied the complete source unchanged to native. Source scope is only `decomp/src/Game/System/WPadLeaveWatcher.cpp` and `src/Game/System/WPadLeaveWatcher.cpp`. Headers required no change and already match exactly.

## Validation

The retail source of truth is `notes/gateway-audit-20260907/restoration/retail/{asm,obj}/Game/System/WPadLeaveWatcher.*`. All five functions are **100% Wii objdiff matches**, including the recovered `update()` at **0x803AD0EC**, **240 bytes**. `objdiff-before.json` preserves the missing-function baseline; `objdiff-recovered.json` is the final result. Both full-TU Wii compiles exit 0, with exact commands in `wii-results.json`. The complete native TU compiles with LLVM23, exit 0 (`native-compile.json`).

`retail-byte-proof.json` independently verifies every instruction of all five functions against the real `main.dol` (SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`). It also verifies the four enabled activity flag bytes at 0x806B25B0 are `01 01 01 01`; the pointer flag is the zero-initialized SBSS byte at 0x806B6F20. `source-manifest.json` records source/header hashes and identical reference/native copies.

This task did not run Xmake, a linked runtime fixture, or a physical-input test, and did not commit. Parent owns the complete original input owner integration and its runtime proof.

## Original behavior retained

The watcher returns immediately while suspended. It then checks, in the exact retail short-circuit order:

1. Pointer `_45` activity and its mutable enable flag (default false).
2. Core acceleration nonstationary state and its flag (default true).
3. Extension acceleration nonstationary state and its flag (default true).
4. Any button transition and its flag (default true).
5. Stick change and its flag (default true).

The activity query is evaluated before its enable flag, as in retail; default-disabled pointer watching does not remove the original pointer-field read. Any enabled activity resets `mStep` to zero. Otherwise it increments only while below the original 3600-frame limit. `start()` resumes without resetting, `stop()` suspends, and `restart()` resets and resumes. No timeout action or forced controller disconnect belongs to this function.

The source uses mutable file-local booleans for the original mutable data bytes. It does not replace them with constants or new native input policy. All dependencies remain original WPad child methods: `WPadAcceleration::isStationary()`, `WPadButton::isChangeAnyState()`, and `WPadStick::isChanged()`.
