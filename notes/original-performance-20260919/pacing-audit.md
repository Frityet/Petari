# Original frontend timing and pacing audit

## Optional measurement

`SMGPC_DEBUG_FRAME_TIMING=1` enables a debug-only, single normal-exit summary
in `src/app/OriginalGameApplication.cpp`. It uses `steady_clock` and performs
three clock reads per completed frame, plus one initial read. The disabled
path does not read the clock or write output.

The interval starts immediately before the first frontend loop, after original
initialization. Consecutive completed-frame boundaries include event polling,
unsuccessful `aurora_begin_frame` attempts, the original `GameSystem::frameLoop`,
VI and device waits, Aurora end-frame work and screenshot requests. Startup,
teardown, and an incomplete final poll are excluded. A 300-entry rolling sum
reports the final 300 completed frames, or all completed frames for a shorter
run. The counts match only successful completed frames; exceptions emit no
normal-exit timing summary.

The three reported phase means are wall time: `begin_poll_ms` covers the prior
boundary through successful Aurora begin, `process_ms` covers the original
process frame including its retrace/device waits and enabled diagnostics,
and `end_ms` covers Aurora end plus the screenshot-request boundary. They
partition the same measured wall interval. They are not CPU-use measurements.
There is no simulation multiplier, sleep, skipped original update, or Game
source change in this patch. Validation is the coordinated main build and
root-owned actual-process timing run; no standalone timer test is warranted.

## Read-only source findings

- `GameSystem::frameLoop` calls original `MainLoopFramework::waitForRetrace`
  once. That invokes `waitForTick` and then `JUTVideo::waitRetraceIfNeed`.
  The latter is empty in both native and decomp source, so it adds no second
  wait. The two direct `VIWaitForRetrace` calls in `JUTVideo::setRenderMode`
  happen during mode/black initialization, not every ordinary frame.
- `GameSystemFrameControl::setMovement60fps` selects one VI retrace for the
  current non-PAL mode. The native VI clock period is 16,683,350 ns, about
  59.94 Hz. PAL retains original `frameToTick(1)` pacing. The VI worker
  coalesces hardware interrupts delayed by guest CPU ownership; it does not
  fabricate a backlog of callbacks after a stall.
- `AuroraWindow::poll_events` and `aurora::begin_frame` add no unconditional
  frame sleep. `gfx::pace_frame_start()` is currently not called. The begin
  path can wait for actual frame/staging slots. There are two frame slots and
  five staging slots; presenting is queued to the render worker.
- Host Vsync defaults on (`SMGPC_ENABLE_VSYNC`). Surface selection prefers
  FifoRelaxed if supported, otherwise Fifo. Presentation can therefore cause
  real backpressure, but it is not an unconditional second serial frontend
  wait. No evidence from the baseline supports disabling it or bypassing the
  original VI timing.
- One-millisecond sleeps in `wait_for_submitted_work` poll actual GPU work
  completion for original GX synchronization; 100-microsecond begin waits
  similarly poll actual slot availability. Their overhead can be profiled
  separately if they remain dominant after compiler optimization. Removing
  the required original synchronization is not justified by their presence.

The baseline `debug-sample.txt` has 3,847/5,826 main-thread samples in
`GXSetCopyClamp`, almost all waiting in FIFO drain; the render worker is
usually waiting for queued work while the FIFO worker spends substantial
time validating/decoding vertices. This is consistent with the unoptimized
CPU decoder bottleneck described in the parent README. It does not prove
that every later stage will reach nominal pacing after optimization.

## Diagnostics overhead

Without `SMGPC_DEBUG_ACTOR_TRACE_PATH`/layout dump configuration,
`OriginalProcessTrace::capture` returns immediately. Without
`SMGPC_DEBUG_WPAD_INPUT_FILE`, input-file replay returns without file I/O.
When that feature is enabled, it opens and reads its small input document
every frame; actor tracing serializes/flushed JSON only at its configured
interval. Script-state logs are transition-based. The neutral 600-frame
baseline log contains only 51 lines including startup/teardown, so ordinary
per-frame log spam is not needed to reproduce the slowdown. Performance
comparison runs should keep these optional diagnostics off, aside from the
single-summary timer, and keep screenshot timing outside the measured run.

No pacing behavior was changed during this audit.

## First optimized measurement

The root-owned clean run `../gateway-compat-20260919/perf-neutral-o2-600.json`
verified all 600 frames, exit 0 and process retirement without a debugger.
Its new timing line reports 12,315.336 ms for the measured frame loop,
20.526 ms/frame, and 53.807 FPS for the final 300 frames (5,575.441 ms).
Phase means are 0.505 ms begin/poll, 19.892 ms original process, and 0.129 ms
end. Rounded phases sum to the reported mean. The complete process took
13.09289 seconds, including startup/teardown, which is a different interval.
This validates the instrumentation and shows remaining time is mainly within
the original process. It does not establish sustained 59.94 FPS or identify
the optimized process's dominant internal operation without a new profile.
