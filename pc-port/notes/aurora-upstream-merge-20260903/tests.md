# Aurora upstream merge: test preservation

Resolved only the four delegated Aurora test conflicts:

- `tests/CMakeLists.txt`
- `tests/gx_fifo_test.cpp`
- `tests/gx_test_common.hpp`
- `tests/gx_test_stubs.cpp`

The merged CMake file retains all 15 local executable targets and all 10
upstream executable targets, resulting in 19 distinct targets. The FIFO target
now includes both the local destruction-state source and the upstream register
decoder/thread sources. The upstream recording, thread, I/O, and time tests remain
registered alongside the local depth snapshot, copy filter, depth range,
destruction, input, audio, and resource tests.

The merged FIFO file contains 214 test cases. All 190 local test names remain.
Of 189 upstream names, 186 remain unchanged; the other three have the following
equivalent or stronger local coverage:

| Upstream case | Retained case |
| --- | --- |
| `DestroyCopyTex_EmitsAuroraDestroyCommand` | `DestroyCopyTex_WhileRendererAliveEmitsStablePointerValue` |
| `DestroyTexObj_EmitsAuroraDestroyCommandAndClearsIdentity` | `DestroyTexObj_WhileRendererAliveEmitsStableValueAndClearsIdentity` |
| `DestroyTlutObj_EmitsAuroraDestroyCommandAndClearsIdentity` | `DestroyTlutObj_WhileRendererAliveEmitsStableValueAndClearsIdentity` |

The texture and TLUT cases additionally check repeated destruction and ensure
decoding does not create a binding for the destroyed identity. Both upstream
`MarksLoadedSlotNoCacheUntilReloaded` tests are preserved in full: they inspect
the loaded slot after each separately captured FIFO record. These complement
the local same-buffer load/destroy ordering and post-renderer-shutdown tests.

The fixture shuts down the FIFO worker before shutting down destruction-state
tracking. Test renderer symbols follow upstream's `Resources` and recording
interfaces; obsolete free buffer/stat globals are removed. The copy resolve
stub preserves both the upstream atomic resolve counter and the local captured
`CopyFilter` values. The coordinator confirmed `CopyFilter` is retained in
`gfx/types.hpp` and as the final `resolve_pass_into` argument. The scissor stub
uses the current `const ClipRect&` signature.

Validation at source freeze: no conflict markers in the four owned files;
`git diff --check` passed; executable and FIFO test inventories compared against
merge stages 2 and 3. No configure, build, or test run was performed during this
resolution, so execution remains a gate for the coordinated completed merge.

## Isolated macOS execution

Built the merged CMake test targets in `build/aurora-upstream-merge-tests`,
separate from the parent xmake output. The build used Apple Clang, Debug,
arm64, and the already installed Dawn `v20260807.225922`, SDL `3.4.10`, and
Abseil `20240722.0` packages. CMake compilation was limited to two jobs. DVD,
CARD, and RmlUi were disabled for this test tree; this run does not validate
their separate dependency closures.

From the Petari repository root, the exact configure command was:

```sh
cmake -S pc-port/aurora -B build/aurora-upstream-merge-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH='/Users/frityet/.xmake/packages/d/dawn-build/v20260807.225922/f85799e820484cd0ac3a29330792249b;/Users/frityet/.xmake/packages/l/libsdl3/3.4.10/b94428e1287f440b9b14c181168b4faa;/Users/frityet/.xmake/packages/a/abseil/20240722.0/452e715454324f4c831156d3357589b0;/opt/homebrew' \
  -DAURORA_DAWN_PROVIDER=system -DAURORA_SDL3_PROVIDER=system \
  -DAURORA_ENABLE_DVD=OFF -DAURORA_ENABLE_CARD=OFF -DAURORA_ENABLE_RMLUI=OFF
```

The initial build and the additional retained test build were:

```sh
cmake --build build/aurora-upstream-merge-tests --parallel 2 --target \
  gx_fifo_tests gfx_recording_tests time_tests os_time_tests io_tests \
  render_worker_tests gx_destruction_state_tests gx_z_scale_offset_render_test \
  gx_depth_snapshot_render_test gx_copy_filter_render_test \
  gx_texture_cache_tests texture_replacement_streaming_tests

cmake --build build/aurora-upstream-merge-tests --parallel 2 --target \
  gx_fifo_tests functional_test brlan_tests audio_tests wpad_tests \
  rfl_character_resource_tests os_alloc_tests
```

All named targets built. After the depth-uniform correction described below,
the final GPU relink was:

