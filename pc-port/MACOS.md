# Running on Apple Silicon macOS

The title/File Select and Gateway showcases run with Metal on the M5 Max.
This was verified with macOS 26.6.2, Homebrew LLVM 23.1.0, and the Korean
`Super Mario Wii - Galaxy Adventure (Korea).rvz` in the repository root.
The port reads the RVZ directly; no extraction or ISO conversion is needed.

Install Xcode or its command line tools and these Homebrew dependencies, then
run the following commands **from the repository root**:

```sh
brew install llvm@23 xmake cmake ninja
git submodule update --init --recursive
./pc-port/script/build_and_run.sh
```

The launcher selects Homebrew LLVM 23, builds an arm64 debug executable using
LLVM's shared libc++, and starts `smg-pc-showcase` at the title screen. It finds
exactly one root `.rvz` automatically. To select another image, pass
`--disc "/path/to/game.rvz"` or set `SMGPC_DISC_IMAGE`.

```sh
# Start directly in Gateway.
./pc-port/script/build_and_run.sh gateway

# Run bounded rendering/resource/physics checks and exit.
./pc-port/script/build_and_run.sh title --verify
./pc-port/script/build_and_run.sh gateway --verify

# Build without launching, or launch under LLDB.
./pc-port/script/build_and_run.sh --build-only
./pc-port/script/build_and_run.sh gateway --debug
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

Both `smg-pc-showcase` and the full `smg-pc` target compile. The full target's
runtime currently stops at an unsupported `FileSelector` placement, so use
the showcase launcher for the working routes. These are limited portions of
the game; full gameplay and progression are not implemented or validated.
To compile the full target without launching it:

```sh
./pc-port/script/build_and_run.sh --strict --build-only
```

Build output is under `pc-port/build/macosx/arm64/debug`. Add `--logs` to save
output to `pc-port/build/macos-run.log`, or use `--help` for other options.
Screenshots and other relative runtime output paths resolve inside `pc-port`.
The launcher also stages `pc-port/build/smg-pc-showcase.app` to give SDL a native
macOS application identity. Launch through the script so the disc path and
route are supplied. The bundle uses the installed Homebrew LLVM runtime.
