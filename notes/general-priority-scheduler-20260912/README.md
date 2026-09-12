# General SDK priority scheduling — 2026-09-12

Original JKRAramStream and JKRDecomp resume higher-priority workers whose entry points initialize their queues. Aurora previously notified the host worker but let the caller continue while retaining the guest CPU gate. The next original queue operation could therefore observe an uninitialized queue.

Aurora now orders runnable SDK threads by effective priority and arrival using an actual intrusive ready queue. Resume, queue wake, priority changes and suspension select higher-priority ready work before the caller continues. Explicit yield requeues the caller after its priority peers while allowing host callbacks to enter the CPU gate; lower-priority SDK workers cannot steal its turn. Sleeping, cancellation, suspended waiters and completion maintain queue membership. Returning from an explicit host wait also honors higher-priority ready guest work.

The original reference is `decomp/src/RVL_SDK/os/OSThread.c`, particularly SelectThread, OSResumeThread, OSSuspendThread, OSWakeupThread and OSSetThreadPriority. Scheduling is valid while the caller's saved interrupt bit is disabled; that per-context bit survives switching. OSEnableScheduler still only updates its nesting count, as in the reference.

Validation: 39 OS tests pass for 20 consecutive iterations; 290 FIFO/graphics-command tests pass. New cases cover 64 immediate worker queue startups and wakeups, native pointer delivery, masked interrupts, disabled scheduling, nested suspension, equal-priority FIFO selection, lower-priority deferral, priority changes, ready-thread cancellation and return from a host wait. Receipts record the tested binaries and command results. The native original JKRThread and DrawSyncManager executables are separately checked in the startup notes.

This remains cooperative native execution at explicit SDK/host boundaries. It does not asynchronously interrupt arbitrary C++ loops, emulate PowerPC register contexts or establish complete GameSystem startup or Gateway progression.
