# Original DVD callback execution and cooperative waits

This checkpoint extends the published original FileLoader/DVD descriptor work. It does not change Game source or replace the original file worker, request queue, or archive mounting behavior.

## Callback ownership and delivery

Each actual DVD command captures the caller's `callbackGuest` allocation policy in native POD command storage. It never takes over `DVDCommandBlock::userData`. The worker finishes and closes its operation-owned host stream before entering the shared `GuestInterruptExecutionScope`. Inside that scope it observes any cancellation requested while waiting for the CPU, publishes the final command state and transferred size, and calls the actual client callback. Publication and callback delivery therefore belong to the same serialized guest execution interval. The callback obtains the captured allocation policy and uses the embedding application's current SDK heap selection; no heap pointer or heap lifetime is copied into the command.

Active asynchronous cancellation records its own callback and allocation policy, returns immediately, and rejects a duplicate active cancellation. The actual completion interrupt first invokes the canceled read with original SDK result -3 and then invokes the cancellation request with result 0 and the same command pointer. Queued/immediate cancellation runs on its caller with interrupts masked, preserving the caller's current context and scheduler nesting. This is intentionally distinct from an I/O interrupt. A cancellation request's own callback retains its caller's allocation policy independently of the canceled read's original callback. Low-level completion callbacks enter the same shared interrupt boundary, and the existing reset-cover callback registration retains its allocation policy under the guest CPU gate. Existing low-level synchronous timing is unchanged. Unsupported stream service behavior remains outside this checkpoint; the shared cancellation constant is corrected from the previous -6 to the SDK's -3.

## Native waits and lock order

The actual DVD worker's synchronous wait, synchronous active cancellation, close/drain, synchronous cancel-all, and external stop/join now release an outer guest CPU ownership scope while waiting for the native worker. Condition-variable waiters release the DVD native mutex before restoring the guest CPU and then reacquire/recheck the predicate. Callbacks always execute outside that native mutex. This allows an original callback to post to an SDK queue, or re-enter an ordinary DVD operation, while another caller waits for it to finish.

Cancellation-all follows the original pause/cancel/resume ordering. DVDPause and DVDResume now use that same actual worker state: a paused worker retains queued commands and allows an existing operation to finish. Cancel-all and stop pop pending descriptors one at a time before delivering each callback. A nonempty check avoids entering guest TLS during empty static-owner retirement; actual removal then acquires the guest CPU and rechecks the queue first. The direct SDK close/drain path likewise acquires the CPU before it removes a queued descriptor. They do not retain a vector snapshot that a callback could invalidate by canceling and freeing another descriptor.

The native catalog lifetime barrier from the prior checkpoint remains authoritative for host overlay operation payloads. The callback starts after that operation's handles and barrier are released. A stale catalog/disc generation fails the actual request and still receives the same captured callback delivery policy; it does not acquire a host stream from the new catalog.

## Validation

Final fresh compilation/link and real-disc runs are recorded in `dvd-results.json` and `file-loader-results.json`. All **58 DVD tests passed**, and the actual original FileLoader passed callback allocation/heap selection plus **three complete retirement cycles**. The standalone DVD binary SHA-256 is `9367a52604958561ff3233cec364b6be684f4e2de4224fb77fb8d641c40223b9`; the FileLoader binary SHA-256 is `e6a3e1ac8c495bea5776ffeea18c0f00452294068f094a1a32de709d5e020e15`.

The isolated `sanitizer/dvd-results.json` run passes the same 58 cases with ASan and UBSan on the changed production DVD source and test translation unit (support libraries remain their existing non-sanitized builds). Its binary SHA-256 is `e4615b0cd2bc1a80efb6d78c47f42053cc7580cda20803f9c1fbda9b0b221678`. `native-compile.json` retains the preliminary direct LLVM checks; the final fresh compiler/link commands are in the runtime receipts. `REVIEW.md` records the findings closed before publication and the final independent review.

An intermediate run exposed an empty-queue shutdown path entering guest TLS after its teardown. The production correction and independent process-exit regression are both included in these final passing results.

