# Build portability audit

The existing local Dawn and nod package recipes already provide arm64 macOS
artifacts at the versions selected by Aurora. SDL3's local recipe also declares
the macOS frameworks needed for static linkage.

The showcase and focused test targets previously replaced the configured C++
compiler with an unqualified `clang++`. They now inherit the selected toolchain,
which allows the Homebrew LLVM 23 configuration to apply consistently. Their
unused-section linker setting selects Darwin `-dead_strip` on Apple platforms
and retains GNU `--gc-sections` elsewhere.

The debug PNG utilities previously linked the Linux-only `libz.so.1` name and
added the unconfigured zlib-ng source include directory. They now declare and use
the existing xmake `zlib` dependency, which supplies its matching headers and
library on each platform.

Inspection found `/opt/homebrew/opt/llvm` resolves to LLVM 23.1.0. A compiler
`-###` dry run showed it uses LLVM 23 libc++ headers but defaults to the SDK
`-lc++` search path. xmake 3.1.1's `c++_shared` runtime handling adds the selected
LLVM libc++ include, library, and runtime search paths when using its LLVM
toolchain; the main build configuration should select this runtime if it uses
Homebrew libc++.

Validation for these edits: `git diff --check` passed. Actual configure, compile,
link, and runtime checks are performed by the main build task and recorded
separately. No game source was changed by this audit.

The first dependency build subsequently exposed a real fmt 11.1.4 compatibility
error: `include/fmt/format.h` uses `malloc` and `free` in its allocator without
including `<cstdlib>`. LLVM 23's libc++ no longer provides those declarations
through the other headers. The local `packages/f/fmt` recipe keeps the existing
version and its source checksum, mirrors the upstream xmake installation
settings, and applies a checksummed one-line include patch before installation.
The patch passed `patch --dry-run` against the exact downloaded fmt source. This
also repairs the installed header for dependent translation units instead of
only forcing an include during fmt's own compilation.

The SDL3 recipe now avoids downloading external EGL/OpenGL header registries on
Apple targets and WebAssembly. On macOS it disables SDL's optional GLES backend,
which otherwise selects an external EGL implementation; native Metal and CGL
remain available. iOS retains the SDK OpenGLES implementation. The dependency
include list starts as an empty table, fixing iteration when no external header
packages are needed. This follows the Apple branch in the SDL CMake source
present in Dolphin's initialized SDL submodule.

The libpng build reached its archive step and failed because xmake searched for
`/opt/homebrew/opt/llvm/bin/ar`, while Homebrew provides `llvm-ar`. This is a
toolchain selection issue rather than a libpng ARM assembly source issue.
Inspection traced it to xmake 3.1.1's LLVM toolchain loading the Xcode tool setup,
which overwrites `llvm-ar` with `<bindir>/ar`. Its host-toolchain package helper
then omits the parent's explicit archiver override when configuring child xmake
builds, so passing `--ar` only on the parent did not fix libpng.

The local libpng recipe preserves the existing v1.6.58 source/checksum and
upstream xmake target rules, including ARM NEON, but passes the configured
archiver into the child explicitly. The local ImGui recipe now forwards the same
setting so its child build uses the selected tool too. These package changes
avoid modifying either Homebrew's installed tools or xmake's global files.
