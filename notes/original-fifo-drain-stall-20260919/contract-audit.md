# FIFO flow-control contract audit

Read-only review on 2026-09-19 while the separate FIFO implementation agent owns
the production changes. This records requirements and risks, not a claim that the
new implementation or Gateway progression has passed validation.

## Captured failure

`live-capture-state.log` records `written = published = 2807552207`,
`fetched = 2806388075`, and `decoded = processed = 2806388074` with a
524288-byte logical ring. The 1164132 unfetched bytes span more than two rings.
The enabled, hit breakpoint is at the fetched cursor, one byte into a draw.
The bytes at the decoded cursor begin with a valid draw header. A modulo ring
address cannot uniquely identify the intended pending occurrence once the
producer has lapped an unread consumer. This is evidence for missing producer
flow control, independently of command-prefix buffering.

## Reference behavior

* `decomp/src/RVL_SDK/gx/GXFifo.c:44-58,104-117`: the default high watermark is
  `size - 16 KiB`; the low watermark is `round_down_32(size / 2)`. A high-water
  interrupt disables high-water delivery, enables low-water delivery, and
  suspends the GX producer thread. Low-water delivery resumes that thread and
  restores high-water delivery. The local retail `GXFifo.s` agrees: producer
  suspend is at `0x804BA860`, resume at `0x804BA7C0`; defaults are formed at
  `0x804BA8E8` and `0x804BA8F4`.
* `dolphin/Source/Core/VideoCommon/CommandProcessor.cpp:479-483,517-521` uses
  strict `distance > high` and `distance < low` predicates. The FIFO register
  low words mask addresses/watermarks to 32-byte alignment. Tiny host test
  buffers and a zero low watermark therefore need an explicitly documented
  host rule; they are not faithful SDK-sized FIFO configurations.
* `decomp/src/RVL_SDK/gx/GXFifo.c:177-292`: flow-control interrupts apply to
  linked CPU and GP FIFO storage. GP binding programs the watermark registers;
  CPU binding does not. Saved objects can carry limits independently of the
  active GP register copy.
* `decomp/src/RVL_SDK/gx/GXMisc.c:37-52` flushes dirty state, writes eight zero
  words, then executes `PPCSync`. Retail `GXMisc.s` contains the eight writes at
  `0x804BBFC8..0x804BBFE4`, followed by the sync call at `0x804BBFE8`. This is
  exactly 32 NOOP bytes, not rounding the current length to a 32-byte boundary.
  It ensures earlier partial gather-pipe contents reach the FIFO. At audit time
  Aurora `GXFlush` only flushed dirty state.
* `dolphin/Source/Core/VideoCommon/Fifo.cpp:215-234,327-347` copies a 32-byte
  gather-pipe burst into separate retained decode storage. It advances the CP
  read pointer and decrements the ring distance even when the command decoder
  retains an incomplete prefix. Its storage compaction keeps undecoded bytes.
  Thus Aurora ring capacity must use **written minus fetched**, while decoder
  buffer retention must continue to use **decoded**. GPU completion is a third
  boundary and cannot substitute for either cursor.

The assembly paths inspected are under
`decomp/build/original-kcollision-traversal-20260903/retail/asm/RVL_SDK/gx/`.
No new decompilation or Game changes were needed for this audit.

## Implementation traps communicated to the owner

1. Publish complete bytes already produced before a high-water wait, even if
   they end inside a fixed-size GX command. Waiting on decoded would deadlock
   any command larger than the available ring window. Preserve the existing
   streaming preflight so those bytes have no command effects until complete.
2. Release guest CPU ownership during waits, with no FIFO/control/execution/
   decoder-buffer mutex held. VI or GX callbacks must be able to disable or
   rearm a breakpoint while the producer is suspended. A callback-free enabled
   breakpoint is still a real GP stop; flow control must not bypass it.
3. `Drain` and `Inline` processing modes have no FIFO worker. A throttled
   producer must perform synchronous fetch/decode progress there rather than
   only notify a nonexistent worker. Breakpoint handling still must yield to
   the guest control owner.
4. Hardware flow control is not scoped to host render frames. Disabling it
   outside `sFrameActive` requires a demonstrated ownership reason; otherwise
   startup writes can already exceed ring capacity at the next frame boundary.
5. `GX_AUTO` is a mutable Aurora extension: `GXBegin` writes a zero byte-length
   field which `GXEnd` patches. Publishing that field during a high-water wait
   can decode a zero-length draw and violates `patch_u32`'s unpublished-byte
   invariant. A publication fence alone deadlocks a record larger than the
   ring. Stage the patchable record outside the ring, finalize its length, then
   submit the immutable bytes through the ordinary throttled path.
