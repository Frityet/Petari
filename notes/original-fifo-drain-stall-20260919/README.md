# Original Gateway FIFO ring backpressure

The original held-A replay stalled twice because the native CPU producer could
lap unread GP FIFO storage. This change restores general linked-FIFO capacity
and watermark behavior in Aurora; it does not bypass a stopped GP breakpoint or
change any Game source. A fresh original-game replay is still required to show
progress past the observed stalls.

Published Aurora commit `cead297129094e289e18e940d477b83359de7662`
(`Apply linked FIFO backpressure and stage patchable draw commands`) to
`origin/codex/macos-compat`; `git ls-remote` confirms the same full SHA.
The Aurora worktree is clean after publication. Root owns the parent gitlink
and notes checkpoint.

## Captured failures

The first stopped process (PID 29307) was attached and detached for observation,
then explicitly terminated and verified absent. `live-capture-state.log` records
frame 1543, a 524288-byte ring, written/published 2807552207, fetched/breakpoint
2806388075, and decoded/processed 2806388074. Thus 1164132 bytes were pending,
more than two complete rings. The breakpoint was one byte into a draw header.

A second fresh run, PID 31558, reproduced the stall at frame 1578. Its
`second-capture.log` records written/published 2904760385, fetched/breakpoint
2904120541, and decoded/processed 2904120522. The 639844 unfetched bytes exceeded
the same ring capacity. The GP was stopped 19 bytes into an XF command. The
original DrawSyncManager retained two saved addresses ten bytes apart, had an
empty message queue, and waited for command tokens. Its callback-free GP
breakpoint is legitimate; the missing producer bound made the saved physical
address resolve to an earlier unread ring lap instead of the intended boundary.
The process was explicitly killed through LLDB and verified absent.

These are debugger captures, not completed bounded runs. Debugger attachment
can reparent the process: `replay-result.log` correctly reports null exit/frame
counts and `verified_bounded_completion: false`, despite the shell's reported
wait status. Root retains launch environments and run logs under
`notes/gateway-compat-20260919/`.

## Compatibility change

* Linked CPU/GP storage is bounded throughout its lifetime, including before a
  host render frame. GP binding owns the programmed high/low watermarks; a CPU
  object carrying other limits cannot overwrite them.
* Producer capacity is based on written minus **fetched**, allowing a command
  larger than the ring to stream while the decoder retains its partial prefix.
  High-water suspension publishes bytes already written, and resumes strictly
  below the low watermark. Waiting releases original guest CPU ownership and
  holds no decoder, buffer, FIFO-object, or execution mutex.
* High-water comparison is strict (`count > high`). Native small FIFOs may use
  high equal to physical size; their hard capacity still prevents overwriting
  unread bytes. The accepted zero-low native configuration resumes when empty.
  These small-buffer rules are not claims about SDK-valid aligned Wii FIFOs.
* GP detach releases a blocked producer. Shutdown cancels blocked writes before
  retiring storage. Abort cancels the remainder of a partially submitted write.
  CPU rebinding during a blocked multi-byte write fails explicitly instead of
  splitting a command across owners.
* Aurora's variable-length `GX_AUTO` extension now stages its patchable command
  outside the ring until GXEnd finalizes its length. It then submits immutable
  bytes through ordinary flow control. The FIFO identity/revision, command
  epoch, submission lifetime, display-list exclusivity, and shutdown ordering
  remain checked. Fixed-count and indexed commands retain ordinary streaming.

Reference findings and the staging contribution are recorded in
`contract-audit.md`. The SDK/Dolphin reference explicitly distinguishes fetched
ring bytes from decoded commands, and suspends/resumes the producer around the
high/low interrupts.

## Validation

`regression-before.log` demonstrates the new producer-capacity regression fails
against the preceding implementation: a paused consumer allowed all 256 bytes
to be written into a 128-byte ring and the producer returned immediately.
`regression-after.log` records the focused pass after flow control. The first
shell wrapper printed the following `cat` status; the GoogleTest failure in the
log is the authoritative result for the before case.

The independent CMake target is `build/aurora-fifo-tests-20260919/tests/gx_fifo_tests`.
`full-final.log` passes all **318 tests in 11 suites**. Added coverage
checks pre-frame GP limits, strict low-water hysteresis, genuine guest ownership
yield to a breakpoint callback, fixed and indexed draws larger than the ring, GP detach,
shutdown, all seven internal splits of a finalized AUTO header, no ring advance
before AUTO finalization, exactly one decoded draw, owner-change rejection,
abort before/during submission, and shutdown during submission. Existing tests
remain enabled and unchanged except the added cases. The earlier focused run's
single failure was a new test observing an already-hit empty-ring breakpoint
before its asynchronous producer had started; waiting for both breakpoint and
produced bytes corrected the fixture. The full suites pass without retries.

The alternate processing branches were also compiled independently: temporary
copies of fifo.cpp under `build/aurora-fifo-tests-20260919/mode-checks/` differ
only in the `kProcessingMode` selector. Reusing Ninja's exact compiler/link
arguments and all other existing objects, **8 focused tests pass in Drain and
the same 8 pass in Inline**. `mode-Drain.log`, `mode-Inline.log`, and
`mode-checks.json` record the cases and results. These exercise strict low-water
waiting, guest callback ownership, large fixed/indexed/AUTO commands, abort,
and shutdown. The repository source retains Thread mode throughout; shared
native build configuration was not modified.

## Explicit boundaries

This does not implement a complete 32-byte gather pipe. In particular, retail
GXFlush writes eight NOOP words; Aurora's current GXFlush only flushes dirty
state. That independent surface remains deferred. The published FIFO decoder
processes byte prefixes and retains their command ownership rather than
emulating each hardware gather burst. This checkpoint validates the compiled
threaded processing mode and the focused alternate-mode cases above.
No full Gateway, rabbit progression, or Rosalina spawn
claim follows from the focused suite.
