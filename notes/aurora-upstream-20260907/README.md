# Aurora upstream synchronization — 2026-09-07

The active Aurora submodule was clean at
`8c90b037b877278b558a2ea7984525c564daa140` on `codex/macos-compat`.
The configured fork is `https://github.com/frityet/aurora`; the canonical
upstream is `https://github.com/encounter/aurora.git`, default branch `main`.
`git fetch --all --prune` confirmed that both `origin/main` and
`origin/codex/macos-compat` are already ancestors of the starting branch.
Other remote feature branches are separate work, not upstream integration tips.

Canonical upstream advanced from `f1189541e5d8b97fdf61946377853488d504d9df`
to `749d6ee7a22bdfab78c8ece9047bca5d79aa72ca` with three commits:

- `3251f4e23e3bec49d0e8a09f3c532e443fa29865`: THP video/audio decoder and
  a shared byte reader extracted from the GX command processor.
- `b6b0b34c4a4f3e15cec39f97b9ae7e397bd5d8cf`: Windows CARD path fix.
- `749d6ee7a22bdfab78c8ece9047bca5d79aa72ca`: RmlUi runtime-texture mipmaps.

An ordinary `git merge --no-ff --no-commit upstream/main` produced two
conflicts. The CMake resolution retains the fork's base/platform/core
dependency structure and enables upstream's THP target after core. The GX
resolution retains the existing checked-array-span and unsized-retail-array
handling while adopting `ByteReader` throughout. Source comparison confirms
that the command processor differs from the starting commit only by deleting
its old Reader implementation and replacing its references with ByteReader.
Existing Gekko matrix/math and functional adapter sources remain unchanged;
see `source-preservation.json`.

Aurora's xmake integration now exposes `aurora_enable_thp` (default true) and
the real `aurora-thp` library, matching upstream CMake. Two retained GX FIFO
death tests use the shared reader's new `Reader overrun` diagnostic while
preserving their byte-count and offset assertions. No Game sources or root
index entries are part of this Aurora change.

Aurora has no nested Git submodules. `git submodule update --init --recursive`
completed successfully. Upstream did not change its pinned dependency versions.

## Validation

All 255 existing GX FIFO/GD tests pass after a fresh standalone CMake build
using Homebrew Clang 23.1.0, including unsized arrays, physical GD pointers,
texture replay, retained depth scale, and malformed-command aborts. The test
runner, build commands, logs, and GoogleTest XML are saved alongside this file.
The separate build directory is in the system temporary folder, leaving the
root xmake build available for coordinated integration work. Existing cached
dependency sources were used without changing their revisions.

Both new THP translation units compile in CMake. `thp-smoke.cpp` passes against
those objects: an independently constructed zero-coefficient 16x16 4:2:0 MCU
decodes to neutral 128 Y/U/V planes; synthetic stereo ADPCM preserves distinct
channel samples in both planar/interleaved output, mono duplicates correctly,
and invalid-input/output return checks pass. This bounded smoke does not prove
full movie compatibility or Dolphin-identical decoding for arbitrary content.

The parent task additionally reports successful native root xmake builds of
`smg-pc-game` and `aurora-thp` using Homebrew LLVM 23; logs are in
`../upstream-sync-20260907/game-build.log` and
`../upstream-sync-20260907/aurora-thp-build.log`. Windows CARD behavior and
optional RmlUi rendering were not executed on this macOS validation pass.
`git diff --cached --check` passes. These checks do not establish that the full
Gateway bunny chase works.

## Completed merge

Merge commit `b94a330df2b0a270ea3872ee5bc9f5147f354ba0` was pushed to
`origin/codex/macos-compat`. Its parents are the starting fork commit and
canonical upstream `749d6ee7a22bdfab78c8ece9047bca5d79aa72ca`. An independent
`git ls-remote` confirms that the remote branch equals the new HEAD,
`git merge-base --is-ancestor upstream/main HEAD` succeeds, and Aurora
is clean. See `merge-result.json`. The root task records the submodule pointer.