6. Keep such staging separate from display-list capture: display lists have
   their own explicit capacity, padding, and unsupported-AUTO contract. A
   record's source allocation must remain alive until a possibly blocking
   submission finishes; no decoder mutex may span submission. Capture/validate
   FIFO generation and abort epoch so a rebind or abort cannot silently submit
   an old record into a new owner. Generic indexed draws already know both
   counts in their header and need no mutable-record workaround.
7. Preserve the native saved-pointer mapping as monotonically counted bytes
   plus the initial ring offset, modulo ring size. Enforce the unread capacity
   bound rather than choosing an arbitrary old/new lap for a breakpoint.

Useful focused cases are a fixed draw larger than a small ring, a high-water
stop inside its header and payload, GP breakpoint rearm by another guest thread,
detached CPU/GP streams, both synchronous processing modes, a staged AUTO draw
larger than a ring, and exactly one finalized draw after `GXEnd`. Check that
retained undecoded prefixes and the existing strict finite-display-list failure
behavior survive all these paths.

## Patchable-record closure

At the FIFO implementation owner's request, this audit also supplied the
independent mutable-record fix in `aurora/lib/gx/fifo_recording.{hpp,cpp}` and
`aurora/lib/dolphin/gx/GXVert.cpp`. `GX_AUTO` now records its header and vertices
outside ring storage, patches the final byte count, then submits immutable bytes
through the usual capacity-controlled FIFO writer. Ordinary fixed-count and
indexed draws retain their streaming path. No Game source changed.

The recording retains its CPU FIFO identity/revision and command epoch. Binding
another FIFO or entering display-list recording while a patchable record is
pending is rejected. Abort discards an unsubmitted record; the integrated
throttled writer must also cancel the remainder when abort occurs while
submitting. A submitting flag retains the FIFO owner through final publication
and lets shutdown wait without holding guest CPU ownership.

Five focused tests were added in `aurora/tests/gx_fifo_test.cpp`: a 1800-byte
AUTO payload submitted through a 256-byte ring at all seven possible header
split positions; abort before submission followed by a valid new draw; rejection
of CPU/GP rebinding while recording; abort while submission is blocked at a
breakpoint inside the finalized header; and FIFO shutdown while blocked in that
same submission.

`full-with-shutdown.log` records the integrated 317/317 test pass, including all
five added cases. This is isolated FIFO/encoder/decoder validation. Native
Gateway progression remains a separate runtime claim. A subsequent correction
to the non-worker processing-mode wait loop is owned and revalidated by the
FIFO implementation agent before publication.

## Scheduler-disabled replay follow-up

The post-publication original-game replay aborted within 4.7 seconds; it did
not validate completion. The separate `scheduler-disabled-capture2.log` stops
at original frame 62 inside a 77920-byte GXCallDisplayList append. Its
GuestThreadWaitScope correctly rejects a blocking wait with `Reschedule > 0`.
Root identified the native J3dCommandScope created by SceneJ3dScope around
the draw phase as the broad scheduler prohibition. The original shared J3D
mutex is the candidate ownership mechanism; weakening the Aurora scheduler
guard would permit a guest switch prohibited by the current guest context.

Additional source evidence:

* `decomp/src/RVL_SDK/os/OSThread.c:303-312`: SelectThread returns when
  Reschedule is positive.
* `decomp/src/RVL_SDK/os/OSThread.c:574-608`: OSSuspendThread increments the
  suspend count, changes running state to ready, and invokes __OSReschedule.
* `aurora/lib/dolphin/os/OSExecution.cpp:503-517`: the native host-wait scope
  rejects scheduler-disabled blocking, otherwise releases guest CPU ownership
  and checks scheduling when ownership returns.
* `aurora/lib/dolphin/gx/GXDispList.cpp:49-62` currently inlines a display-list
  payload into FIFO storage, unlike the retail 9-byte address/length call.
  `aurora/lib/gx/command_processor.cpp:437-442` skips raw nested CALL_DL. This is
  a separate unresolved list ownership/execution gap, with extra ring pressure,
  and has not been fixed by changing scheduler or FIFO safety semantics.

No Aurora or Game production changes accompanied this follow-up audit. The
native J3D boundary repair and fresh original completion evidence belong to
the subsequent coordinated checkpoint.
