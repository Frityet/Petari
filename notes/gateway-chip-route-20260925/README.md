# Original Gateway through Grand Star collection

A continuous fresh-save run reached the Grand Star through the original game systems. The user took manual control at frame18035; all scripted buttons/pointer/stick were cleared and controller processes stopped. The user completed the remaining rabbit, tutorial, all five star chips, three launches, both Goomba/key/cage encounters, the paired pipe and all16 reverse panels. No actor, transform, nerve, velocity, story flag or save edits were made.

| Sample frame | Observed original behavior |
| --- | --- |
|19990|Rosetta appears after the three rabbits.|
|24930→25230|Tower launch shoots and releases Mario.|
|25900,26020,26230,26480,27020|All five chips enter Got.|
|27710→27860|Second launch completes.|
|29410|Small-planet key cage opens.|
|30240→30330|Small-to-Middle launch completes.|
|31120|Chief Goomba key cage opens.|
|32660→32790|Paired pipe transit into Inside.|
|35420/35450|Panel observer completes; all16 panels reach End.|
|35780/35810|Grand Star WeakToWait, then collectible Wait.|
|36570|Grand Star StageClearDemo begins.|
|37330|Last complete trace sample; stage handoff subsequently aborts.|

Frames are first observed ten-frame samples, not exact transition times. `route-evidence.json` and `route-milestone-snapshots.jsonl.gz` preserve the selected states and exact original rows. `completed-panels.jpg` shows the finished panels during normal play; it is not a capture of Grand Star collection and has no exact frame stamp. The attempted later screenshot timed out as the process aborted and is not evidence.

## Remaining handoff failure and general fix

The process ended SIGABRT/exit-6 after700.23seconds, without timeout, with its executable unchanged and reaped. `handoff-crash.json` records the original diagnostic report: the async scene-destruction thread calls LiveActor resource release→draw-buffer removal→GXDrawDone→fifo::drain, while the main frame thread is also waiting in GXDrawDone. The assertion is **GX drain retired beyond its FIFO storage**. AstroGalaxy initialization was not observed. Grand Star collection is established; the complete transition remains unverified.

FIFO drain snapshots an absolute write cursor, then yields guest CPU ownership while waiting for completion. Another guest can issue commands and drain a later cursor before the first caller resumes. The old subtraction `target-bufferBase` then underflows. Aurora now treats an already-retired prefix as satisfied, preserves the monotonic storage cursor, and validates the remaining64-bit range before narrowing it. The assertion still catches a target genuinely beyond retained storage. No game-specific branch or bypass was added; no Game code changed in this fix.

A focused regression uses the real FIFO/decoder/OS guest-ownership implementation. A breakpoint holds a five-byte prefix; a newer drain retires ten bytes while the earlier caller is waiting to reacquire the guest CPU. The old code reproduces the exact assertion and exits-6. With the fix, it resumes, preserves cursor10, and accepts a subsequent command. The existing sleeping-callback/unregister case also passes. These fixtures stub GPU rendering; actual GPU/frame lifetime was separately checked in the preceding checkpoint.

Validation: two focused FIFO cases passed; `xmake build -y -j16 smg-pc` passed in7.13seconds. The CMake fixture needed its missing recorder-lock and scissor-refresh link surfaces updated; those are test-only renderer stubs. No broad test suite or synthetic stage-clear path was used. A new real stage-handoff attempt is still required.

## Provenance

Run source root `d88ec4b7bb0e888b826835b28e1b405f39616f28`, Aurora `522da71e3a4d529cfcf9c25ac0a2cc5a8de7f87d`, signed bundle SHA256 `9cd1045eef85bb7f5ee0d730093d25d156427aabd0caf9027d99d4f7548fc919`, PID65443. Full actor trace:435033816bytes/3734rows, SHA256 `0a487d703e509b8d0fe807c677883a9b240c1b6d2bda26f81280d7f464bdbdd4`. The full trace, NAND and raw logs stay local; compact lossless evidence and native JPEG are published. `src/compat` and `src/scene` remain physically absent and untracked.

The pre-handoff automated rabbit route looped through a crater warp. Its attempted short detour and surface planning are notes-only input/navigation work, not runtime compatibility changes and not claimed as successful routes. After manual handoff, no GUI keys or controller scripts were sent. The paused parent supervisor was resumed only after the game was terminal so it could reap children; it did not restart the game.
