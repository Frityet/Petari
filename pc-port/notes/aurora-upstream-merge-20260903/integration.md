# Native integration after the Aurora merge

All 15 selected debug targets build against Dawn `v20260807.225922` on
Apple Silicon macOS with Homebrew LLVM. The first configuration command
inadvertently selected xmake's default release mode and Apple toolchain;
its attempted test build reported a missing debug-only target. The explicit
configuration below restored the prior debug/LLVM settings. No source change
was needed to compile the merged renderer.

From `pc-port`:

```sh
PATH=/opt/homebrew/opt/llvm/bin:$PATH xmake f -y -m debug -p macosx -a arm64 \
  --toolchain=llvm --sdk=/opt/homebrew/opt/llvm \
  --ar=/opt/homebrew/opt/llvm/bin/llvm-ar --runtimes=c++_shared \
  --target_minver=26.0 --aurora_dawn_version=v20260807.225922
```

The build uses `xmake build -j8` with all targets listed in
`run-integration.py`. Configuration and build logs are retained locally as
`configure-debug.log` and `build-debug.log`. The build completed successfully
in 34.749 seconds, with existing Game override warnings.

`python3 notes/aurora-upstream-merge-20260903/run-integration.py` runs those
executables serially, sets the real-disc resource input, captures each log
and executable hash, and appends exact outcomes to `integration-results.json`.
The script exits nonzero when any test fails; the existing walking failure
is not hidden or reclassified as a pass.

## Results

- Binder/KCL and all 29 Aurora-native groups pass.
- Original camera runtime, OnlyCamera, view interpolation, view service,
  stage-start camera, and actor-event camera all pass. The five latter
  suites contain 59 cases; disc-backed resource probes are enabled.
- Stage collision registration and triangle filtering pass.
- Original XanimeCore and J3D joint traversal pass.
- Gateway spin checkpoint passes.
- The full Mario Gateway walk still fails the existing assertion that Mario
  remains grounded across the real planet KCL seam, at frame 157. This is the
  same boundary observed before the merge. Original floor/state/writeback
  integration remains unfinished; the assertion and production movement
  code have not been changed for this merge.
- The actual Gateway showcase smoke test passes with 28 rendered frames:
  PlanetMap and animated Mario packet submission, visible actor center,
  GPU drawing, gravity acceleration, and exact planet KCL contact.

The frame-20 capture `showcase.png` was visually compared with
`original-binder-native-20260903/showcase.png`: Mario, terrain, background,
and framing are consistent with the pre-merge capture. The PNG bytes differ;
their hashes are in `showcase-comparison.json`. Neither this brief smoke run
nor the camera suites establish completion of the Gateway demo or general
rendering parity. The existing unusual circular ground rendering is present
in both captures. No Dawn validation error appeared in the showcase log.

This checkpoint changes Aurora, its dependency recipe, tests, and notes.
It makes no production `Game/` edits or game-specific compatibility rules.

## Final rendering correction and rerun

The retained GX depth-range pixel test exposed a merge interaction: upstream
tracks uniform changes separately, while our depth scale/offset remains in
that uniform. `set_logical_viewport` now marks it dirty when either near or
far depth changes. The test retains its exact color and clipping assertions;
its macOS setup now selects Metal and explicitly configures VI/framebuffer
size, and its indexed vertex storage survives until FIFO consumption.
The retained depth-snapshot and copy-filter tests also select the supported
platform backend.

All three GPU programs pass after that fix. The xmake showcase was then
rebuilt against the final GX source (`build-depth-invalidation.log`, 1.359
seconds) and rerun: it again passes 28 frames in 3.32 seconds with no validation
errors. The final frame-20 capture was inspected too; its exact binary and
image hashes are retained. This final run is the second showcase entry in
`integration-results.json`. CPU/camera/math sources did not change during this
isolated depth-uniform correction; those earlier passing tests were not
repeated. See `tests.md` for the independent CMake suites and their limits.
