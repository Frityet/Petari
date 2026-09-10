# Original input ownership — 2026-09-10

Previous goal turn: **progress**. Root036aae7c2/cb9e66737 restored actual six-chunk UserFile ownership, original execution encoding and host string boundaries; decomp499e1034d contains recovered source and latest upstream73f5b40dc. The refreshed movement app passed13/13 replay checks at59.906 FPS. The full Gateway bunny/Rosalina goal remains active.

This continuation targets complete original WPad/GamePadUtil processing rather than native raw-input substitutes. A parallel generalized Aurora BRLAN correction carries demonstrated original interpolation semantics from the upstream review. Source changes, retail evidence, linked validation and publication will be recorded here as work proceeds.

Preserve the user-staged DISCREPENCY_REPORT.md/MACOS.md deletions, existing original-sequence-galaxy-move note edit and untracked walking packager. One shared Xmake lane; all new commits/pushes by codex.

## Implementation checkpoint

- Recovered missing/corrected WPadAcceleration, WPadHVSwing, WPadLeaveWatcher, WPadRumble and GamePadUtil behavior in decomp first. Wii evidence lives in the named subfolders. Whole native Game source copies match that reference, including WPad, WPadButton and WPadInfoChecker. No native controller algorithms were inserted into Game.
- Deleted GamePadUtilCompat, OriginalWPadRecords and OriginalWPadRumblePause. The full original WPad update now runs every child in retail order. OriginalWPadHolder contains the original holder methods with a single explicit native owner lookup until the actual GameSystem owner graph is available.
- WPadOwnership owns the actual holder, four SDK read buffers and two complete Game pads. The original constructor initializes its process-static rumble callback array once on the host heap through a temporary original WPad; ordinary owner objects retain their JKR domain. Native typed destruction retires every child. Exact prior rumble callback pointers (including menu/pause instances and null) are preserved across nested owners.
- Aurora implements typed connection/extension callbacks, one outstanding asynchronous info query per channel, sensor-bar settings, sampling initialization/reset and allocator registration. Successful info completion occurs in the owning input pump; rejected queries invoke the original synchronous error callback. The KB+M virtual remote reports a full battery and no speaker. Speaker enable requests explicitly return WPAD_ERR_INVALID; original static speaker connection methods handle that absence.
- A dedicated SDK client scope retains/restores callbacks and pending requests. Review found a callback-retirement use-after-free in the first implementation; a reproduced ASan failure and the corrected passing run are retained in wpad/. Dispatch now keeps buffers in their owning client and checks client generations after callbacks.
- Keyboard spin supplies a 200 ms physical acceleration pulse through Aurora, with 1 g gravity at rest. Original Game gesture detection owns swing state, history, thresholds and trigger timing. Native swing boolean APIs were removed.
- Aurora BRLAN uses the original step/Hermite lookup, tolerance, duplicate-key selection and unfused arithmetic. Six new regression groups fail the prior implementation; the fixed existing suite passes 10/10 and a 28,672-sample comparison matches original results exactly.

## Upstream

The reference first contained upstream73f5b40dc. A fresh fetch found two more JKernel updates, now merged cleanly as ec3406dfc (upstream d1ae0a05cc023d52ecdcbc7731c8c79f0cb84dc6). Native JKRHeap adopts the actual ARALT storage symbol and disposer traversal while keeping host-width address ranges, allocation provenance and finalizers. JKRDvdAramRipper remains an inactive native owner route; its Wii/ARAM addresses were not transplanted into host decoding. Detailed disposition is in upstream/README.md.

## Validation

After the final header/allocator cleanup, all six selected targets build and link. The five executable checks pass: original pointer/stick input, complete WPad ownership, acceleration, keyboard gestures and the actual PlayerUtil owner route. `cleanup-validation.json` records each result. The owner fixture exercises expected allocation failure, exact paused-rumble restoration, pending-query suspension/cancellation, four-channel reads and repeated complete teardown.

The first two new fixture runs exposed invalid fixture assumptions: generated extension records omitted WPAD_ERR_NONE, and the extension gesture assertion ignored the original constructor's 2 g threshold. Only test inputs/expectations were corrected. Production behavior was preserved; the corrected linked fixtures pass.

Aurora's existing WPAD suite passes31/31 under ASan/UBSan. Existing BRLAN tests pass10/10 under both sanitizers and independent CMake, with28,672 exact original-result comparisons. New Game source/header comparison passes18/18 byte-identical to decomp (`game-source-closeness.json`).

The first960-tick1280×720 replay completed all13 checks with finite movement/camera, all four WASD directions and two complete jumps at59.808 FPS, exit0. Final exact-binary replay and package publication are recorded separately below when complete.

The full Gateway sequence through bunny capture and Rosalina is still unfinished. This is the bounded movement demo. The source-recovered scalar world-stick helper and physical input pipeline are active; the new gesture fixture proves original detector responses, not a complete unlocked Mario spin animation. The five previously documented legacy owner-fixture failures and inactive ARAM/DVD owner route remain outside this checkpoint.

Large compiler/objdiff logs are committed as `.gz` siblings of their recorded paths; `compressed-evidence.json` lists the original names and hashes. Executables and debug-symbol bundles remain local build artifacts.

Final binary `019f7be6e2f99ef7db96cb573f9334eba41f63739271f327d7266a68eb8f0f30` passed13/13 replay checks over960 ticks at1280×720 and59.939 FPS, exit0. `demo-runtime-final.json`, `demo-runtime-final.log.gz` and `demo-validation-final.json` retain the exact result.