```sh
cmake --build build/aurora-upstream-merge-tests --parallel 2 --target \
  gx_z_scale_offset_render_test gx_depth_snapshot_render_test gx_copy_filter_render_test
```

Each GoogleTest binary was invoked directly as
`build/aurora-upstream-merge-tests/tests/<target> --gtest_output=xml:<absolute-notes-dir>/<target>.xml`.
`functional_test` and the three standalone GPU executables were invoked without
arguments. A Python subprocess runner imposed a 120-second timeout on each
process and ran them serially. The final GPU run gave each executable distinct
`XDG_RUNTIME_DIR`, `XDG_CONFIG_HOME`, and `XDG_CACHE_HOME` directories beneath
the isolated build tree, with mode `0700`, and used the actual Metal backend.
The parent released the GPU before these tests ran.

`ctest --test-dir build/aurora-upstream-merge-tests --show-only=json-v1` also
confirmed all 323 cases are registered. The execution results below come from
the direct executable runs, not a claimed full `ctest` execution.

| Executable | Passed / total |
| --- | ---: |
| `gx_fifo_tests` | 233 / 233 |
| `gfx_recording_tests` | 9 / 9 |
| `time_tests` | 5 / 5 |
| `os_time_tests` | 3 / 3 |
| `io_tests` | 7 / 7 |
| `render_worker_tests` | 6 / 6 |
| `gx_destruction_state_tests` | 2 / 2 |
| `gx_texture_cache_tests` | 18 / 18 |
| `texture_replacement_streaming_tests` | 6 / 6 |
| `brlan_tests` | 4 / 4 |
| `audio_tests` | 11 / 12 |
| `wpad_tests` | 3 / 3 |
| `rfl_character_resource_tests` | 6 / 6 |
| `os_alloc_tests` | 5 / 5 |
| `functional_test` | 1 / 1 |
| `gx_z_scale_offset_render_test` | 1 / 1 |
| `gx_depth_snapshot_render_test` | 1 / 1 |
| `gx_copy_filter_render_test` | 1 / 1 |

All 289 specifically requested CPU cases and all three real GPU programs pass.
The additional retained suites bring the overall result to 322/323 cases,
with 17/18 executables succeeding. `cmake-final-results.json` records final
process outcomes. Individual `.log` and GoogleTest `.xml` files retain details;
earlier `cmake-cpu-results.json` records the initial run before two diagnostic
expectations were updated, and is superseded by the final results.

## Issues exposed and resolved

Two retained FIFO death tests expected command-specific overrun messages from
the old decoder. The merged central `Reader::take` correctly rejected both
truncated packets. Their assertions now require the exact new checked bounds
diagnostic, including requested size, stream offset, and remaining byte count.
No decoder behavior was changed to satisfy the tests.

The three retained GPU test setups now select Metal on Apple and Vulkan
elsewhere, and require the requested backend. Pixel/depth assertions and
thresholds are unchanged. The Z-scale fixture also needed the same real VI
mode and scale-one setup used by the other two fixtures: without it, the
Retina run returned a valid `512x384` display copy rather than the expected
`256x192`. `zscale-before-vi-mode.log` captures that initial diagnostic.

The Z-scale indexed draw originally passed a function-local vertex array to
GX and returned before the new asynchronous FIFO consumed it. The fixture now
retains the array in the frame's scope until readback completes. This preserves
the GX borrowed-array lifetime contract without per-draw synchronization or
sleeps. Its right-hand geometry then rendered correctly.

The remaining equal-depth pixel failure exposed a production omission during
the merge: `set_logical_viewport` dirtied the shared uniform only when viewport
left/top/width/height changed, while the retained GX depth-range transform also
depends on near/far. The Aurora owner added near/far to that predicate in
`lib/gx/gx.cpp`. Rebuilding and rerunning all three GPU programs passed the
unchanged depth ordering, clipping, tagged snapshot, and copy-filter assertions.

## Unchanged audio failure

`PcmAudioMixer.ScheduledGateReleasesALoopingSequenceLayer` fails its final
inactive-voice assertion after four output frames. Its sample values pass.
The mixer increments release elapsed time after generating a sample and marks
the layer finished when the next sample observes the completed release, so the
voice remains active at that exact buffer boundary. This code was not changed
as part of the merge. `audio-baseline-blobs.txt` confirms identical pre-merge
HEAD/current blob hashes for `lib/audio.cpp`, `include/aurora/audio.hpp`, and
`tests/audio_test.cpp`. This is a bounded source comparison, not a separately
rebuilt pre-merge runtime. No audio source or test expectation was modified.
