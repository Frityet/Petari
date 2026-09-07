# Running on Apple Silicon macOS

The launcher targets Metal on Apple Silicon using Homebrew LLVM 23.
It reads RVZ files directly; no extraction or ISO conversion is needed.

Current clean builds are blocked by existing game-source issues: the showcase
now reaches incomplete effect APIs in the original `MarioEffect.cpp`, while the full `smg-pc`
target fails to link missing game methods. The pointer-width blocker is corrected; the latest dependencies, game library,
and eight focused tests (including Metal GX copy pixels) pass. A new application
run has not yet been completed. The title/File Select
and Gateway runtime results described below are historical results from before
this cleanup, not a claim that the current clean build succeeds.

Install Xcode or its command line tools and these Homebrew dependencies, then
run the following commands **from the repository root**:

```sh
brew install llvm@23 xmake cmake ninja
git submodule update --init --recursive
./script/build_and_run.sh
```

The launcher selects Homebrew LLVM 23, builds an arm64 debug executable using
LLVM's shared libc++, and starts `smg-pc-showcase` at the title screen. It finds
exactly one root `.rvz` automatically. To select another image, pass
`--disc "/path/to/game.rvz"` or set `SMGPC_DISC_IMAGE`.

```sh
# Start directly in Gateway.
./script/build_and_run.sh gateway

# Run bounded rendering/resource/physics checks and exit.
./script/build_and_run.sh title --verify
./script/build_and_run.sh gateway --verify

# Build without launching, or launch under LLDB.
./script/build_and_run.sh --build-only
./script/build_and_run.sh gateway --debug
```

At the title prompt, hold **Enter + Backspace**.
Use the arrow keys and a fresh Enter press to select a blank file. In game,
**WASD** drives the Nunchuk stick; the **arrow keys** drive the Wii Remote
D-pad used by the original camera controls. **C** resets the camera when the
authored camera permits it. **F9** toggles the development camera and **Esc**
quits. Movement and animation remain part of the WIP implementation.

`--verify` stops when the existing smoke assertions pass, with a default limit
of 360 frames; it may finish much earlier. Title and Gateway smoke checks
passed on the M5 Max, as did the title/File Select route regression test.
The real-disc Mario movement test also passes: standing, 325.685 units of
grounded walking, advancing Wait/Run animations, release to idle, and player
recreation. The generic animation override regression passes as well.

Earlier builds reached bounded title and Gateway showcase routes; the full
target stopped at an unsupported `FileSelector` placement. Full gameplay and
progression are not implemented or validated.
To compile the full target without launching it:

```sh
./script/build_and_run.sh --strict --build-only
```

Build output is under `build/macosx/arm64/debug`. Add `--logs` to save
output to `build/macos-run.log`, or use `--help` for other options.
Screenshots and other relative runtime output paths resolve inside the repository root.
The launcher also stages `build/smg-pc-showcase.app` to give SDL a native
macOS application identity. Launch through the script so the disc path and
route are supplied. The bundle uses the installed Homebrew LLVM runtime.
