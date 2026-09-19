# Out-of-line GX display-list execution

Published Aurora checkpoint: `1bdd5ba3fe9b4433ded84b61ae167b44ee230b03` on `origin/codex/macos-compat`; `git ls-remote` independently matched that SHA after push. Only the eleven display-list implementation/test paths were committed. The peer's unrelated BRLAN edits remained unstaged.

## Problem and contract

The original draw path calls `GXCallDisplayList` for model display lists. Aurora previously copied the complete list into its CPU FIFO; a captured original frame submitted a 77,920-byte list this way. That changed physical FIFO pressure, producer suspension and breakpoint pointer positions. Raw retail `CALL_DL` commands were also unconditionally skipped.

The SDK reference in `decomp/src/RVL_SDK/gx/GXDisplayList.c:69` emits a one-byte opcode, four-byte address and four-byte length after flushing dirty GX state and any flush primitive. List bytes occupy a separate memory span. Dolphin's `OpcodeDecoding.h:202` masks the low five address and length bits. `OpcodeDecoding.cpp:141` executes a list before resuming its outer command stream and warns/skips nested calls. Its normal decoder resolves list memory at execution; its deterministic preprocessing path copies the complete referenced span into its auxiliary FIFO.

Aurora's native address model cannot put arbitrary host pointers into the retail four-byte address. The new explicit `GX_AURORA_CALL_DISPLAY_LIST` extension is **15 bytes**: `GX_AURORA` (one), subtype (two), host address (eight), byte length (four), all multibyte fields big-endian. The raw retail command remains **nine bytes** and resolves through Aurora's existing physical MEM1 mapping. Cached/uncached address aliases and hardware alignment are applied to the raw command. The complete raw span must fit mapped MEM1. Raw MEM2 mapping is not implemented by Aurora's current `OSAddress.cpp` and is not invented here; native host spans are independent of that physical mapping.

## Implementation and ownership

- `GXCallDisplayList` emits the finalized native reference command in one epoch-cancelable FIFO write and publishes it. It preserves the preceding dirty-state/flush-primitive operations. If a high-water wait is interrupted by abort, no later header field is emitted after the canceled prefix.
- The source remains borrowed until GP fetch. Null nonempty spans and pointer arithmetic overflow are rejected; callers still own the actual allocation and must keep it readable until consumption. There is no existing generic allocator lease that can retain arbitrary native J3D allocations: the native GX array registry likewise documents a borrowed lifetime. This change does not claim runtime validation of every mapped host byte.
- At the call's GP encounter, the worker copies the source into host-owned command-stream storage. This preserves mutations before GP fetch while retaining stable bytes through token callbacks and deferred continuation. It is a whole-list fetch snapshot, not a simulation of individual hardware cache-line fetch timing.
- FIFO fetched/decoded positions advance by the call command only. Completed position remains before the call until its referenced list and callbacks finish. The list's continuation resumes before outer FIFO bytes and before a breakpoint following the call.
- Token and draw-done callbacks keep their existing ordering and guest interrupt boundary. Nested native or retail calls **warn and skip**, without resolving/dereferencing the nested span. A nested call does not throw merely because its unused address is invalid.
- Abort invalidates the list epoch, suppresses stale callbacks and discards the remaining list before recovery commands. Reprogramming the GP stream clears its continuation. Host allocation routing covers snapshot creation and stream retirement.
- Decoded primitive data is copied into renderer-owned vertex/index storage by the existing decoder. It does not retain a pointer into the temporary display-list vector.

The public header documents `GXDrawDone` or `AuroraDrainGXCommands` as a safe retirement boundary. This is a caller lifetime requirement, not an assertion that all game resource lifetimes were independently audited. The existing draw-sync heap retirement path drains outstanding GX commands. The unrelated raw-control-call ownership caveat from the prior FIFO work remains outside this change.

## Validation

The independent CMake `gx_fifo_tests` executable passes **333 tests from 12 suites**, including eleven new display-list cases. `final-build.log` and `final-tests.log` contain the final build and full run (1.611 seconds). The command was:

```sh
cmake --build build/aurora-fifo-tests-20260919 --target gx_fifo_tests -j4
build/aurora-fifo-tests-20260919/tests/gx_fifo_tests
```

New coverage proves:

1. A native call occupies 15 FIFO bytes and observes a source mutation before GP fetch.
2. A 77,925-byte list executes through a 128-byte physical ring; submitting its 15-byte call from an interrupt does not trigger payload-sized producer suspension.
3. Raw calls occupy nine bytes, honor alignment and aliases, and reject an extent crossing MEM1's end.
4. Empty native spans complete and invalid null/overflow spans fail explicitly.
5. Nested calls warn/skip, including an invalid nested physical address.
6. Token and draw-done callbacks run at the correct point within a list before its remaining and outer commands.
7. Drain waits through an in-flight callback, and a mutation after fetch cannot change the retained snapshot.
8. Abort revokes a pending callback, discards the list remainder and permits recovery commands.
9. Every split from byte 1 through byte 14 of the native call header remains incomplete until GP fetch resumes.
10. Truncated commands in a finite list still fail explicitly.
11. Abort during a suspended call submission cancels every remaining header byte; a recovery command then decodes normally.

Existing GD replay tests now exercise the real call-and-drain boundary instead of directly feeding captured FIFO payload to the decoder. Their data assertions are unchanged.

Intermediate logs are retained: the initial compile lacked the OS address declaration and was corrected by including its public header; the first new-test run exposed an incorrect fixture assumption about the 41-byte initial dirty-state flush. The fixture now flushes that state before measuring call sizes. `regression-tests2.log` passed the initial nine cases; `complete-tests.log` passed 332 tests including the additional empty/invalid-span case. These were test/setup corrections, not skipped assertions.

Final emission review then identified a real abort edge case: separate writes for the extension opcode, pointer and length could cancel the suspended pointer suffix but still emit the later four-byte length. `abort-submission-before.log` reproduces this exactly (written advanced from five to nine bytes after abort). The finalized 15-byte stack record is now submitted with one existing epoch-cancelable `write_data` call. The new regression and final 333-test run pass. A short intermediate compile error from incorrectly qualifying the existing global `bswap` helper is preserved in `abort-submission-namespace-build.log`.

This checkpoint validates the compatibility boundary. Original-game completion, visual correctness and timing remain separate runtime evidence owned by the parent task; no speedup or full Gateway progression is claimed from these tests.
