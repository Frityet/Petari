# Original animation-camera execution

CANM/CKAN event cameras now execute the unchanged Game `CameraAnim` controller
and its original accessors. The native event evaluator's interpolation,
target transformation, roll calculation, frame advancement, and endpoint
clamps are removed. `CameraAnimation::sample`, retained for resource
inspection, uses those same original accessors.

`OriginalAnimationCamera` owns the actual CameraAnim, CameraMan, pose objects,
two accessors, and validated native resource handle. CameraAnim does not
allocate a height arranger, so none is fabricated. Construction calls the
original `setParam` and `reset`; the first `calc` happens in the camera
phase after the selected target's movement.

The original controller now owns the cursor. Same-chunk animation requests
retain it and can update speed. A different chunk or an ended/restarted
event constructs a new controller. During active playback, `calc` samples
the current frame and then advances. After completion, it retains the last
sampled position/watch/up and updates only the final roll/FOV; the cursor
stops. An animation-only pause still samples against the live target while
holding the cursor. A director pause skips target movement and calculation.

`OriginalGameCamera::pose_param` exposes its retained original manager pose
for the same copy that `CameraDirector::startEvent` performs. The animation
owner copies this pose before reset, preserving unrolled up, roll, and
offset fields. Event-to-event changes copy the preceding controller's raw
manager pose. Explicit native views have an already rolled up vector and
are represented with zero additional roll at that boundary. A pending
event preserves its seed until its first calculation.

Safe-pose calculation uses the existing original
`CameraLocalUtil::calcSafePose`: its finite-pose arithmetic is the same as
`CameraManEvent::setSafePose`, including minimum watch separation, up-vector
repair, and field transfer. It additionally has the original NaN guard.
The rendered view uses the existing original CameraDirector pose conversion.
The shared `PublishedCameraTarget` was extracted from OriginalGameCamera
without changing its field mapping or validation, so both controllers use
the same typed target boundary.

## Verification

All seven selected macOS arm64 targets build: actor-event camera,
stage-start camera, original camera runtime, Mario walk, Gateway spin
checkpoint, Gateway demo scene, and showcase. The source verifier checks
four byte-identical Game imports and the shared Hermite fallback.

Sixteen actor event-camera cases pass with the real RMGK01 disc. They cover
fractional CANM and both CKAN tangent layouts against independent cubic
polynomials, arbitrary nonzero four-value type tags, repeated key times,
lookahead beyond the searched records, invalid read extents, copied manager
fallback, native metadata/alignment and lifetime, both pause modes, frame
completion, and same-chunk reuse. Real DemoMeetTico resource checks use the
original accessors and controller.

All fourteen stage-start cases and the original-camera runtime test pass.
The real Gateway spin fixture passes with CameraAnim executing its cutscene
camera. The real Mario walk still passes with 325.684 units of movement and
Wait -> Run -> Wait, including actor recreation/retirement. The Gateway demo
scene fixture also passes. Logs are retained locally alongside this note.
`git diff --check` passes.

This completes original CANM/CKAN execution within the existing event
service. The full CameraDirector/CameraManEvent priority/interpolation and
CameraManGame selection remain separate work. The current EYEPOS_FIX
native evaluator is the next remaining event controller to replace with
its original `CameraFixedPoint`; it must preserve that controller's reset,
offset accumulation, and three up modes. The complete bunny chase through
Rosalina has not been demonstrated by these bounded fixtures.
