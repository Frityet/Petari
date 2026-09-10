# Original DrawSyncManager recovery

The complete original owner is recovered in `decomp/src/Game/System/DrawSyncManager.cpp` and copied byte-identically to native `src/Game/System/DrawSyncManager.cpp`. Its header is also identical. No host replacement manager, direct Game workaround, Xmake run, or native compile was added in this task.

## Recovered contracts

- Five typed callback token ranges replace the old opaque 0x28-byte field. Slots 0–2 allocate from token 1; slots 3–4 allocate from 0xA000. Inclusive endpoints and the original u16 arithmetic are retained.
- The original 0x8000-byte worker stack, 20-message queue, and capacity-plus-one FIFO are allocated through actual pointer types. The queue and array allocations therefore scale correctly to native pointer width without a second Game implementation.
- The worker receives blocking messages. Pointer messages at or above 0x80000000 append FIFO boundaries; 16-bit token messages retire a boundary; values in the intervening range terminate the worker (the destructor sends 0x10000). The source uses uintptr_t to preserve native pointer values.
- At exactly two queued boundaries the newest boundary becomes the GX breakpoint. A completion leaving one boundary disables the breakpoint; two or more select the next boundary after the head. The original volatile `_373` reads are retained even though the retail worker does not branch on their values.
- Callback token zero posts completion unless mode bit 1 is set. Other tokens invoke only the first registered matching range, then post completion under the same mode condition. Unmatched tokens do not post completion.
- The destructor unregisters draw-sync, disables the breakpoint, sends the exit message, and joins. Retail does not individually free stack, messages, or FIFO buffers; the owning heap lifecycle must retain and retire them appropriately.

## Proven existing reference correction

`pushBreakPoint` previously ran when `(_36C & 3) != 0`. Retail 0x80398E54–0x80398E5C branches past submission when either bit is set, so the recovered condition is `!(_36C & 3)`. Its corrected body matches 100%.

The SDK reference header `decomp/libs/RVL_SDK/include/revolution/gx/GXFifo.h` also lacked the real `GXEnableBreakPt(void*)` declaration. It was added to compile the actual call; no SDK implementation was substituted.

## Compact proof

`wii-build.json` records the full translation-unit compile, exit 0. The first attempted compile only identified that missing SDK declaration; its diagnostic is retained separately. `objdiff.json` and `match-summary.json` record one successful comparison:

- `threadFunc`, `drawSyncCallback`, `setCallback`, generated token-range assignment: 100%.
- Constructor: 97.169014%; callback dispatch: 95.65385%.
- Corrected `pushBreakPoint`: 100%.
- Existing reset/destructor/FIFO bodies: 100%, except the existing tiny `prepareReset` compiles with a different inline shape (89.166664%). No semantic tuning was made for that score.

All 1,740 instruction bytes in the retail assembly were independently verified against the actual main.dol, zero mismatches (`retail-dol-proof.json`). `source-manifest.json` records current source hashes.

## Explicit native GX frontier

At import, Aurora has only a declaration for `GXSetDrawSyncCallback`; `GXEnableBreakPt` and `GXDisableBreakPt` are TODOs. `GXGetFifoPtrs` returns null pointers. Its original Dolphin no-argument `GXGetCPUFifo`/`GXGetGPFifo` accessors were incompatible with the Wii output-copy signatures. The canonical SDK declarations now use the Wii signatures, and the unused pointer-return implementations were removed. Actual snapshots remain unresolved until live FIFO metadata and cursor ownership are implemented; copying an empty initialization object would falsely claim a valid snapshot. `GXEnableBreakPt` and `GXDisableBreakPt` now have declarations but intentionally no fabricated definitions. The parent is informed that this owner requires genuine ordered native command/FIFO completion support; this task does not turn those calls into no-ops. The restored owner is source-complete, not a claim that full process startup runs.

Separately, canonical Aurora `revolution/os.h` now includes the already-existing `dolphin/os/OSFastCast.h`, providing `OSInitFastCast` to original GameSystem, FunctionAsyncExecutor and NANDManagerThread callers without per-Game changes.
