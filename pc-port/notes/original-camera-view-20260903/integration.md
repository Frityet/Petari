# Original camera view phase on PC

The rendered authored game and event cameras now pass through the original
`CameraViewInterpolator`, retained across camera phases. Its Game source and
header are exact copies of the root decompilation. Raw CameraMan poses remain
separate from the rendered view: damping and collision never feed their output
back into the controller's authored position or local offset.

`OriginalCameraView` owns the original object and releases its Binder and typed
TriangleFilterFunc stage-arena allocations. It invokes the original director's
pose-to-matrix calculation, applies the CameraMan flag queries, and calls the
unchanged `updateCameraMtx`. The native output scope supplies the view/inverse
view and FOV context through the ordinary MR setters/getters. Its matrix update
follows CameraContext::setViewMtx; that original method ignores the extra flags
and position argument. The renderer's pose is derived from the resulting inverse
view, including the original damped orientation. Its look direction uses the
manager's focus distance to avoid one-unit subtraction precision loss at large
world positions. Aspect, clip planes, and projection offsets stay with the scene.

CameraSystemService runs this phase after the selected original controller. It
retains the last rendered view for pending events and pause/duplicate phases.
The original controller's virtual flags and authored chunk flags govern damping,
collision, and target-position correction; CameraAnim's original virtuals disable
anti-oscillation and collision. Player tracking passes the original target object
directly to the controller and view phase, preserving its identity and last move.

Event start interpolation is requested when a pending chunk becomes current,
following CameraManEvent::checkReset. The authored start override, requested frame
count, and original default of 60 retain their original precedence. Finish uses
the original end override/start override/request/default order. Zero-frame cuts
and the director's one-frame finish gate use the original interpolator fields.
Manual scene cameras are converted to CameraPoseParam for entry and finish, so
events start from the prior visible view and can blend back to that existing
source. Ending an event preserves its last rendered view until the next phase.

The original repulsive sphere/cylinder classes are registered in the shared area
factory, under the original CameraRepulsiveArea manager (order 34, capacity 128).
The sphere's zero repulsion is verified retail behavior. The cylinder uses the
original form/radius calculation. Camera collision uses the same StageCollision
service and ownerless Binder path as other callers, with the original near-plane
sphere radius and CameraThrough triangle filter.

## General collision fix found by the camera test

Reconstructing StageCollisionService at the same address could reuse both its
pointer and revision number, leaving Triangle::getAttributes with the previous
scene's cached rows. A NoThrough wall then remained solid after replacement with
a Through wall. The service now has a unique lifetime generation, included in the
shared attribute cache key. This fixes stale attributes for all triangle callers.

## Evidence

The source import, math recovery, and original compiler comparisons are recorded
in README.md and the accompanying verifiers. Root decompilation checkpoint
`2c68509cf` was pushed before the PC integration checkpoint. The original compiler
matches 98/98 area/filter/helper instructions after relocation; getQuat retains
all 13 symbolic control paths and four quaternion formulas, with one extra load
and different allocation/scheduling. These are functional correspondence results,
not a claim that the whole view interpolator is byte-identical retail code.

The live Mario walking run uses the real disc, original CameraParallel and
CameraTargetPlayer, and the shared collision scene. It walks 325.684 units,
retains NoSlip/Lawn prism 4642 and 14,521 loaded triangles, and transitions
Wait -> Run -> Wait. It verifies the raw authored orbit separately from the
rendered interpolated view and checks retirement/recreation. `mario-walk.png`
was captured and visually inspected: Mario remains framed on the planet; the
bright background/rendering limitations visible in earlier captures remain.

Runtime logs and the screenshot remain local and ignored. Final target results
from the complete validation run:

| Target | Result |
| --- | --- |
| smg-pc-camera-view-interpolator-tests | 7/7, including same-address collision replacement |
| smg-pc-camera-view-service-tests | 3/3, manual/event entry, pause, and finish |
| smg-pc-stage-start-camera-tests | 14/14, with real-disc authored camera lookup |
| smg-pc-actor-event-camera-tests | 23/23, including real-disc event resources |
| smg-pc-original-camera-runtime-tests | Pass |
| smg-pc-mario-gateway-walk-tests | Pass, 325.684 units, Wait -> Run -> Wait |
| smg-pc-gateway-spin-checkpoint-tests | Pass, bounded existing spin checkpoint |
| smg-pc-gateway-demo-scene-tests | Pass |
| smg-pc-showcase | Builds and links |

All nine targets were rebuilt after the final source changes. The four MR camera
position/direction getters now use their original inverse-view extraction and
normalization bodies, so they also read the bound original output context. All
three source/compiler verifiers and `git diff --check` pass. Runtime command:

```sh
SMGPC_REAL_DISC='../Super Mario Wii - Galaxy Adventure (Korea).rvz' \
  build/macosx/arm64/debug/<target>
```

Run from `pc-port`; build with Homebrew LLVM on PATH using `xmake build -j8`.
Existing compiler override warnings and the GPU destruction message appear on
clean shutdown and did not fail the runs. The screenshot above was captured in
the earlier successful walk run; the final run reconfirmed the walking proof
after the camera getters and collision cache change.

## Remaining scope

This brings up the original view interpolation system; full CameraDirector and
CameraManGame selection are still incomplete. OnlyCamera pose processing,
subjective camera, the full event priority FIFO, target restoration on event
exit, and complete Mario action states remain future work. The existing native
programmable-camera path has not been replaced with its full original manager.
The shared scalar square-root/reciprocal-square-root primitives still differ
numerically from PPC estimates, and collision ordering/moving geometry are not
claimed to have complete retail parity. The bounded spin checkpoint does not
verify the whole bunny chase through Rosalina.
