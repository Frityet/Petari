# Aurora SDK interrupt and scheduling ownership — 2026-09-03

Original J3D packet, shape, and model-data code now reaches SDK interrupt and
scheduler critical sections. Aurora declared these APIs but did not provide
them. `lib/dolphin/os/OSExecution.cpp` supplies `OSDisableInterrupts`,
`OSEnableInterrupts`, `OSRestoreInterrupts`, `OSDisableScheduler`,
`OSEnableScheduler`, and `OSYieldThread` through a shared cooperative
emulated-CPU gate. CMake and xmake both include the provider in Aurora OS.

## Original contracts and host mapping

The source contract comes from `src/RVL_SDK/os/OSInterrupt.c` and
`src/RVL_SDK/os/OSThread.c`, checked against the supplied RMGK01 DOL.

| API | Retail behavior | Native boundary |
| --- | --- | --- |
| Disable interrupts | Clear MSR EE; return its old bit | Set the caller's bit false and retain CPU ownership |
| Enable interrupts | Set MSR EE; return its old bit | Set the caller's bit true; release ownership if scheduling also allows it |
| Restore interrupts | Set EE from whether argument is nonzero; return old EE | Restore the Boolean state directly, without a nesting counter |
| Disable scheduler | Return old signed Reschedule, then increment | Same global count, updated under interrupt exclusion |
| Enable scheduler | Return old signed Reschedule, then decrement | Same decrement; negative values are preserved, not clamped |
| Yield thread | Save EE, SelectThread(TRUE), restore EE; selection refuses while count > 0 | Release the CPU gate, yield the host thread, reacquire, and restore EE when count <= 0 |

Repeated interrupt disables therefore return true, false, false. Restoring true
enables interrupts immediately; restoring false does not release the gate.
This also supports actual J3D methods whose retail first-call static guard
captures an interrupt flag once and later restores it without another fresh
per-call interrupt disable. Their scheduler calls still protect each invocation.

Scheduler updates use a `u32` bit pattern and `std::bit_cast<s32>` for the
returned signed count and `> 0` test. This preserves PowerPC increment/decrement
wrap without signed C++ overflow. Original `OSEnableScheduler` does not call
SelectThread, so the native function does not insert an explicit host yield.

A single `std::mutex` serializes participating API callers. A thread-local flag
records whether the calling host thread already owns that mutex, so nested calls
do not lock it recursively. The caller's interrupt bit is also thread-local,
like a saved execution-context EE bit. The scheduler count is global and is
only accessed while owning the mutex. A caller retains ownership until both
interrupts are enabled and the signed scheduler count is <= 0. Only the owning
host thread unlocks the mutex.

An explicit yield is different from ordinary interrupt restoration: original
`SelectThread(TRUE)` may switch even if EE was already disabled. The native
provider releases its gate for this explicit handoff and reacquires it before
restoring the caller's saved EE bit. It does not release the gate while the
scheduler count is positive. Waiting callers observe protected memory through
the mutex's acquire/release synchronization.

This is a cooperative boundary for emulated SDK callers. It does not suspend
unrelated native workers, mask host signals, implement interrupt dispatch,
create SDK thread contexts, or reproduce Wii priority queues. Every thread
participating in a critical section must use these APIs and leave balanced
scheduler/interrupt state before exiting. Existing rendering and I/O workers
continue to use their own native synchronization. No Game code changes or
J3D-specific branches were introduced.

## Evidence

Retail functions inspected:

- Interrupt disable/enable/restore: `0x804A9778/0x14`, `0x804A978C/0x14`,
  `0x804A97A0/0x24`.
- Scheduler disable/enable: `0x804AC580/0x3C`, `0x804AC5BC/0x3C`.
- Yield: `0x804ACAE0/0x3C`; SelectThread begins at `0x804AC8A0`.

At `0x804AC594` and `0x804AC5D0`, both counter functions load the same SDA slot
`-7992(r13)`. They return that loaded value after adding +1/-1 and storing it
back. SelectThread reads that same slot, compares it signed against zero, and
continues only when <= 0. Yield saves the disable result, passes true to
SelectThread, then restores the saved value. The interrupt routines extract
the old EE bit and never maintain a disable depth.

`verify.py` checks the DOL identity and 26 relevant instruction words, records
retail function hashes, and captures source/test/binary hashes. It does not
claim instruction matching between PowerPC register operations and the host
mutex implementation. Raw disassembly remains under the ignored directory
`build/aurora-os-execution-20260903/`.

## Validation

The isolated CMake `os_execution_tests` target passes **12/12 tests**. The same
12 tests also pass with the new provider and test translation units compiled
under Clang ThreadSanitizer, with no sanitizer diagnostics. The existing Google
Test libraries used by that second run are not instrumented. Normal and
sanitized logs/XML are retained here.

The tests cover Boolean restore semantics, nonzero values other than one,
scheduler nesting and negative counts, preservation of interrupt state,
cross-thread exclusion for both kinds of critical section, both mixed release
orders, and explicit yield with/without scheduler exclusion. Six concurrent
threads also perform 12,000 protected increments through mixed interrupt and
scheduler calls and verify the final value and returned prior states.

The scope is CPU verification. No GPU tests or shared native xmake build were
run by this task. The parent owns full J3D/native integration validation.

Reproduce with the previously configured isolated Aurora CMake build:

```sh
python3 pc-port/notes/aurora-os-execution-20260903/verify.py --run
```

Without `--run`, the script verifies the retail words and records the existing
test results and source hashes. It requires the supplied ignored `main.dol`
and the existing build's Google Test sources/libraries for the sanitizer run.
