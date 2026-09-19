# Local Dolphin pose-oracle availability

Read-only inspection found a usable local no-GUI executable at `dolphin/build-macos-upstream-20260907/Binaries/dolphin-emu-nogui`. Its `--version` reports Dolphin 2606-374; SHA256 is `a292a9ec7ce641ac439249815ea06e2c51aed5dfe5536c49e753cbd47b553faf`. Inspected Dolphin source HEAD is `d681903c67`. Only help/version commands were run; no game was booted.

The fork already provides general emulated-controller scripts (`Core/HW/WiimoteEmu/ScriptedInput.cpp`), unique presented-frame capture (`VideoCommon/FrameDumper.cpp`), GX trace windows (`VideoCommon/BPStructs.cpp`), and frame-selected save states (`VideoCommon/Present.cpp`). Their environment names are recorded in `dolphin-oracle.json`. Semantic anchor labels in the GX trace are operator-supplied labels; they do not constitute an existing Mario actor/state hook. A bounded source search found no Game/Mario semantic trace provider.

The existing GDB stub can read PowerPC registers/memory, continue or step, and place breakpoints. `Core/Config/MainSettings.cpp` exposes `Dolphin.General.GDBPort` and `GDBSocket`; `Core/Core.cpp` initializes the stub and pauses initial execution. The command below is a prospective invocation supported by the inspected CLI and source, not an executed retail comparison:

```sh
./dolphin/build-macos-upstream-20260907/Binaries/dolphin-emu-nogui   -p macos -v Metal -u /tmp/petari-retail-pose-user   -C Dolphin.General.GDBPort=55020   -e '/Users/frityet/Projects/petari/Super Mario Wii - Galaxy Adventure (Korea).rvz'
```

A client must connect and continue execution. After independently validating the loaded retail DOL/region, original method breakpoints and register r3 provide object identity for read-only memory snapshots. Native 64-bit object offsets must not be used for retail 32-bit objects. The old Linux shared-memory path is unsuitable as an assumed macOS interface: current Darwin memory allocation uses Mach memory entries.

No currently usable Gateway wakeup checkpoint was established. The prior `notes/dolphin-oracle-20260806T201750Z/seed-dolphin-user` directory exists, but the specifically documented `/tmp/dolphin-gateway-safe-f11400/frame-11400.sav` is absent. The inspected older route ends at Castle-side guiding Toad around frame 22150 and explicitly does not claim Gateway. Current save-state format is 192; historical format 190 states are rejected. An ordinary route to Gateway and a new compatible checkpoint are therefore still needed before an economical repeatable wakeup/curved-surface pose comparison.

Useful prior evidence, not fresh runtime claims: `notes/dolphin-upstream-20260907/README.md`, `notes/dolphin-oracle-20260806T201750Z/README.md`, `notes/dolphin-nunchuk-oracle-20260806T214347Z/README.md`, and `notes/dolphin-gateway-jpa-oracle-20260806T231821Z/README.md`.
