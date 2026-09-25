# Camera, input and player handoff audit

2026-09-25; baseline `a71fc2ea7`. Read-only production audit; no builds, tests, controller actions or source edits. This note is the only file written by this lane.

## Result

No concrete Gateway-blocking camera/input/physics glue defect was established. There is no justified production fix from this bounded pass. Keep the original movement and camera algorithms; use the currently running controller route to identify a failed original gate, if any. This is not a proof of exhaustive physics or rendering correctness.

## Actual authority and rejected false leads

- `src/app/OriginalGameApplication.cpp:314` publishes controller state, calls `Scene::beginNativeFrame`, then the actual `GameSystem::frameLoop`. `src/Game/Scene/Scene.cpp:161` begins scheduler diagnostics; it does not call the older `RuntimeContext::begin_frame` input publisher. Therefore the two input implementations do not double-consume the same frame on this path.
- Actual controller path: `OriginalGameApplication.cpp:354` → Aurora `KPADRead` (`aurora/lib/wpad.cpp:434`) → original `WPadHolder`/`WPad` → `GamePadUtil.cpp:165/225` → `MarioActorPad.cpp:231` → `Mario.cpp:1534`. WASD is distinct from Wii D-pad arrows (`src/render/RendererService.cpp:354`); walking does not accidentally request subjective camera.
- `Mario::inputStick` multiplies by 1.5, clamps each axis and magnitude, applies the authored 0.1 angle margin, then reconstructs a unit-bounded direction (`Mario.cpp:1543-1603`). `MarioModule::calcWorldPadDir` additionally shapes axes (`MarioModule.cpp:258`), before `Mario::calcMoveDir` uses the original camera basis and smoothed gravity (`MarioMove.cpp:985`). The current notes-only `follow_actor.py:31` already inverts both shaping steps. Sending a desired final direction directly as raw stick values was a historical operator mistake, not a reason to modify Game math.
- `src/camera/CameraDirectorRuntime.cpp:57` reads the actual `CameraContext::mViewInv` and camera parameters for native presentation. It does not calculate a substitute target or movement basis. Mario reads actual camera directions (`MarioActor.cpp:2721`, `MarioModule.cpp:398`), not this presentation pose.
- `PlayerSystemService::synchronize_attached_actor` (`RuntimeServices.cpp:1106`) only snapshots original actor state. Searches of current production sources found no callers of its `attach_actor` or mutating `set_base_matrix`. This legacy bridge is not the player physics authority on the actual GameSystem route, so deleting it would be cleanup rather than a demonstrated progression fix.
- Actual-process keyboard diagonals are `(±1, ±1)` whereas legacy RuntimeContext normalizes them. Ordinary Mario walking applies its own clamp and angle reconstruction, so this difference is not evidence of excessive diagonal Mario speed or a rabbit-route blocker. Do not change generic SDK stick behavior merely to mask it.

## Prior route evidence and what it does not mean

The early `notes/original-gateway-chase-20260919/README.md` only reached two catches. The later `notes/demo-system-verification-20260919/final2-run.md:3-35` supersedes that limitation: all three caught, tower appeared, Rosalina alive/unhidden at frame 11440, 30000 frames and exit 0. That historical result is not current-build validation and did not prove an unobstructed rendered Rosalina.

The same later run traced crater recovery to the authored PullBackCylinder, observed the third rabbit on actual map contact, and caught it after ordinary rightward navigation around the crater. It also documented ordinary jumps clearing the pipe rim and a stone obstacle. Therefore a controller driving straight toward a world-space goal can stall even when original collision/recovery is functioning. A new current stall should be classified using actual Mario status, ground host/gravity and accepted controller input before changing collision or camera code.

## Useful next action

Finish the current controller-only route and inspect its first actual failure, if present. The existing trace already includes Mario movement-up, camera axes, stick position and velocity (`OriginalProcessTrace.cpp:174-179`) plus consumed WPad stick (`:302`). Those fields can distinguish input/basis error from an authored obstacle without a new fixture or game-state override. No additional broad camera/runtime cleanup is recommended as part of reaching Rosalina.
