# Native startup without the Wii Remote strap screen

The user explicitly authorized omitting the starting Wii Remote health/safety
screen. Its game-owned implementation is `LogoScene`'s `WiiRemoteStrap` layout and
the StrapFadein/StrapDisplay/StrapFadeout nerves. The original display waits at
least 450 frames for input and otherwise advances at 1200 frames.

On `TARGET_PC`, the two incoming transitions to that flow now select the existing
`LogoSceneWaitReadDoneSystemArchive` nerve. The original region-specific China
ISBN flow still runs and then uses the same archive wait. The unused strap layout
is not loaded or constructed on PC. The ordinary LogoFader remains to cover
background loading.

All subsequent behavior remains in the original sequence:
WaitReadDoneSystemArchive -> MountGameData -> Deactive. This retains the actual
archive-completion check, save-data preload, and GameSequence completion callback
that chooses the next title/scene. It does not simulate controller input, publish
dummy process owners, force archive completion, or jump straight to gameplay.

The small platform branches are in `src/Game/Scene/LogoScene.cpp`, rather than a
host-side replacement sequence: its nerve instances have anonymous-namespace
ownership and the scene already owns its save/archive handoff. The explicit
user-authorized omission is the reason for this Game source difference.
Reference decomp and non-PC behavior are unchanged.

Validation: changed-source whitespace check and the coordinated native build
pass. `startup.lldb`, `startup.log` and `startup.json` record a bounded run against
the Korean disc with a private fresh NAND directory, no stage override and no
input injection. The actual Logo scene enters its archive-wait state with
`mStrapLayout == nullptr` and an ordinary LogoFader. The debugger then observes
the original save-data preload, Deactive, GameSequence end-scene and after-boot
callbacks. The original scene controller starts `Game` / `FileSelect`, and the
application reports 300 completed frames.

The three strap methods have no linked breakpoint locations in this build,
consistent with their now-unreachable native nerves being discarded. The null
layout and observed original continuation are the runtime evidence for omission.

After frame completion the debugger's catch-all exception breakpoint stops at
`OSExecution.cpp:387`, the internal `ThreadExit` raised for worker cancellation
during retirement. Its expected handler is `run_managed_thread` in the same file.
The debugger then kills the process; this particular check does not establish a
clean application exit. No startup/audio exception occurred before completion.
