# J3D ownership across device waits

The original-process replay aborted at frame 62 with `blocking host wait while
scheduler is disabled`. The captured stack is
`../original-fifo-drain-stall-20260919/scheduler-disabled-capture2.log`:
`SceneScheduler::execute_draw_buffer` owns `SceneJ3dScope`, whose compatibility
`J3dCommandScope` disabled the OS scheduler across the entire draw. General FIFO
high-water flow control correctly attempted to yield the guest CPU while the
device consumed commands. That wait exposed the scope's broad scheduler lock.

The original SDK's FIFO overflow handler suspends its producer and resumes it
after the low-water interrupt (`decomp/src/RVL_SDK/gx/GXFifo.c:44-58`). The
original scheduler refuses to select another thread while `Reschedule > 0`
(`decomp/src/RVL_SDK/os/OSThread.c`, `SelectThread`). Aurora's corresponding
device-wait check remains unchanged.

## Change

`src/compat/J3dCommandScope.{hpp,cpp}` now owns cooperative guest execution plus
the actual original recursive mutexes: current-heap mutex
`MR::MutexHolder<1>::sMutex`, then J3D mutex `MR::MutexHolder<0>::sMutex`. The
scope saves GD state after acquiring both locks and restores GD and the caller's
interrupt bit before releasing them. It does not disable the scheduler.

The heap-first order is necessary even for a caller that does not yet have a
`JkrAllocationScope`: several `SceneScheduler` entries construct `SceneJ3dScope`
before `invoke_game_callback` enters the allocation domain, whereas resource
construction already owns the heap mutex before entering J3D. Acquiring J3D
first in the scene would invert those two locks. Existing allocation-domain
callers simply recurse through the same original heap mutex.

The guest execution scope prevents incidental interrupt restoration from
releasing the CPU between ordinary J3D statements. An explicit device wait can
still yield the CPU while retaining the J3D/heap mutexes. Another resource owner
sleeps on those mutexes; device interrupts can run independently. The mutex
guards release their own recursive acquisition and, during exception unwind,
any additional unbalanced original acquisitions made inside the scope. They
retain acquisitions that existed before entry.

`SceneJ3dScope.hpp` continues to snapshot and restore the full existing J3D
context after command ownership has been acquired. Its duplicate mutex recovery
was removed because the command owner now provides the same behavior for scene
and resource callers. The original J3D routines' smaller scheduler critical
sections and one-time static interrupt snapshots remain unchanged. No Game or
Aurora source changed for this fix.

## Validation

`xmake build -j8 smg-pc-j3d-command-scheduling-tests` passed; `build.log` records
the build. `xmake run smg-pc-j3d-command-scheduling-tests` exited 0; `tests.log`
records the result. The run command also rebuilt a concurrently completed
Aurora FIFO dependency; this log therefore describes the integrated local
state, not a separately pinned binary.

The focused regression exercises:

* The actual original `J3DModelData::indexToPtr` first with enabled interrupts,
  then disabled, then enabled, checking GD restoration and each caller's bit
  despite the routine's one-time saved flag.
* Nested `SceneJ3dScope` entries around actual original recursive OSMutex
  ownership, including a host exception crossing a manually acquired original
  lock. GD, J3D flags, joint matrix, texture-coordinate scale, and pre-existing
  lock counts are restored.
* A real `GuestThreadWaitScope` while another guest thread attempts a retained
  `JkrAllocationScope` plus J3D ownership. An actual
  `GuestInterruptExecutionScope` makes progress during the wait, while the
  competing resource cannot replace the active scene's GD/J3D/heap context.
* The inverse-entry-order scenario: a resource already owns heap1 when a scene
  starts its scope. A guest observer verifies that the scene sleeps on heap1
  without owning J3D0, allowing the resource to finish and release both.

This proves the focused scheduling and context boundary. The parent reports
that the main executable build passed with this fix. Its first replay was
intentionally terminated after a separate FIFO write-limit race was identified;
that is neither a J3D failure nor a completed runtime run. Final original
Gateway integration remains pending the FIFO correction and clean replay.

## Subsequent integrated runtime

After the final Aurora interrupt-abort correction, the fresh original-process
`fifo-j3d-final-held-a-2600` replay completed all 2,600 frames and exited 0 with
the PID gone and no debugger attached. See
`../original-fifo-interrupt-abort-20260919/README.md` for binary provenance,
input spans, screenshot review and explicit gameplay/rendering limits.
