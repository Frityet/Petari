# Original process startup — 2026-09-10

The previous published checkpoint (`3cdda5de` / `e9425d8b`) restored original Lumas, actor cameras, line collision, shared groups and scene prerequisites. This checkpoint supplies more of the original process graph and its general native services. The current playable Gateway preview still uses its bounded scene owner; full original GameScene startup and the bunny chase through Rosalina are not complete.

## Implementation

- Imported complete original GameSystem, object/scene controllers, frame/dimming/font helpers, NameObjRegister, stationed archive loader and HeapMemoryWatcher sources.
- Imported original FunctionAsyncExecutor/OSThreadWrapper and added actual suspended native OS workers, queue-based waits, join/cancel/cleanup and guest execution scopes. Added native fixed-unit heaps and a caller-owned MEM2 arena.
- Restored original GameSequenceDirector/Progress, comet and Luigi sequencing. Recovered missing stage-result and mail-size helpers from retail, reference first.
- Replaced the SaveDataHandleSequence facade with the original source. Restored original NANDManager worker, SaveDataHandler and banner flow, with general NAND file operations/quota handling and explicit UTF16/banner byte-order boundaries.
- Added true fixed archive mounting and volume registration, borrowed resource pointer identity, original heap disposal, and native model-resource ownership bridges. The previously empty embedded archive registration now uses the shared mount service; decompression returns caller-owned JKR heap bytes instead of cached vectors.
- Added Aurora CPU/GPU FIFO metadata snapshots backed by real producer/decoder cursors. Guest ring positions are mapped over the native command stream; unsupported breakpoint/callback and independent FIFO routing remain explicit.
- Recovered DrawSyncManager from retail and restored parameterized original functors. Corrected the decompilation's invalid clone alignment from direct retail instructions.
- Removed StorySequenceExecutor source-inclusion/compile wrappers, the obsolete SaveDataHandleSequence facade, and the duplicate scenario preload implementation. Necessary native declarations and pointer-width adjustments stay at SDK/build boundaries where possible.

Each subdirectory explains source provenance, implementation and remaining dependencies. Large objdiff text is published as `.json.gz`; original local files remain available. Raw objects, binaries and debug bundles are excluded.

## Validation policy and result

Following the user's request, use one combined showcase build checkpoint and a short launch after the implementation is coherent. Restarting that same build is only for concrete compiler errors; no broad fixture sweep or repeated long gameplay replay. The integrated build passed after closing missing declarations and original callback templates (62.511 seconds on its final compilation pass). The final build after FIFO metadata support also passed (105.289 seconds). One 240-frame Gateway launch with scripted walking/jump inputs then exited 0 in 4.926 seconds at 59.59 displayed FPS. It created all 78 ready authored rows. See `../expanded-gateway-showcase-20260910/smoke.json` and `smoke.log.gz`; binary SHA256 is `fc91f414cd3fc44501eae46e69fc16fbeb6529b1056545e6ab8fa9ef790faa83`. No additional gameplay replay was run.

One focused OS worker lifecycle probe was run because thread cancellation/ownership was newly implemented: 38 checks passed, build and process exit 0. It is not a full process boot test. Reference decompilation checks are bounded retail-recovery evidence, not additional gameplay validation. The two obsolete save-facade test sources were updated but not run.

## Remaining activation work

No partially populated GameSystem is installed to satisfy queries. Actual process activation still needs the complete child/provider graph: main-loop/display/FIFO callback and breakpoint behavior, full stationed layout resources, Home/system-error/reset owners, audio initialization boundary and NWC24 storage SDK. Complete scene activation must consolidate process save/story owners and scene registration, replace worker-incompatible thread-local scene bindings, and resolve original polling loops that rely on Wii preemption. See `process/README.md`.

The new process imports compiling does not establish that the full process runs. The user then requested a fresh showcase. A separately named Gateway Showcase bundle is being created from that same smoke-tested executable; the older movement app is retained. All four existing user changes remain outside these commits. Commits use `codex <codex@openai.com>`, with submodules published before the root gitlinks.
