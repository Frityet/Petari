# Repository flattening completed — 2026-09-07

The repository root is the Xmake PC port. `decomp/` is a separate submodule from
Frityet/Petari tracking `pcp-decomp`; Aurora and Dolphin live at the root.
The earlier checkpoint is described in `checkpoint.md`.

## Completed changes

- Flattened source, package, script, test, documentation, container, and dependency paths.
- Updated source comparison tests and the audit to read `decomp/`.
- Copied eight required fallback headers from the original embedded decomp tree
  into the port. The native project has no compile dependency on `decomp/`.
- Preserved all 732 existing Game source/header/build-definition files and the
  app build definition byte-for-byte against the initial working-tree hashes.
  This includes the preexisting uncommitted PC changes, committed in the checkpoint.
- Consolidated six debug probes on shared root and fixture discovery, including
  partial layout-only extractions, and added a regression test.
- Removed old parent-directory fixture paths; the restart test accepts
  `SMGPC_REAL_DISC` and otherwise uses a repository-root image.
- Replaced decompilation CI with native PC builds using the root Dockerfile.
- Updated VS Code build and compile-database settings and added a root Run action.
- Updated README/MACOS documentation with the root build commands, decomp workflow,
  and the verified limitations of current clean application builds.

## Validation

macOS arm64, Homebrew LLVM 23, Xmake 3.1.1:

- Recursive submodule sync/init succeeded.
- `xmake build -y smg-pc-game` succeeded from the repository root.
- Debug-path, text-encoding, fixed-step-clock, and scene-scheduler-heap tests built and passed.
- All six updated debug probes passed compilation checks with their generated build flags.
- Four changed fixture-test translation units compiled. The ActorShadowCsv test
  has existing API-signature errors on lines unrelated to its fixture path change.
- `xmake source-closeness-audit --output=notes/repo-flatten-20260907/source-audit`
  succeeded: 737 original Game files and 181 compatibility files audited.
- Generated compile commands contain no old `/petari/pc-port/` paths and no
  `/petari/decomp/` include or source dependencies.
- Shell syntax, editor JSON, and Run-action TOML validated.
- GitHub workflow passed actionlint 1.7.7. Linux/container execution was not run.
- No `pc-port/` directory or tracked original decomp build tree remains at the root.

## Existing application/source limitations

- The showcase clean build stops at two pointer-to-u32 casts in unchanged
  `src/Game/Player/MarioTeresa.cpp`. Both errors were reproduced using the
  original pre-flattening fallback include paths; see `original-include-baseline.log`.
- The main application and upstream-component test compile but fail linking
  missing Game methods. No Game implementation or production target definition
  was changed to hide or work around these failures.
- Both source-mirror tests already failed before flattening. Against the selected
  decomp branch the Game check reports 19 differences; the Player check reports
  77 failures, including 24 missing source counterparts in that branch.
  The source reference is deliberately the requested branch, not a replacement
  with the previous embedded snapshot.
- No newly built application runtime success or complete gameplay is claimed.

Detailed local logs and machine-readable results are alongside this report.
The pre-flattening source snapshot, initial patch, and old port build caches are
preserved at `/Users/frityet/Projects/petari-cleanup-backup-20260907T091745`.
The old Wii build artifacts remain in `decomp/build/`; reconfigure inside
`decomp/` before attempting a decompilation build.

The decomp submodule commit is local. Publish it before publishing a parent
commit that references it. No remote branches were pushed by this cleanup.
