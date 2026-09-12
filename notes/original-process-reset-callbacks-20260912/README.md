# Native process reset/power SDK review — 2026-09-12

The new generic process boundary had two differences from the retail SDK: it returned physical held state from OSGetResetButtonState and retained callbacks after invocation. Both are corrected using the complete local OSStateTM source and retail assembly, without changing Game's reset owner or the host process-transition handler.

OSGetResetButtonState now consumes the pending press latch. A held input does not repeat; release does not discard an unconsumed press; repeated presses coalesce until queried. The native physical-input entry accepts actual press/release reports and does not derive them from menu or restart requests.

Reset and power callbacks are copied and cleared before delivery, preserving safe callback re-registration. Null is the native representation of the original empty default callback and returns as null when replaced. Setters and latch reads preserve the caller's interrupt bit, and callback delivery runs under the actual GuestInterruptExecutionScope with captured allocation routing. This serializes delivery against guest execution and restores context, scheduler state, routing and interrupt state after normal return or a terminal ProcessRequest exception.

OSGetResetCode reads the host-inherited AURORA_PROCESS_RESET_CODE as an entire unsigned decimal u32 and returns 0x80000000 OR that code. Cold launch and invalid/overflow input return zero. The four terminal SDK calls retain their typed ProcessRequest handoff; this lane never exits, re-execs, reboots or powers down the host. Root owns the top-level handler and real child re-exec proof.

The newly drafted OSGetResetSwitchState alias was removed: it was unused by Game and absent from this retail export set, and aliasing its name to the Wii consume-on-read query would create misleading legacy behavior. No obsolete OSResetSystem implementation was added.

## Proof

`compile-command.json` builds the actual SDK source with 9 new reset tests and the existing 48 alarm/execution/mutex/message regressions. All 57 tests pass, exit 0, in `test-result.json` / `test-run.log`.

Coverage includes press/release latch boundaries, previous handler/null behavior, one-shot and reentrant successor registration, actual interrupt context and borrowed guest thread identity, allocation routing in both modes, disabled-state preservation, native delivery waiting for the guest CPU, exception unwind, all four terminal request payloads, and inherited restart-code validation.

`reference-proof.json` records retail instruction addresses and exact source hashes. This is source/assembly behavioral review of already recovered SDK code; no new decompilation or synthetic test provider was introduced. The native service does not emulate IOS STM device-registration failures or electrical front-panel debouncing; its input is an explicit platform event. The existing VI dimming subsystem is outside this bounded task.

`source-manifest.json` contains the exact three changed/new Aurora paths. Root owns CMake/Xmake activation and publication. No shared build or host process-handler edit was performed by this lane.
