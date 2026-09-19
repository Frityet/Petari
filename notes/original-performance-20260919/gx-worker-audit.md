# Read-only GX worker audit

The unoptimized baseline `debug-sample.txt` captures 5,826 wall-clock samples per main worker thread. The main thread spends 3,845 samples in `fifo::drain` called by `GXSetCopyClamp`, plus 254 in the draw-done drain. The FIFO worker spends 4,326 samples in its processing branch and 1,500 asleep. Within processing, 2,236 samples are in the `require_draw_array_spans` call beneath `draw_prim`. The Aurora render worker waits for queue input in 4,920 samples (~84%). These overlapping per-thread counts are not additive CPU percentages.

`aurora/lib/gx/command_processor.cpp:578` scans each vertex's attributes, recomputes component widths and indexed-element extents, and accumulates required spans before checking each array. At O0, the trace attributes many samples to otherwise tiny std::array/std::max/read_bits helpers. The boundary checks must remain; the first measured change is compiler O2 with debug assertions and existing Game floating-point rules retained. No GX command, shader, queue, callback, validation, or simulation behavior was changed for this comparison.

If O2 does not remove this bottleneck, layout-invariant work could be precomputed for the active VCD/VAT while retaining every indexed extent check. That is a future candidate, not implemented or benchmarked.

## Optimized build verification

The serialized library builds and main/focused target links all passed. See `o2-library-builds.json`, `o2-batch-builds.json` and verbose per-target logs. A second task briefly started another Xmake while the first owned the slot; the second was immediately stopped, reported an empty log while waiting for Xmake's lock, and returned143. The owner reran all four library dependency checks before linking main/tests; every recheck passed.

`o2-effective-flags.json` records exact effective commands for MarioMove, Binder, GameMathCompat, the GX decoder and FIFO. Each uses LLVM23, `-O2 -g -DSMGPC_DEBUG_BUILD`, no NDEBUG, and no fast-math. Binder and GameMathCompat retain their existing `-ffp-contract=off`. No per-file floating-point option was removed. The linked main hash is `dcc7420463f7095aff46eb5f709cc2cbbf7814a3d70192911c1860b2a7d7279e`.

Pure matrix/quaternion and controller-file/frame-edge tests passed. The actual-process pointer and shadow probes were intentionally deferred until the root task completed its optimized600-frame runtime measurement, avoiding measurement interference.
