# Focused actual OS thread lifecycle proof

The parent's one requested bounded headless probe compiled and ran once. Build exit **0**, run exit **0**, **38 assertions passed**. No production changes or repeat run were necessary. Runtime was 0.428754 seconds; binary SHA256 `fceb24fbc7026c110cf0307e5a9b63ae1c96a118de4a0543ae3a2ad0e26475e8`.

The probe directly compiles current `OSExecution.cpp`, `OSMutex.cpp`, and `OSMessage.cpp`; no substitute threading provider, Game TU, window, GPU, Xmake invocation, test sweep, or timing benchmark is involved. Main-thread observations of raw SDK fields use `GuestThreadExecutionScope`, and actual blocking message/join calls hand CPU ownership to the original SDK worker contract.

Coverage:

- OSCreateThread creates the supplied identity and initial suspension count 1; a queued message and repeated CPU yields do not enter the callback until resumed.
- The worker inherits actual guest callback allocation routing and returns the precise message payload through its reply queue.
- A blocked worker can be suspended twice and awakened by a queued command. Neither the pending wakeup nor only one resume permits execution; the final resume does, and suspend/resume return their original prior counts.
- Joining waits for the actual entry-point result; successful completion is terminated, and a second join fails.
- A separate worker owns one OS mutex recursively twice and blocks on an actual empty OS message queue. Cancellation ends it without returning from that receive, removes its queue links, releases both recursive lock levels and ownership links, and lets the actual main thread acquire the mutex. Join returns the cancellation sentinel.

`thread-lifecycle.cpp` is the complete probe, `result.json` records exact command and production source hashes, and `build.log`/`run.log` retain evidence. The only compiler warning is the pre-existing OSMutex ASSERT macro redefinition. This is focused native lifecycle proof, not validation of full GameSystem startup or a preemptive priority scheduler.
