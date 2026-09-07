# Dolphin upstream merge — 2026-09-07

The Dolphin debugger/reference fork was clean and detached at
`ed8e44d4be114fc70258fbfaeb239f3e83b041fe` before work. The exact old hash is
recorded in `head-before.txt`; the existing local `master` and `origin/master`
both identified that commit. Work reattached the existing `master` branch.

Remotes:

- `origin`: `https://github.com/Frityet/dolphin`
- `upstream`: `https://github.com/dolphin-emu/dolphin.git` (added locally)

Fetched both remotes with pruning. Origin was already current. Canonical
`upstream/master` was `a2efdf1197be8132674b90fe9cf4761df39752ed`, dated
2026-09-06, with 429 incoming commits since the fork base and 8 fork-only
commits. `git merge --no-ff --no-commit upstream/master` merged without
conflicts. The existing parity trace, deterministic Wiimote/Nunchuk input,
presented-frame savestate hooks, shared-memory option, and portability changes
remain in the resulting tree. `fork-delta-after.txt` records the same 19 paths
that differ from canonical upstream. `preservation.json` verifies that 14 of
those files are byte-identical to the original fork, while the other five
received conflict-free changes from both histories.

Created merge `4308a07516056f4412eec64fa13365ccae524a92` and pushed it to
`origin/master`. Both the original fork head and incoming canonical head are
verified ancestors. Follow-up `d681903c67` adds the explicit `<vector>` include
found during validation and is also pushed to `origin/master`; `head-final.txt`
records its full hash. The branch is clean and matches its remote.

Ran recursive submodule sync/init/update to the commits recorded by the merged
Dolphin tree. Three direct pins changed: Qt, SDL, and cpp-ipc. Their exact before
and after hashes, plus every nested pin, are in `submodules-before.txt` and
`submodules-after.txt`. No third-party dependency was advanced beyond Dolphin's
recorded gitlinks.

## Behavior relevant to retained oracle artifacts

Upstream changes the savestate format version from 190 to 192. `State.cpp`
explicitly rejects a different format version. Historical version-190 `.sav`
files require recapture with the updated reference binary. The optional
`SMGPC_DOLPHIN_SAVE_STATE_FRAME` / `SMGPC_DOLPHIN_SAVE_STATE_PATH` hooks still
call the unchanged `State::SaveAs(Core::System&, std::string)` API.

Upstream renamed the no-GUI CMake target to `dolphin-nogui`; its executable is
still named `dolphin-emu-nogui`.

## Validation

- Merge was conflict-free; no unresolved index paths.
- All 8 original fork commits are retained through merge ancestry.
- Recursive submodule state is checked against recorded gitlinks.
- Incoming upstream whitespace warnings are recorded in
  `upstream-whitespace.txt` without rewriting upstream content.
- Fresh native arm64 CMake configure succeeded with Homebrew LLVM 23.1.0,
  Release mode, no Qt UI, Metal/OpenGL support, no Vulkan or LLVM disassembler,
  and tests enabled. See `configure.log`.
- The first build exposed a local toolchain setup issue: mbedTLS invokes Apple
  ranlib flags rejected by LLVM ranlib. Fresh configure with `CMAKE_AR=/usr/bin/ar`
  and `CMAKE_RANLIB=/usr/bin/ranlib` fixed it without changing sources. Changing
  these after compiler detection did not override cached compiler metadata, so
  `cmake --fresh` was required. See `configure-native-archive-fresh.log`.
- Native no-GUI and complete test-binary compilation both succeeded; see
  `build-macos26.log` and the post-commit metadata refresh in `build-final.log`.
- All 26 focused existing tests passed across `ScriptedInput.*` and
  `StringUtil.*`; see `focused-tests.log`. The entire test binary was built,
  but the full upstream test suite was not run.
- `dolphin-emu-nogui --version` exits successfully. `version-final.log` records
  the clean post-commit binary's revision string.

Build validation also found `CustomTextureData.h` using `std::vector` without
including `<vector>`. Added that missing include as a general header correctness
fix. The fork's existing floating-point `std::from_chars` parser requires macOS
26 with this libc++ configuration; Dolphin defaults to deployment target 11.
The validation build explicitly targets macOS 26.0 on the current macOS 26.6.2
host, preserving the existing parser source. This build is not validated for
older macOS releases.

Reproduce the build with `sh notes/dolphin-upstream-20260907/build.sh`. The local
verification executable is
`dolphin/build-macos-upstream-20260907/Binaries/dolphin-emu-nogui`. Existing oracle
scripts may select it with `SMGPC_DOLPHIN_BIN`; their historical default build
directory was not changed.

This merge does not claim a fresh gameplay/oracle capture or compatibility of
historical savestate files.