The existing DVD suite now additionally checks:

- Actual OS-created thread waiting on an actual SDK queue, woken by a read callback; the ready thread does not run until interrupt callback return. Both guest and host submissions are covered on the same DVD worker, including policy and caller state restoration.
- Active asynchronous cancel and cancel-all return before a blocked read completes, reject a second cancellation, then deliver original -3 and cancellation 0 with the actual block in the same interrupt, preserving separate allocation policies.
- Queued cancellation preserves the caller context, masks interrupts without adding scheduler nesting, retains original-read policy, and separately delivers the cancellation request callback under its own policy.
- Close, active cancel, cancel-all, and stop/join while the caller owns the guest CPU and the host read is blocked. The actual completion callback must enter the CPU and finish before the retiring caller returns.
- Discarded stale-catalog read delivers its real error callback without opening either old or replacement payload.
- Pause retains queued work; cancel-all resumes the worker and returns 0/nullptr when idle, while terminal individual cancellation retains its original state. Another regression cancels and deletes the middle queued descriptor inside the first callback and verifies exactly-once callback order 1,2,3 for both cancellation and close of the middle descriptor.
- A completed read callback may reclaim its own descriptor before cancel-all: the native worker retains its address for retirement synchronization but no longer presents it as an executing command or dereferences it.
- Host disc-service replacement/close on its own worker is rejected before mutation; the subprocess regression observes that diagnostic. Ordinary descriptor `DVDClose` inside a callback remains supported and is covered by the existing close/reopen/read regression.
- A host stop blocked on guest CPU leaves its queued descriptor discoverable, allowing that guest to cancel and reclaim it before stop proceeds.
- Another subprocess intentionally bypasses fixture teardown and exits with an idle DVD worker after guest thread adoption, exercising standard TLS/static destruction ordering. An empty pending drain must never re-enter already-retired guest TLS.
- Reset-cover callback registration retains policy and safely clears itself from inside its interrupt delivery.
- The existing 64-iteration descriptor reuse test now holds the guest CPU across synchronous reads, exercising the native completion wait rather than relying only on host callers.

`OriginalFileLoaderTests.cpp` now supplements its three original process retirement cycles with actual callback allocation. Submission occurs before switching the original SDK current heap to a real child `JKRExpHeap`; the callback's ordinary `new[]` must belong to that selected child for a guest submission and to host storage for a host submission. The callback wakes an actual OS message queue. Both paths retain their bytes, preserve full-pointer caller data, restore caller allocation policy, and reclaim every heap byte after explicit callback/child retirement. The fixture now explicitly closes its host disc owner during ordinary scope retirement; the separate process-exit regression proves idle static-owner shutdown independently.

## Native ABI and constant audit

`abi.json` compares the prior committed SDK header against the new header using actual native compilation. `DVDCommandBlock` stays 88 bytes and `DVDFileInfo` stays 120 bytes; FileInfo `startAddr`, `length`, `callback`, and `nativeGeneration` remain at 88, 92, 96, and 104. The captured policy byte uses existing command tail padding. No caller-owned field moves. `DVD_RESULT_CANCELED` has no named users in `src/`; the named usages are confined to Aurora's DVD implementation and tests. Original Game callers retain their original source literals/ordinary error branches.

## Limits

This is callback execution and ownership evidence, not full GameSystem startup, Gateway gameplay, emulated DVD hardware timing, or audio streaming. Low-level helper APIs retain their pre-existing synchronous implementation; full hardware drive-state/error-retry behavior remains outside this worker boundary. The source continues to require nonblocking interrupt callbacks, as the original SDK does. Host disc-service close/replacement from inside its worker is an explicit invalid reentrancy boundary: the worker cannot join itself, and no detached worker is reported as retired. Applications must request that host-service operation from outside the callback. SDK descriptor close/reopen remains supported. Broader simultaneous host disc-service open/close publication from multiple native callers is not claimed by this checkpoint. ResourceHolderManager ownership migration remains separate work.
