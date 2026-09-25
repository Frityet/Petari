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
