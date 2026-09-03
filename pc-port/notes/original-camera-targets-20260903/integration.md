# Original camera target integration

Event cameras now bind the actual `CameraTargetObj` interface to the original
CameraParallel, CameraFixedPoint, and CameraAnim controllers. The previous
native actor/matrix snapshot conversion, basis normalization, omitted
gravity/ground fields, and limited virtual interface are removed from
EventCamera. The native stage-resource inspection APIs still support a
published target state; live event execution uses direct object overloads.
Controller owners borrow targets only during reset/calc and do not own the
stage's matrix/object/player targets.

The event owner retains one actual CameraTargetActor, matching
CameraTargetHolder's reuse of its cached actor target. Explicit actor requests
change its actor pointer, and the original movement method handles live,
clipped, and dead actors. Retired actor/object pointers are rejected through
the existing NameObj generation checks before any dereference. Cached axes
survive switching to a clipped actor, while position/velocity remain actual
actor getter results.

CameraTargetMtx now executes its original movement: raw basis extraction,
translation history, one-shot invalidation, CubeCamera lookup, and scene
gravity query. Both matrix and arbitrary CameraTargetObj arguments are
supported. CameraUtil's argument selection follows the original order:
object, matrix, actor, player, then no-target retention. An object argument
which happens to hold a matrix target does not gain matrix-argument behavior.

EventCameraRuntime owns the event target's camera phase. It advances the
selected target before invoking the camera manager/controller calculation.
Player targets use PlayerSystemService's retained target and frame guard;
other targets receive their actual virtual movement. A repeated frame does
not move the target or advance the event camera cursor again. Director pause
skips both operations and does not consume the pending frame.

The original matrix argument invalidations are significant: an explicit
CameraTargetArg request invalidates the next movement, and a changed event
chunk reapplies that matrix argument after its first target movement. The
second invalidation is therefore consumed by the following phase. The first
two movement deltas after a new matrix event are zero. An explicit request
for the same active chunk invalidates once; a no-target request does not.
A generic object argument does neither. Replacing a pending animation's
matrix argument with an object updates the pending rebind flag, preventing
a stale invalidation or null matrix dereference.

Pending requests and the last calculated event are separate. A request for
another chunk does not destroy the current controller before the next phase.
If the latest request returns to the calculated chunk, the pending change is
cancelled and the existing controller/cursor is retained. The selected target
still changes immediately when an explicit target argument is provided, as in
CameraManEvent::start. Only promotion of a different chunk during the phase
performs the additional matrix rebind. This models same-priority pending
replacement without adding a complete multi-priority event FIFO.
Cancelling a pending chunk clears both pending and current entries in that
slot, following the original cleanChunkFIFO rule.

## Verification

All seven selected macOS arm64 targets build. With the real RMGK01 disc,
23 actor/event cases and 14 stage-start cases pass. New independent target
oracles cover raw scaled bases, actual virtual actor matrices, Euler fallback,
point gravity direction (-0.6,-0.8,0), empty-manager zero gravity, translation
history, both request invalidations, generic object behavior, clipping/death,
retained actor orientation, pause/repeated frames, and pending target changes.
The matrix fixtures create real AreaObjContainer and PlanetGravityManager
scene objects; no unavailable-scene fallback was introduced in production.

Original-camera runtime, real Mario walking, spin-checkpoint, and demo-scene
integration checks also pass. The existing walking fixture still reports
325.684 units and Wait -> Run -> Wait, and verifies actor retirement and
recreation. The bounded spin checkpoint does not demonstrate the complete
bunny chase through Rosalina.

Source and retail instruction correspondence are in README.md and the
included verifiers. Build/runtime logs stay local in this directory.

## Remaining scope

Full CameraDirector/CameraManGame selection, event FIFO priorities, view
interpolation, target restoration on event exit, and complete Mario action
states remain outstanding. This integrates original target behavior into
the existing stage/event ownership; it does not claim that full director
startup or the Gateway demo is complete. Original scene startup dependencies
were recovered separately in `scene-current-start-iter-20260903`.
