# Shareable tester builds — 2026-09-25

Requested ARM64 macOS and x86_64 Linux builds, with static application dependencies. No disc assets or saves are packaged. Build from the current original-game implementation, including the overlapping GX drain fix.

The wider full-game linkage audit is pending while these packages are prepared. Preserve user control of gameplay. Native system libraries and GPU drivers remain supplied by the OS.

## Mac package

`dist/petari-macos-arm64-c97beea7.zip` is 10,910,963 bytes. `macos-package.json` records its SHA256, executable hash, architecture, minimum macOS and the complete dynamic dependency list. `otool -L` contains only Apple system frameworks and libraries. C++, libpng, zlib, SDL, Dawn, nod and other application dependencies are static. The app is ad-hoc signed, not notarized. The launcher selects the tester's disc and records a per-run log. It does not inject a stage, controller input, saves or assets.

The packaged executable was checked through the launcher's native disc-open error path, including a relative `--disc` argument. This is a loader/package check, not new gameplay validation. The latest runtime fix remains the general overlapping FIFO drain correction.

## Rebuild commands

Mac uses an isolated `XMAKE_CONFIGDIR=build/tester-config-macos`, `xmake f -y --static_deps=y --runtimes=c++_static --ar=/opt/homebrew/opt/llvm/bin/llvm-ar --ranlib=/opt/homebrew/opt/llvm/bin/llvm-ranlib`, then `xmake build -y -j16 smg-pc`. Normal checkout configuration is preserved.

Linux uses a separate Ubuntu 24.04 x86_64 filesystem under Apple Container/Rosetta, with LLVM 23 and xmake 3.1.1. It builds from a 32,870,400-byte source-only snapshot, without decomp, Dolphin, discs, NAND or historical notes. Use `xmake f -y -m debug --toolchain=llvm --runtimes=stdc++_static --static_deps=y --ar=/usr/bin/llvm-ar-23 --ranlib=/usr/bin/llvm-ranlib-23`. Dawn's prebuilt Linux archive exposes GNU C++ ABI symbols, so the project now defaults Linux to GNU C++ while retaining libc++ on macOS.

The `static_deps` option prevents automatic substitution of system package libraries. The first Mac attempt exposed a Homebrew libpng dependency; an explicit LLVM archiver was needed to build the static local package. Linux initially needed explicit container DNS and an llvm-config symlink for xmake toolchain discovery.

## Linux compilation boundary

The first game build reached `OriginalGameApplication.cpp` and Clang rejected the MSL `vsnprintf` assembler label because a GNU C++ header had already used that function. Moving the existing MSL header to the first include establishes the intended formatting boundary before any Game or standard-library header. This only changes include order in the PC application; no Game behavior or Game source was changed. `linux-build.log` records the first failure; `linux-build-retry.log` records the continued build.

Aurora's ARAM provider also needed its direct `<cstdint>` include for `std::uintptr_t`. A later error in the original `Meramera.cpp` came from GNU `<math.h>` importing C++20 `std::lerp` into the global namespace. The general fix is in `aurora/include/dolphin/ppc_math.h`: use `<cmath>` from C++ and retain `<math.h>` from C. The original Game helper and its arithmetic remain unchanged. A focused compilation of the exact failed Meramera translation unit passed with the corrected boundary (`linux-meramera-compile.log`, empty on success).

Linux's case-sensitive filesystem revealed three pre-existing case mismatches, also present in the donor: `KariKariDirector.hpp` in `Enemy.hpp`/`MarioSlider.cpp`, and `Xanimecore.hpp` in `ActorMovementUtil.cpp`. The port includes now exactly match `KarikariDirector.hpp` and `XanimeCore.hpp`. A single include-path audit found no additional relative-path case mismatches. These filename corrections only change include spellings; no function bodies or game behavior changed. `include-case-audit.json` lists the original mismatches.

## ELF linkage

The completed compiler pass exposed duplicate ELF definitions. The older Nerve macros already used native inline instance storage, but the newer `NEW_NERVE` and `NEW_NERVE_ONEND` macros still emitted a strong global definition for every header inclusion. They now use the same `NERVE_NATIVE_INSTANCE` / `INIT_NERVE` policy as the other macros, preserving one native instance per state and retaining the original Wii definition path.

Twelve unused matching-only helper definitions also reused six external names (`dummy`, `FORCE_OPERATOR`, `FORCE_MATCH_SDATA2`, and three copied actor-prefixed names). These no-callsite helpers now have file-local `static` linkage; their bodies are unchanged. `local-match-helpers.json` lists the files. The build retains strict duplicate-symbol diagnostics.
