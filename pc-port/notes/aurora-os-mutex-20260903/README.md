# Original OS mutex ownership on native threads

`OSMutex.cpp` supplies the five original mutex bodies from `src/RVL_SDK/os/OSMutex.c`. It preserves recursive count/owner behavior, intrusive owned-mutex links, non-owner unlock behavior, priority donation, wake-all, and the exact internal release function `__OSUnlockAllMutex`. There is no original API named OSUnlockAllMutexes.

The native changes to those bodies are includes/assert support and bit-preserving unsigned arithmetic for recursive count increment/decrement, avoiding C++ signed overflow while preserving the retail PowerPC wrap. `verify.py` checks that this is the complete source difference.

## Original compiler and retail evidence

The current root file compiles with the configured **GC/3.0a5.2, cflags_sdk, VERSION=0**. Objdiff reports **100% for all five functions and the complete 772-byte .text**:

| Function | RMGK01 address | Size |
| --- | --- | --- |
| OSInitMutex | 0x804AAAC4 | 0x38 |
| OSLockMutex | 0x804AAAFC | 0xDC |
| OSUnlockMutex | 0x804AABD8 | 0xC8 |
| __OSUnlockAllMutex | 0x804AACA0 | 0x6C |
| OSTryLockMutex | 0x804AAD0C | 0xBC |

The verifier checks every one of the target object's **193 instruction words** against the current supplied DOL, resolving its external branch relocations to current symbol addresses. DOL SHA-1 is `25c5959534b3c21246c6c7e42021b916b41fb578`. The existing split object is therefore independently tied to this exact executable, not assumed current from an earlier extraction. Original compiler command, object hashes, individual match percentages, and source hashes are recorded in `evidence.json`; generated objects/disassembly remain under ignored `build/aurora-os-mutex-20260903/`.

Retail evidence confirms that initialization leaves the link fields alone; uncontended and recursive acquisition increment the same count; acquisition only enqueues the mutex once; unlock decrements only when the caller is the owner; final unlock removes the owned link, restores effective priority, and wakes the complete waiting queue. Unlock-all ignores recursion depth, clears count/owner, and wakes each owned mutex. Root `OSThread.c::OSExitThread` and `OSCancelThread` both call this exact helper.

## Cooperative native boundary

The existing OSExecution emulated-CPU gate remains the synchronization authority. A participating host thread gets a real thread-local `OSThread` ownership record. Initial ownership fields follow `__OSThreadInit`: running, detached, base/effective priority 16, suspend zero, no waited mutex, empty join/owned-mutex queues. This record does not pretend to contain an executing PowerPC context or fabricated emulated stack.

When a mutex is contended, `OSSleepThread` publishes the actual waiting state, queue, and priority-ordered intrusive link under the CPU gate. A condition variable atomically drops that same gate while waiting and reacquires it before returning. The caller's interrupt-enabled bit remains unchanged. Merely waiting on a separate host recursive mutex while retaining this gate would deadlock the owner at OSUnlockMutex; that pattern is not used.

`OSWakeupThread` empties the original wait queue, marks each waiter ready, and notifies the native waits. There is no priority run-queue or native preemption claim. Waiting queue ordering and effective-priority donation do follow the original, including transitive donation through a blocked owner and reordering a waiting thread when priority changes. Native runnable ordering is determined by the host scheduler. The native OSGetThreadPriority word read additionally uses the shared gate to synchronize with another host's setter.

When a participating host thread actually exits, its TLS lifetime invokes `__OSUnlockAllMutex` under the gate and retires its identity. This implements release at the real native exit boundary. SDK-created/cancelled thread execution remains unavailable: no OSCreateThread, OSExitThread, OSCancelThread, suspend/resume, or forged virtual implementations were added. Future actual SDK thread lifecycle code must use the same owner identity/release helper.

Contended sleep while the scheduler-disable count is positive fails visibly. Retail SelectThread refuses such a handoff; silently releasing the scheduler gate would violate its contract. Likewise a native thread cannot exit with scheduler ownership left disabled. These are explicit unsupported execution boundaries. The existing cooperative provider still does not stop unrelated native workers or mask host signals.

Original `CurrentHeapRestorer` is a concrete consumer: its constructor locks MutexHolder1 and calls becomeCurrentHeap, which recursively locks that same mutex. JKRHeap and JKRSolidHeap use the same recursive mutex APIs for allocator operations. No Game source change or heap/global allocation override is part of this mutex tranche.

## Verification

The published production sources pass **23/23 native tests**, comprising 11 focused mutex tests and all 12 previous OSExecution tests. The same suite passes **23/23 with ThreadSanitizer**, with no diagnostics; existing Google Test static libraries are not instrumented. Coverage includes recursion and original links, non-owner try/unlock, count wrap, blocking with interrupts already disabled, scheduler exclusion, transitive priority donation, priority/arrival queue ordering and reordering, unlock-all, real native thread exit with a blocked waiter, and 6,000 contended recursive increments. The death fixture verifies the scheduler-disabled contention boundary.

Reproduce without a shared xmake or GPU build:

```sh
python3 pc-port/notes/aurora-os-mutex-20260903/verify.py --run
```

Without `--run`, the verifier checks source correspondence, current retail instructions, and retained results. Parent owns shared CMake/xmake integration and end-to-end runtime gates. Production files are `aurora/lib/dolphin/os/OSMutex.cpp`, `thread.hpp`, the extended `OSExecution.cpp`, and `aurora/tests/os_mutex_test.cpp`.

## Coordinated integration

Parent wired the production provider into Aurora CMake and xmake and both
provider files into os_execution_tests. The actual CMake target rebuild and
all23 test cases pass (cmake-native.log). This gate is independent of the
in-progress native heap and animation-owner changes.
