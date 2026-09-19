# FIFO capacity immediately after an interrupt abort

Published Aurora `bce9e36b21845f1cb886d9cdbcbcc65193fc91e8`
(`Release aborted FIFO capacity before interrupt recovery writes`) to
`origin/codex/macos-compat`; the full remote SHA was verified with ls-remote.
Aurora is clean. Root owns the parent gitlink and notes checkpoint.

The actual GXAbortFrame path does **not** call stop_worker or discard_recording.
The previously passing AlarmAbortDiscardsStoppedCommandsAndPreservesRingAddresses
already enters GXAbortFrame inside GuestInterruptExecutionScope. The initial
concern about those teardown helpers breaking every watchdog abort was not
reproduced on that path.

A related real omission was reproduced instead: the original MainLoopFramework
watchdog writes recovery BP and draw-done commands **inside the same interrupt**
after aborting. At a full, stopped ring, Aurora only marked old commands for
asynchronous abandonment. Ring capacity remained unavailable until the worker
could retire them, making the interrupt's recovery write enter a forbidden host
wait. `before-test.log` fails deterministically with exit -6 and the existing
`blocking host wait while scheduler is disabled` safeguard.

## Reference and repair

`decomp/src/RVL_SDK/gx/GXMisc.c:94-103` calls __GXCleanGPFifo during GXAbortFrame.
`decomp/src/RVL_SDK/gx/GXFifo.c:413-457` disables GP reads, sets the read pointer
to the write pointer, sets the count to zero, updates the linked CPU FIFO, clears
the breakpoint, then enables reads. Ring space is therefore available before
GXAbortFrame returns. GPU/host encoder retirement is a separate native concern.

Aurora now treats `max(fetched, abortFloor)` as the logical fetch cursor for
capacity, saved FIFO pointers/count, breakpoint placement, and GP programming.
Abort publishes that free capacity immediately without waiting for renderer
retirement. Completed-command retirement remains asynchronous; the regression
checks that the count is already zero while completed is still below the abort
floor, then writes the original recovery commands within the interrupt.

Worker/abort capacity advances use an atomic maximum, so a worker that computed
an older limit cannot overwrite the interrupt's larger abort limit. Binding,
watermark programming, initialization, and shutdown explicitly reset the limit
while holding the existing execution mutex, which excludes decoder updates.
The mutex-free abort path executes under original guest CPU ownership in the
alarm interrupt, excluding guest FIFO binding/configuration. Concurrent raw
host control calls without that ownership are outside the serialized GX
producer/control contract; this change does not add such a capability.
Root and a second agent reviewed the CAS/retirement ordering before publication;
the reviewer found no blocker for the original-process contract and retained
the raw-host-control ownership caveat above for a separate follow-up.

The writer also rechecks the command epoch before waiting and treats a fetched
cursor at or beyond its saved write cursor as free space, avoiding unsigned
subtraction underflow after an abort. No-op recording cleanup and a stop with
zero blocked producers now avoid constructing a wait scope. Actual outstanding
submissions, producers, and worker joins retain the existing wait safeguards.
Aurora OSExecution and all Game source remain unchanged.

## Validation

`verified-build.log` followed by `verified-full.log` passes **322/322 tests in
11 suites**. The build is checked before executing the test binary. Four added
regressions cover immediate ISR recovery writes after abort, empty recording
cleanup inside an interrupt, repeated already-retired shutdown inside an
interrupt, and rejection of an ISR producer write that really must wait on an
unaborted full FIFO. Existing abort/submission and ownership tests remain
enabled.

`final-build.log` and `cas-build.log` document an intermediate test macro syntax
failure (a template comma inside EXPECT_DEATH). Their accompanying 321-test
logs ran an older binary and are **not** evidence for the final source. The
corrected checked build and 322-test log above are authoritative.

The focused tests are separate from the original-process runtime result below.

## Completed original-process replay

The fresh `fifo-j3d-final-held-a-2600` run completed all 2,600 original
GameSystem frames in 223.385 seconds and exited 0, with the actual completion
marker and PID 39531 verified absent. No debugger attached and no timeout or
external termination occurred. Binary SHA256:
`f93c8531ba097a0b157ae8d58b132f585337bfc3efa4197eef167ad6aff55206`.
The final main build passed; `original-build.log.gz` preserves its output.

The recorded debug controller script is the same eight held-A spans beginning
at frames 1400, 1500, 1600, 1700, 1800, 1900, 2000 and 2100, each through
start+10 inclusive. This is original WPad/KPAD controller replay, not proof of
physical keyboard use. Pointer and stick scripts were explicitly empty, and
fresh save storage was used. This run passed both previously captured FIFO
stalls at frames 1543 and 1578.

The native frame-2300 PNG was opened and visually inspected. It shows Mario
on the starting flower field, a rabbit, the planet and HUD. The tutorial text
and A prompt overlap and clip at the left edge, so rendering/UI parity remains
incomplete. No rabbit catches, original completion-group state, or Rosalina
appearance were validated. The elapsed time includes startup and is not a
steady-state FPS measurement. This establishes bounded original startup and
controller replay completion, not completion of the requested demo.

Exact launch/result JSON, compressed raw log and compressed unmodified PNG
are in `../gateway-compat-20260919/fifo-j3d-final-held-a-2600*`.
