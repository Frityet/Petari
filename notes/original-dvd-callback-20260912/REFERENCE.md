# Original DVD callback context

Read against the current reference SDK, not inferred from native worker timing.

- `RVL_SDK/os/OSInterrupt.c::__OSDispatchInterrupt` disables the scheduler around the interrupt handler, restores it, then reschedules.
- `RVL_SDK/ipc/ipcclt.c::IpcReplyHandler` clears a temporary `OSContext`, makes it current around `rep->cb`, clears it again and restores the interrupted context. `dvd_broadway.c::doTransactionCallback` forwards the low-level DVD callback on this route.
- `RVL_SDK/dvd/dvd.c::cbForStateBusy` invokes command completion and cancellation-request callbacks directly in that interrupt route. `dvdfs.c::cbForReadAsync` simply recovers `DVDFileInfo` and invokes its callback. Actual asynchronous native completions therefore need the guest CPU, temporary interrupt context, disabled interrupt mask and disabled scheduler.
- `dvd.c::DVDCancelAsync` also has immediate caller-side branches: terminal states and queued state 2 complete synchronously under `OSDisableInterrupts` alone. The queued branch removes the command, publishes state 10, invokes its original completion with -3, then invokes the cancellation-request callback with 0. These inline callbacks preserve the caller's current context and scheduler state.
- `dvd.c::DVDCancelAllAsync` pauses the queue, cancels queued commands, and forwards the cancellation request to the executing command. Without an executing command it calls the request callback with `(0, NULL)`. The request result is success 0, not the canceled command's -3 result.
- `dvd.c::DVDCancel` waits on the real `__DVDThreadQueue`; its completion callback wakes that queue. `dvdfs.c::DVDClose` calls `DVDCancel`, with no per-open host allocation to release.

These distinctions justify separate interrupt and caller-inline delivery policies. A scheduler-disabled interrupt callback cannot perform a blocking guest queue wait. A native wait must release its private mutex before restoring guest CPU ownership, then recheck the condition.
