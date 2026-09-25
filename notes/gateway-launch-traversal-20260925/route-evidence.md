# Gateway launch attempt: terminal evidence

The fresh-save route completed all three rabbit catches, climbed the tower, finished the Rosalina tutorial, broke the crystal gate and activated the first launch star. **Driver864 reached Shoot at frame24300, then the process crashed with SIGSEGV** while rendering its path. No arrival on the BlackHole planet, later progression or clean completion is established.

This is the run launched from source checkpoint `c3f55804a08e0fde8e91b9c8d3725f9f8c1d9937`. Its executable SHA256 remained `dd622defca96d44fecefe82d865e5019c201fdcd4c9bedfece6105fa9b2bffcc` before/after the run. `route.json` records exit **-11**, elapsed **425.169s**, no timeout, and the process reaped. The requested100000-frame budget was not completed; `completed_frames` is null and `verified_bounded_completion` is false. Initial dirty state is preserved in the before-status/patch artifacts, not silently treated as a clean checkout.

## Observed milestones

Frames below are exact trace sample indices (10-frame sampling); the compressed extract retains15 of these milestones. state transitions may have occurred between samples. Rabbit completion/talk dates also agree with `operator.jsonl`; the successful climber handoff is recorded in `climb.jsonl`.

| Frame | Observation |
| --- | --- |
| 90 | Opening: Mario present; tutorial, cages and first driver not yet active |
| 1400 | Rabbit operator binds this run’s authored rabbit occurrences |
| 3160 | Bush rabbit1090 first sampled Caught |
| 3530 | Bush Tico1075 dialogue completed |
| 4660 | Pipe rabbit1085 first sampled Caught |
| 5030 | Pipe Tico1069 dialogue completed |
| 7630 | Hole rabbit1095 first sampled Caught |
| 8120 | Hole Tico1081 enters WhiteOut after dialogue |
| 8290 | Tower crystals1013/1016/1020 appear |
| 8590 | Rosetta1060 alive and visible; rabbit operator completes |
| 9010 | Climber begins ordinary controller stair ascent |
| 13860 | Climber reaches waypoint27 |
| 13980 | Spin tutorial entry; Tico903 alive and Rosetta at demo pose |
| 15000 | Scheduled renderer screenshot during Rosalina tutorial |
| 16990 | Rosetta1060 and Tico903 retire; climber emits tutorial-finished-handoff |
| 21190 | Crystal1013 first sampled Break, hidden |
| 21240 | Crystal1013 dead |
| 22100 | Crystal1016 first sampled Break, hidden |
| 22110 | Crystal1020 first sampled Break; driver864 Appear |
| 22120 | Driver864 first sampled alive Wait |
| 22160 | Both remaining gate crystals dead |
| 22940 | Driver864 Capture step4; Mario bound |
| 22990 | Driver864 Capture step54; Mario still bound |
| 23000 | Capture expires; driver864 Wait and Mario unbound |
| 24250 | Before successful activation: driver864 Wait; Mario unbound |
| 24260 | Driver864 ShootStart step7; Mario bound |
| 24290 | Driver864 ShootStart step37; Mario centred on star |
| 24300 | Last trace: driver864 Shoot step0; Mario bound |

The first Capture expired without shooting: Capture step54 at22990 became Wait with Mario unbound at23000. A later normal-control activation produced ShootStart step7 at24260, step37 at24290, then Shoot step0 at24300. Mario remained bound and located at the star centre `[15080,-8042.270,7580]` in the last sample. The run therefore proves activation, not the authored300-frame flight or landing. Spin-permission/gesture bits are not directly recorded; cage Break and driver activation are the behavioral evidence.

## Crash boundary

The final debug-input log entry shows frame24303 processing. The macOS crash report identifies `EXC_BAD_ACCESS` / `KERN_INVALID_ADDRESS` at **0x00000000cc008000**, with `SpinDriverPathDrawer::sendPoint` (`SpinDriverPathDrawer.cpp:369`) inlined into `SpinDriverPathDrawer::draw` (`:350`) at the top of the faulting main-thread stack. See `launch-crash.json`; this note deliberately does not duplicate its image list, registers or full stack. The exact crash frame is not directly logged; the last complete actor sample is24300, and the last input revision is logged at24303. The completed-frame counter is unavailable after the fault. This pre-fix run does not validate subsequent FIFO/GX changes or builds.

## Saved visual evidence

Each image below was inspected for its actual contents. The five UI captures are JPEG bytes with matching `.jpg` suffixes, saved by the parent operator during this run. They carry no exact frame association in the saved artifact; filesystem timestamps are not substituted for frame evidence. The PNG is the configured renderer capture at15000.

| Artifact | Frame provenance | Visible evidence |
| --- | --- | --- |
| [tower-ascent.jpg](tower-ascent.jpg) | Not frame-stamped | Mario on authored tower stairs; Rosalina visible above. |
| [route-frame15000.png](route-frame15000.png) | 15000 | Rendered Rosalina tutorial close-up. |
| [crystal-spin.jpg](crystal-spin.jpg) | Not frame-stamped | Mario beside remaining crystal cluster, Luma and spin/shard effects; does not show all cages gone. |
| [launch-reveal.jpg](launch-reveal.jpg) | Not frame-stamped | Revealed orange launch star with Mario and crystal fragments. |
| [first-launch.jpg](first-launch.jpg) | Not frame-stamped | Luma dialogue beside revealed launch star; Mario remains on tower. |
| [launch-flight.jpg](launch-flight.jpg) | Not frame-stamped | Mario inside rotating launch star at tower; no interplanetary flight or landing visible. |

In particular, **`launch-flight.jpg` shows the star spinning on the tower**, not Mario traversing space. `first-launch.jpg` shows Luma dialogue before departure. Neither image overrides the terminal trace/crash evidence.

## Compact provenance

`milestone-snapshots.jsonl.gz` contains **15 exact raw trace lines**, all original actor/controller fields preserved, selected from the terminal2431-snapshot trace. It is a milestone extract, not another copy of the full trace. The source trace SHA256 after termination is **`bb44be4d918f704d20314238031d5f721f1cd0f3dc5c08cc16e6de9b07f80777`**. `route-evidence.json` records the selected indices, compact actor states, terminal facts, source hashes and screenshot hashes without copying raw logs or crash metadata.

This evidence pass changed only the two summaries and compressed milestone extract. No production code, controller input, builds or tests were changed or run.
