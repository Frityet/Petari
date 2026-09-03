# macOS Apple Silicon build and runtime validation

Request: compile the existing PC port on an M5 Max using Homebrew LLVM 23,
initialize all submodules, and run against the supplied Korean RVZ.

## Initial state

- Parent branch: `pcp-aurora`.
- Homebrew LLVM: 23.1.0, native arm64; macOS 26.6.2.
- Aurora initially had an empty checkout/index at `4a21d57`; its apparent
  staged deletions were saved to `aurora-initial-index.patch` before restoring
  the incomplete checkout. Recursive submodule initialization then checked out
  the parent-pinned `87849d17b106ba6df9d5389c03cd8427710766c4`.
- Dolphin and its recursive dependencies were initialized at the pinned
  `ed8e44d4be114fc70258fbfaeb239f3e83b041fe`.
- The supplied `.rvz` remains in the repository root and is read directly by nod.

## Scope from previous notes

The current runnable showcase includes title, file select, and a bounded Gateway
walking scene. See `../gateway-rabbit-route-20260809T151500Z/README.md` and
`../gateway-spin-unlock-20260809T090149Z/README.md`. Those Linux results do not
establish macOS correctness, and this work does not turn the incomplete port
into a complete game.

## Work in progress

- Replace GNU-only link flags with platform-appropriate flags and inherit the
  selected LLVM toolchain in showcase/test targets.
- Link PNG debug tools through the existing zlib package.
- Include Aurora's existing `system_info_mac.mm` and Foundation dependency in
  its Xmake build, matching its CMake build.
- Use LLVM's matching libc++ headers, library, and rpath.
- Initial configure log captures fmt 11.1.4's missing `<cstdlib>` include under
  libc++ 23; package resolution is being repaired before compiling the port.

## Dependency and compatibility findings

- fmt 11.1.4 now builds using a pinned local recipe with a checksummed patch
  adding the required `<cstdlib>` header.
- LLVM 23 removed `std::mem_fun` and `std::bind2nd`. Aurora's MSL adapter now
  uses `std::invoke` and retains bound arguments by reference. The standalone
  regression and unchanged `AreaObj.cpp` compile probe pass with LLVM 23.
- SDL3 uses native macOS Metal/CGL headers, so the package no longer fetches
  unrelated EGL/OpenGL registries on Apple; macOS OpenGLES is disabled.
- Xmake 3.1.1's LLVM toolchain imports Xcode settings, selecting the nonexistent
  `LLVM/bin/ar`. The parent build now supplies `--ar=LLVM/bin/llvm-ar`. Nested
  package builds also need this option explicitly forwarded.

## Native build and runtime evidence

- `build-showcase-10.log`: native arm64 showcase linked successfully.
- `build-app-tests.log`: main application and both focused runtime tests built.
- All 586 C++/Objective-C++ entries in the generated compilation database use
  `/opt/homebrew/opt/llvm/bin/clang++`. The executable links the matching
  Homebrew libc++ dylib. `file` identifies the result as arm64 Mach-O.
- The unchanged retail source also requires `-Wno-register`; the gravity manager
  uses Clang's `-fms-extensions` for its existing explicit pointer-narrowing cast,
  matching the prior GCC `-fpermissive` build contract.
- Added direct `<string>` and `<exception>` includes exposed by libc++ 23's
  reduced transitive includes. No reconstructed Game C++ source was edited.
- Dawn's package now declares its required IOSurface framework. Existing cached
  package metadata was refreshed using the pinned Dawn version.
- `title-smoke.log`: Metal selected Apple M5 Max, read the RVZ directly, and
  passed the title GPU packet proof after 3 rendered frames.
- `gateway-smoke.log`: passed after 28 rendered frames, including real Mario
  packets, GPU submission, gravity acceleration, and real KCL contact.
- `title-route-test.log`: exact title-to-file-select contract passed after 332
  title frames; slot navigation, held/fresh input, scene recreation, and teardown
  passed.
- `title.png` and `gateway.png`: actual renderer captures visually inspected.
- `main-app-smoke.log`: the strict main application links and initializes Metal,
  but its existing placement frontier rejects `FileSelector`. The supported
  runnable entrypoint is `smg-pc-showcase`.
- `mario-walk-diagnostic.log`: input, speed, grounding, and drawing passed, but
  Run animation stayed on frame zero. Investigation identified a pre-existing
  missing synchronization after derived actors' CalcAnim override. The generic
  scheduler/compatibility fix publishes controller state after virtual CalcAnim
  and refreshes retained joint matrices, without advancing time twice.
- `mario-walk-final.log`: PASS, 325.685 units of grounded walking, advancing
  Wait -> Run -> Wait animations, release to idle, and player recreation.
- `live-actor-animation-final.log`: PASS 6/6, including custom CalcAnim override
  publication, retained joint pointers, no-calc freeze, and stopped animation.
- `title-route-final.log`: PASS again after the animation change.

Per the user's clarified scope, the launcher is
`pc-port/script/build_and_run.sh`. The initially created root launcher and Codex
configuration were removed; every source/configuration edit is within `pc-port`.

## Final live window check

The launcher stages `pc-port/build/smg-pc-showcase.app` with the native identifier
`org.petari.smg-pc.showcase`, preserving the same binary, arguments, and disc
selection. Computer Use successfully inspected its titled Gateway window and
sent arrow-key input. Actual window captures are `live-window-before.jpg` and
`live-window-after.jpg`; the Gateway window was left open for the user.

Launch from the repository root:

```sh
./pc-port/script/build_and_run.sh          # title -> file select
./pc-port/script/build_and_run.sh gateway # directly into Gateway
```

Final limits: the showcase is a bounded working port slice, not full gameplay.
The strict main application still rejects the unimplemented FileSelector
placement. The tested build is debug/arm64 using LLVM 23.1.0 on macOS 26.6.2;
other hardware, release mode, and complete game progression were not tested.
