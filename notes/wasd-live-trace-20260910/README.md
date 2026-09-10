# Read-only live control trace

The existing debug-only `SMGPC_DEBUG_SIMULATION_TIMING` branch in `src/showcase/Showcase.cpp` now emits a distinct `[smgpc:control]` line after each presentation. It records the simulation tick and presentation number, focus and free-camera state, window W/A/S/D key mappings, raw MR player stick values, the original `MarioActor::getStickValue` outputs, Mario's consumed world stick vector, movement lock bit `_22`, jumping, `isInputDisable`, draw lock bit `_7`, actor `_37C` and `_3C0`, `isEnableNerveChange`, and both Mario/internal and actor velocities.

`MarioActor::getStickValue` in `src/Game/Player/MarioActorPad.cpp` only writes its two supplied output floats and queries input/animation gates. Both output addresses are valid. `MarioModule::isInputDisable` checks movement/status/animation fields and actor `_3C0`; `isEnableNerveChange` queries Wait/NoRush nerves. None of these diagnostic calls advances animation, reads a consuming button trigger, changes a nerve, or changes input. Wii bitfield accessor values are explicitly converted to bool before C varargs.

This is instrumentation only; no Game source, runtime input generation, movement, camera, or timing behavior is modified. The trace is compiled out with NDEBUG and requires the existing environment flag when compiled in. The root agent owns build/run validation; no build was started by this task. Source diff whitespace validation passed. The logged stick getter is an after-movement observation of the current gates; `mStickPos` and velocities provide the actual state left by that tick.


## SDL key replay integration driver

`SMGPC_DEBUG_SDL_KEY_SCRIPT='180-240:w;300-305:space;400-420:a'` now feeds synthetic SDL key events at reached Gateway simulation ticks. Ranges are inclusive: W goes down at tick180 and up at241. Each SDL key is held while any of its ranges includes the current tick, so overlapping ranges do not create duplicate press/release transitions. SDL key names and their current-layout scancodes must resolve; malformed, reversed, overflowed, or empty range entries fail explicitly. Absent or empty environment configuration has no effect.

`src/app/DebugSdlKeyReplay.hpp` queues only SDL key down/up transitions with the actual window ID, keycode, scancode, modifiers, timestamp, and non-repeat state. The Gateway loop then calls its ordinary `window.poll_events()` before `runtime.begin_frame`, including during multiple simulation ticks per presentation. No Wii stick/button state, actor state, or SDL keyboard-state array is directly overwritten. Native script/event allocations use the host allocation scope. Helper and Gateway hook are entirely excluded with NDEBUG.

The driver logs `[smgpc:sdl-replay] ... synthetic=1` for every transition. This validates the SDL event-to-window-to-Wii compatibility path; it is not proof of physical keyboard delivery. Existing `[smgpc:control] wasd=...` reports the window's mapped key state and can therefore reflect either hardware or these synthetic events. The control trace additionally distinguishes Mario's actual `mMovementStates._1` ground state from Binder's existing ground label, and records `mWalkSpeed` and `mVerticalSpeed`.

Source/SDK inspection confirms the event fields and SDL APIs match the installed SDL3. No build or runtime was started by this task, as the parent owns the build lane. Final source hashes are in `source-manifest.json`.

## Parent integration evidence

The parent built the original Xanime lifecycle fixture and showcase with the existing optimized-debug LLVM 23 configuration. The fixture first failed on the old automatic-return predicate and then passed all six groups on the recovered predicate. Its default transition exposed wrong original animation strings, recovered separately from retail addresses. None of the following intermediate binaries replaced the user's packaged demo.

| Native binary | Actual result |
| --- | --- |
| `93f82c340f122e3ef967a486684138bfc654939e945e7aaf411b76d54f090f11` | At tick80 synthetic W maps to raw `(0,1)` but original Mario returns `(0,0)`: lock1, ground0, jumping1. Walk speed is NaN. Bounded100-tick run exits0. |
| `a73a7a3cb524cfec4278d6580adbb10ec5a982ec174768beb7b67e7aefafab4e` | Corrected original ground-result and animation literals unlock input. Persistent NaN speed reaches the finite-area-query boundary at tick402; process exits1. |
| `ce907d58975cf8fbfd7e9f9203785d7d526e884e288be24e88f4098b4f9459dd` | Corrected slow-start division keeps motion finite; 960ticks exit0 at59.61FPS. Initial ground drift and zero airborne gravity remain. This run includes additional physical input outside the replay ranges. |
| `adde5fbbf7dc3187703f862cff99140aa94a9e1f274b342a3f9563684e471489` | Original gravity multiplier1 restores takeoff, reversal under gravity, and landing. The exact960-tick movement replay accepts and releases all four directions, moves on ground, and takes off/lands on both Space presses. Idle still drifts140.43units between ticks100–150, so overall validation correctly fails. |

`validate_demo.py` validates the actual user outcomes in the fixed script documented in its header. It checks finite positions/velocities/camera, standing still, accepted input with actual ground movement, release, and repeated takeoff/landing. `mWalkSpeed` is a normalized blend rather than world speed; movement uses the actual internal velocity magnitude. `analyze_controls.py` supplies compact observed-state groups without declaring success. All these are integration observations, not a claim that the entire Gateway game is implemented.

The benchmark's compact result now records the actual SDL script and window activation hint. An activation hint of0 did not prevent the window being focused on this Mac, so focus and actual key records remain authoritative. A missing or extra key event must not be silently explained away as deterministic replay.

The separately recovered original ground-probe function now passes the same end-to-end validation. `notes/mario-ground-drift-20260910` retains the real first-tick debugger baseline and its failing idle check; `notes/original-mario-ground-probes-20260910` records both passing 960-tick runs, including the package's1280x720 window size. The exact final binary is `e86ab9830fcb1a53347b8311c9fad4f5e98f302b67c4c28d4838f80b23856017`. All13 checks pass; idle displacement is zero, all four keys move Mario on ground and release, and both original jumps land35ticks after takeoff. The package-resolution run exits0 at59.93FPS. A separate frame615 capture visibly shows Mario jumping over the real planet through the original camera view and exits0.
