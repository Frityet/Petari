# GX array upload snapshots — 2026-09-10

## Problem and change

The live demo reached original Mario animation and CameraDirector execution, then exhausted the frame storage buffer while uploading a 164,622-byte native normal array. `GXSetArrayBase` carried no explicit extent. Separately from growth of that proven extent, rebinding or `GXInvalidateVtxCache` discarded the current attribute upload even when its source bytes had not changed.

Aurora now retains one owned CPU snapshot and immutable storage range per source pointer for the current frame. A later request reuses that range only when the requested prefix fits and every requested byte matches. Different contents receive a new upload. Shorter prefix reuse does not read the unrequested tail; a later expanded request checks the tail before reuse. An existing valid attribute `cachedRange` keeps its original fast path, including its ability to outlive the original source bytes until invalidation. No pointer-only or hash-only equality is used.

The cache is cleared at the existing end-of-frame draw-cache boundary and after the FIFO worker stops during shutdown. Snapshot allocations use the host allocation scope. It stores only the latest snapshot for each source, so historical mutations and growing prefixes do not accumulate CPU copies. There is no capacity increase and no mutation of previously published GPU ranges.

This change alone does not eliminate uploads for genuinely growing unsized prefixes. The separately owned native array registration change supplies authoritative resource extents at replay time; it is recorded and tested in the sibling registry cohort.

## Evidence

- `baseline-tests.log`: both new behavioral regressions fail before the cache. Unchanged invalidation uploads twice instead of once; A→B→A uploads three times instead of twice.
- `cache-build.log`: independent Aurora CMake `gx_fifo_tests` build succeeds after the cache change.
- `cache-tests.log` and `cache-tests.xml`: all 261 tests pass, zero failures and zero skips. These include three new snapshot regressions, existing source-retirement behavior, indexed-array bounds, and the existing FIFO/GD suite.
- The test storage provider copies uploaded bytes and returns distinct offsets. Assertions verify upload counts, reused versus replaced ranges, exact changed contents, smaller-prefix behavior, changed tails on later expansion, and new uploads after frame expiry.

The focused executable uses a storage recorder stub and is not a WebGPU/live-demo proof. Parent task owns the combined build and live runtime check. The combined cache and registry result is now recorded in `combined-build.log`, `combined-tests.log`, and `combined-tests.xml`: build and runtime both exit 0, all 269 tests pass with no skips. The eight additional registry tests prove owner retirement, exact/interior bounds, conflict rejection, replay-time GD pointer patch resolution, explicit cap/endianness preservation, and one upload for 16 growing registered indices versus 16 uploads for an unregistered source.

## Owned files

- `aurora/lib/gx/command_processor.cpp`: snapshot helper, draw upload route, frame retirement. The sibling agent separately owns the registry include and `GX_AURORA_LOAD_ARRAYBASE` resolution hunk in this same file.
- `aurora/lib/gx/fifo.cpp`: clear snapshots after worker shutdown.
- `aurora/tests/gx_fifo_test.cpp`: three behavior regressions.
- `aurora/tests/gx_test_common.hpp`: explicit draw-cache reset with isolated test state.
- `aurora/tests/gx_test_stubs.cpp`: observable upload count and distinct range offsets.
