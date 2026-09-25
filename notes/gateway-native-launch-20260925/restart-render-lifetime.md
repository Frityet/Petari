# Restart crash: frame recorder lifetime

2026-09-25. Evidence: `restart-crash.json` records render-worker `submit_frame_prefix` → `CommandEncoder::Finish` → Dawn `ObjectBase::GetDevice`, with null receiver/address0x10. The final log warns that a pending buffer map was aborted by Unmap. Current source uses `FramePacket::encoder`; there is no global `g_state.cmdEncoder` at this call.

## Diagnosis and scope

The render-worker queue is FIFO, but `synchronize()` is only a barrier for already-enqueued work. It does not exclude a second producer. Main-thread `begin_recording`/`end_recording` and FIFO-worker decoding/`complete_draw`/abort retirement shared an unguarded recorder. Beginning published the packet before queuing its encoder creation. Ending moved the encoder, cleared the packet, and remapped its staging buffer in a queued job. A concurrent prefix could keep a raw packet reference across that job. A command epoch detects GX abort, not ordinary retirement/reuse in the same epoch, so it could pass the epoch check then Unmap a remapping staging slot and Finish the cleared encoder. This is a concrete source-level interleaving consistent with both crash observations; the crash report alone does not identify which main/FIFO scheduling order occurred.

`aurora::end_frame` drains a snapshot of the current FIFO write cursor before finalizing. That snapshot is not consumer exclusion: restart/SDK work can publish more tokens, and `process_to` calls `complete_draw` outside the decoder's existing execution mutex. Merely checking for null would lose command/completion semantics and leave staging ownership incorrect.

## Applied fix

Only these initially clean Aurora sources changed:

- `lib/gfx/recording.hpp/.cpp`: one actual recorder-lifetime mutex and `detail::lock_recording()`. Begin/end recording, finish, complete_draw and abandon_recording take it. Recursive locking supports the outer frame transaction and inner recorder helpers.
- `lib/gfx/frame.cpp`: reserve frame/staging slots before taking the gate, then initialize the packet and enqueue BeginFrame encoder creation **before** publishing the recorder. Hold the same gate while closing recording and enqueueing EndFrame retirement.
- `lib/gx/fifo.cpp`: each decoder iteration owns the gate before its execution/buffer locks, including abort acknowledgment/recording retirement. It releases the gate before all draw-done, draw-sync and breakpoint dispatches; complete_draw takes its own bounded borrow, then releases it before any guest callback.
- `lib/aurora.cpp`: after the existing FIFO drain, hold the gate continuously across final-pass finishing, presentation recording and end-frame queueing. This closes the gap between finish and recorder detachment.

Prefix synchronization now retains recorder/packet/staging ownership until the render worker finishes the prefix and continuation setup. EndFrame cannot be queued by a concurrent recorder close during that interval. No null-return, skipped submission, synthetic frame, or game-specific restart path was added.

## Deadlock and ownership boundaries

- Render-worker jobs **never acquire** the recorder gate. Prefix/abort paths may safely wait for their queued work while owning it.
- No FIFO drain is performed under the new outer frame-end gate. `gfx::finish`, texture/pipeline end-frame, and current RML rendering paths contain no FIFO drain or `lock_execution` wait. The decoder's internal order is recorder→execution→buffer; no code holds execution then asks for recorder.
- Guest callbacks run after releasing recorder ownership. Lock contention uses the existing FIFO style: try_lock, yield inside `GuestThreadWaitScope`, restore guest CPU ownership, then retry. CPU ownership is never reacquired while already holding the recorder gate.
- GXAbortFrame remains atomic/nonblocking for the original alarm interrupt. Decoder-side abandonment waits for recorder ownership afterward; existing epoch/submission arbitration remains unchanged.
- Existing map-future retirement and slot accounting remain unchanged. Preventing stale packet/unmap operations addresses the demonstrated ownership race instead of treating an Aborted map callback as success.

No builds or tests in this lane. Root owns the integrated build and a bounded existing rendering check. A helper thread calling direct `gfx::complete_draw` across real begin/end boundaries exercises the new ownership protocol; concurrently writing raw GX commands from multiple producers would violate the separate serialized-guest producer contract and is not an appropriate substitute.
