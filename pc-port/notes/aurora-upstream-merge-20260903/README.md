# Aurora upstream merge — 2026-09-03

The user explicitly requested merging upstream Aurora. Work is isolated to the
nested repository `pc-port/aurora`, on the existing `codex/macos-compat` branch.
The parent repository index and Game sources are not part of this merge.

## Verified upstream and range

The GitHub repository API identifies `Frityet/aurora` as a fork whose parent and
source are `encounter/aurora`. `git ls-remote --symref` independently identifies
that repository's default branch as `main`.

- Local starting commit: `deab54e` (clean and equal to `origin/codex/macos-compat`).
- Common ancestor: `1d10fa1bc502910a6336fdac32f31cd0ac39710d`.
- Incoming upstream: `f1189541e5d8b97fdf61946377853488d504d9df`.
- Incoming commits: 21 (`git log deab54e..f1189541`).
- Added the missing `upstream` remote: `https://github.com/encounter/aurora.git`.

The incoming range contains FIFO processing on a worker, typed register decoding,
separate graphics recording/encoding/frame/resource modules, render target layout
changes, atomic file IO, time scaling and clock APIs, SDL event-string retention,
and PSVECSquareMag rounding corrections. Dawn advances to upstream's
`v20260807.225922`, required by its immediate-data graphics API.

## Conflict resolution

The ordinary merge reported 21 conflicted paths. The resolution adopts upstream's
new structure and transfers local behavior into it:

- Removed `gfx/common.cpp` and `gfx/common.hpp`. Retained their local changes in
  `recording`, `encoding`, `frame_packet`, `resources`, and `types`.
- Tagged depth snapshots remain ordered FIFO requests. Their capture metadata and
  GPU dependencies belong to the individual recorded pass.
- Copy passes retain programmable vertical filter coefficients and clamp flags.
  Built-in draw commands carry their pipeline key alongside the new type-erased
  encoder so a copy never omits a source draw merely because its pipeline is cold.
- GX depth scale/offset remains in shader uniforms with clip distances and
  unclipped WebGPU depth. Upstream moves array offsets into immediate data; the
  retained depth pair has explicit padding so following matrix/vector uniforms
  remain aligned. Logical near/far changes invalidate the uniform cache as well
  as dimension changes: the retained GPU test demonstrated that omitting this
  adaptation allowed an equal-depth shifted primitive to overwrite the reference.
  The decoded viewport retains the retail 342 bias and 2^24 scale.
- Retail unsized GX arrays still upload only the span proven necessary by indexed
  draws/XF loads, with checked wide arithmetic. They now invalidate upstream's
  appropriate dirty bits and preserve its Reader and register decoder APIs.
- Kept resource destruction guards, replacement texture mip bounds, direct
  display-copy APIs, and the 16 MiB vertex buffer.
- Local direct copy-clamp mutation and public frame/display-copy queries synchronize
  with the new FIFO worker before accessing state shared with it. Copy-filter
  encoding constructs the complete payload without reading worker-owned arrays.
- Preserved the local base/platform/core dependency split and macOS drawable,
  shutdown and RmlUi event handling; added upstream IO/time/thread objects to the
  base target. Details are in `macos-lifecycle.md`.
- Preserved both upstream and local test coverage. Details are in `tests.md`.

The local Gekko quaternion/reciprocal implementation (`mtx.c`, `mtx.h`,
`ppc_math.h`) and legacy Metrowerks functional adapters remain byte-identical to
`deab54e`. The only incoming matrix-library edit is upstream's `vec.c`
PSVECSquareMag change: explicitly rounded x² and y² followed by fused z²+x² and
the final sum. No math approximation replaces the existing Gekko work.

## Validation

At source freeze, there are no unmerged paths and `git diff --cached --check`
passes. `verify-source.py` checks the upstream ancestry, preservation of the four
local math/functional files, the intended removal of common.*, and the retained
source invariants. Native build/runtime results and the final merge/push commit
are recorded below when complete. Source inspection alone is not runtime proof.

### Native integration result

All 15 selected native targets compile with the updated Dawn and explicit debug
LLVM configuration. The parent ran the 15 programs serially: 14 pass, including
all 29 Aurora-native groups, Binder/KCL, collision registration/filter queries,
all selected original camera suites, XanimeCore/J3D, Gateway spin, and the actual
Gateway showcase. The full Mario walk test retains the previously observed
frame-157 seam-grounding assertion failure; this merge does not claim to fix it.
The showcase ran 28 frames without GPU validation errors. The parent inspected
its screenshot beside the pre-merge image and reported matching scene/Mario
presentation. Commands, tested binary hashes and logs are retained in
`integration-results.json` and `run-integration.py`.

### Focused Aurora result

All 289 requested CPU cases pass: 233 FIFO, 9 recording, 5 time, 3 OS time, 7 IO,
6 worker/thread, 2 destruction, 18 texture-cache, and 6 texture-streaming cases.
Two retained death tests now match the central Reader's checked FIFO-overrun
message instead of the deleted per-command diagnostic.

All three retained render programs pass on Metal with their pixel/depth oracles
unchanged: exact GX Z-scale/offset, tagged depth snapshot boundaries, and display
copy filtering. Their platform setup selects Metal on Apple and Vulkan elsewhere.
The Z-scale fixture now configures actual VI dimensions and retains its indexed
vertex array through the frame, as required with asynchronous FIFO consumption.

The complete run has 318/319 GoogleTest CPU cases, one passing standalone
functional program, and three passing GPU programs: 322/323 checks overall. The
one failure is `PcmAudioMixer.ScheduledGateReleasesALoopingSequenceLayer`. Its mixer
source, header and test are unchanged from the pre-merge commit; this is recorded
as an unchanged-source failure, not claimed as a passing gate or as independently
verified baseline runtime behavior. Audio is outside this merge's functional edits.
The native CPU/camera integration run preceded the isolated near/far uniform
invalidation correction; the three Metal programs and final native showcase are
rebuilt/rerun after that correction.

### Completed merge

Merge commit `db4197599eb5f6441c39761f8ad6c987998102b7` is pushed to
`origin/codex/macos-compat`. Its parents are the starting local commit and the
verified upstream main commit. The nested working tree is clean; an independent
`git ls-remote` confirms the pushed branch matches HEAD, and
`git merge-base --is-ancestor upstream/main HEAD` succeeds. See
`merge-result.json`. The parent repository records the resulting submodule
pointer and notes in its own checkpoint.
