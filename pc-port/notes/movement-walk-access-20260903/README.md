# Restore original walking speed and inertia bodies

The PC-only substitutes in `Mario::getTargetWalkSpeed` and
`Mario::decideInertia` have been removed. Each function now has the exact
existing root body. This removes 28 lines of PC substitutions and guard
markers from Game source; it introduces no new Game algorithm.

`getTargetWalkSpeed` restores the squat return, severe sinking-floor checks,
the 256-step sink multiplier, movement reduction, and item-dash multiplier.
`decideInertia` restores status/ice/slip dispatch, original braking priorities,
tornado acceleration/braking, reflection and start-spin timers, and the
brake-floor multiplier. These paths remain dependent on their original state
producers. Restoring these functions does not activate absent gameplay states.

## Source and binary verification

- Root `src/Game/Player/MarioWalk.cpp:82` supplies the speed body. RMGK01
  `0x80306d64`, size `0x110`, confirms the same branches and multiplier order.
- Root `src/Game/Player/MarioMove.cpp:810` supplies inertia. RMGK01
  `0x802ebd00`, size `0x2c4`, confirms the early returns, zero-input override
  order, `0.08f` start speed, timer updates, and floor-code 32 scaling.
- Original binary instructions `0x802ebf58` through `0x802ebf70` set the
  start-spin cooldown to 60 and immediately decrement it to 59. The restored
  source and focused checks preserve this transition.

Disassembly used the supplied RMGK01 DOL with SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`. Local disassembly files
`mario_get_target_walk_speed.*` and `mario_decide_inertia.*` remain ignored in
`build/compat-math-oracle/`; no game binary data is staged.

## Dependency closure

The native walk archive already supplies the original ice/slip inertia
helpers, `MarioModule::getFloorCode`, `FloorCode::getCode`, and Mario's gravity
accessor. State and severe-floor queries were source-complete but their full
translation units are excluded from the current build. The new
`src/compat/MarioStateAccessCompat.cpp` exposes exact original bodies for:

- `MarioActor::getGravityVec` from `MarioActorGravity.cpp`;
- `Mario::isWalling` from `MarioWall.cpp`;
- `Mario::getCurrentStatus` and `isStatusActive` from `MarioState.cpp`;
- `Mario::checkCurrentFloorCodeSevere` from `MarioCollision.cpp`.

The first two accessors were initially added by the parent for the original
camera target. This general file replaces that temporary
`MarioCameraAccessCompat.cpp` path and shares real player state with both
camera and movement. Every accessor body was compared byte-for-byte with its
root definition. No state value is synthesized by these providers.

The null active-state list is handled by the original query implementations.
The Mario constructor initializes all newly used counters, target modifiers,
FloorCode ownership, and ground triangles. `FloorCode::getCode` already handles
null or invalid triangles. Neither restored body dereferences an absent
MarioWall, MarioSkate, or other state object.

## Verification

The existing PlayerSourceMirrorTests binary passed on the changed tree:
96/96 retail source branches and 63/63 headers exact. Explicit function
comparison confirms both restored bodies and all five accessor bodies are
root-identical. `git diff --check` passes for the owned files.

`tests/MarioWalkParameterTests.cpp` provides focused checks called with the real
initialized actor. It temporarily installs distinct exactly representable
constant values and parameter inputs, then restores every changed actor field
and the entire constant table, including on exception. Cases cover sink,
reduction, dash, squat, braking priority, tornado choice, reflection timer,
start-spin expiration/cooldown, ice/slip dispatch, and overspeed priority.
The test does not run frame movement or fabricate state objects.

The Apple Silicon debug build and live-actor parameter checks passed on
2026-09-03. The real-disc Gateway test passed stand/walk/release, 60 idle
frames, player replacement, and teardown: walking displaced Mario 325.684
units over real KCL and drew Wait -> Run -> Wait animations. The original XZ
camera tracked the actor during the same run. Detailed evidence is in the
movement/camera note. The broader PC `Mario::update` replacement, inward force
scaling, and idle grounding latch were not changed: accurate restoration of
that orchestration still requires the original missing implementation and
its complete state lifecycle.
