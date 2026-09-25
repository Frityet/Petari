# First launch: finalized route evidence

**The first tower launch completed its flight and Mario reached the BlackHole planet.** The flight evidence below freezes completed trace rows through frame25360 from PID62913, launched at source `ff6bf1b4572b86a58d48c44738ac67e770c4de4d` with binary SHA256 `a0a0912c63f60d8197f2aae7142714eb71bfb9f0ba814f750c31f6b82a78b25b`.

The initial extraction was requested during gameplay; the run is now terminal. The later crash and finalized provenance are recorded below. **Successful first flight does not establish clean termination or complete Gateway progression.** The full actor trace remains local; only selected exact rows are compressed for review.

| Sample frame | Evidence |
| --- | --- |
| 22930 | Driver864 ShootStart step8; Mario bound. |
| 22970 | Shoot step1; Mario at the star centre, bound. |
| 23120 | Shoot step151; Mario at [14492.821,-5730.224,1637.535], bound in mid-flight. |
| 23270 | CoolDown step0; Mario released near authored endpoint [13436.776,-2650.707,-3707.371]. |
| 23800 | Mario unbound, stationary at [13420.260,-2565.819,-3838.281]; movement_low_word0x60020000 has grounded bit0x40000000 set. Ground triangle identifies host1142. |
| 25280 | Cage1007 Break step3; its YellowChip799 appears in Wait. |
| 25340 | Chip799 first sampled Got, dead; cage1007 BreakAfter. |
| 25360 | Chip799 remains Got; Mario unbound with grounded bit0x40000000 set; ground triangle identifies host1142. |

The ten-frame trace sampling makes these first-observed states, not exact transition times. Shoot advances from step1 at22970 to release300 frames later. The stored `binder.ground` is null, and `ground_triangle` can remain stale during a fall: the triangle alone does not prove grounding. At both23800 and25360, the actual `movement_low_word` is **0x60020000**, with grounded bit **0x40000000 set**, while Mario is unbound. At23800 his arrival position is stable and velocity is zero; at25360 he remains grounded while moving slowly (velocity approximately[1.072,0.162,0.360]). These flags and motion observations support landing; triangle host1142 identifies the BlackHole surface. [blackhole-arrival.jpg](blackhole-arrival.jpg) supplies the visual context. That saved JPEG shows Mario on the rocky planet beside crystals and Luma with the normal HUD. Its exact frame is not recorded; it does not alone prove chip collection.

`partial-flight-snapshots.jsonl.gz` retains eight exact original JSONL rows, including all actors and controller fields. The JSON records selected states, fixed-artifact hashes and the finalized full-trace hash below. One chip and first-planet arrival do not establish the five-chip gate, subsequent launches or Grand Star completion. No production code, input, builds or tests were changed or run by this evidence pass.

## Later terminal addendum

The same run subsequently ended **SIGSEGV / exit-11** after485.462s, without timeout; its process was reaped and executable hash remained unchanged. The last complete actor sample is26810, with Mario in `MarioActorNrvGameOverBlackHole` step246; the last input revision is logged at26813. The exact crash frame and completed-frame count are unavailable.

`restart-crash.json` reports `EXC_BAD_ACCESS` at0x10 on thread5, through Dawn `ObjectBase::GetDevice` → `CommandEncoder::APIFinish` → `wgpu::CommandEncoder::Finish` → Aurora `submit_frame_prefix` (`frame.cpp:737`). Root identifies a null command encoder in the render worker; any subsequent frame-pipeline fix is outside this run’s evidence. This later BlackHole death/restart failure is separate from the former direct FIFO path-drawing crash. **The successful first flight, landing and one-chip pickup remain established; clean restart and further progression do not.** No later fix or build is validated by this run.

## Final provenance and compact review files

The terminal full actor trace contains **2682 snapshots / 307723325 bytes**; SHA256 **`4d244613df809f01e11c95d821f1269151814b1b37a1abfc34f69da54dc968cc`**, computed after `route.json` recorded process reaping. Its last complete row is26810. No full-trace gzip or duplicate was created. `terminal-snapshot.jsonl.gz` preserves that one exact last row separately from the eight flight snapshots.

The smaller route/input, rabbit operator, stair operator and navigation logs are preserved as lossless `.gz` copies, with original bytes/hashes recorded in the JSON. Raw local files remain untouched. `route-frame15000.png.gz` is the configured renderer screenshot compressed as a file, without image editing; decompress it to view. `blackhole-arrival.jpg` is already compressed and remains the direct visual arrival artifact.

| Compressed artifact | Bytes |
| --- | ---: |
| `route.log.gz` | 46547 |
| `operator.jsonl.gz` | 33091 |
| `navigation.jsonl.gz` | 20587 |
| `climb.jsonl.gz` | 44576 |
| `route-frame15000.png.gz` | 2934623 |
| `terminal-snapshot.jsonl.gz` | 25317 |

Terminal lifecycle: requested100000 frames; exit-11 after485.462s; no timeout; process reaped; binary SHA256 unchanged. The exact crash frame remains unknown: actor sample26810 and input revision26813 are separate observations, not a completed-frame counter.
