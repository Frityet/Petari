# Original fixed-point camera execution

`CAM_TYPE_EYEPOS_FIX` now executes the unchanged Game `CameraFixedPoint`.
The native event evaluator's instantaneous watch-offset calculation, up-mode
switch, roll trigonometry, and basis repair are removed. Both static event
types use `OriginalGameCamera`, which owns the selected actual controller
and its arena-style child allocations. FixedPoint has no height arranger;
the owner allocates none for it. Parameter translation follows the original
`CamTranslatorFixedPoint` and `CamTranslatorParallel` fields and units.

Event construction copies the raw preceding CameraMan pose, then runs the
original reset. The first normal calculation follows in that same camera
phase, without reapplying the preceding manager's local offset between the
two calls. This matters because FixedPoint's reset calls calc internally.
Only the final result passes through the original safe-pose calculation.
Ordinary frames retain the last published local offset. Same-chunk requests
retain the controller; changed chunks reset from the preceding manager pose.
Director pause skips the phase, leaving both pose and controller state held.
Multiple pending requests preserve the full raw manager seed until a phase
actually calculates it. The former rendered-pose fallback is removed: it
lost offset and unrolled orientation fields when a pending event inherited
both the preceding visible view and a raw manager seed.

The original CameraManEvent interpolation-frame precedence is also applied
to its local-offset-reset request: a nonnegative authored enabled camint
overrides the requested count, otherwise a nonnegative request is used,
otherwise the default is 60. A zero result sets mRequestLOfsReset before
reset and calc; the flag clears after calculation. Repeating the same chunk
does not resend this reset. This does not yet activate the full director
view interpolation or event priority queue.

The actual controller now supplies all three up modes: transformed zone-up,
previous-up transported through successive view rotations, and the global
player's up vector. The latter is independent of the event target's up and
of a bound player's forced render matrix. PlayerUtil's shared typed player
bridge calls the original MarioActor up/front/side accessors, preserving raw
vector values; the matrix fallback remains for generic non-Mario actors.

The Game start-camera initialization path retains its prior lifecycle.
Full CameraDirector/CameraManGame startup, camera selection, priority, and
view interpolation remain outstanding. These changes do not establish
the full Gateway bunny chase through Rosalina or the full Mario action
state system.

The next target-boundary work is the original matrix/actor camera target
lifecycle. The existing native matrix snapshot normalizes its basis, while
CameraTargetMtx movement retains raw matrix columns; only player targets
currently receive camera-phase movement through CameraSystemService.
This pre-existing boundary remains separate from the controller work here.

## Verification

All seven selected macOS arm64 targets build: actor-event camera,
stage-start camera, original camera runtime, real Mario walk, Gateway spin
checkpoint, Gateway demo scene, and showcase.

With the real RMGK01 disc, 19 actor-event cases and 14 stage-start cases pass.
FixedPoint coverage includes independently calculated accumulated offsets
(19, 27.1, 40.951), both offset-reset mechanisms, return to smoothing after
a manager reset, zone-transformed eye/up, path-dependent transported-up,
global player-up distinct from target-up, and the 300-unit safe distance.
Event lifecycle tests cover deferred calculation, pause/resume, same-chunk
retention, manager-offset transfer between chunks, unchanged-chunk zero
requests, and seven authored/requested interpolation precedence cases.
A pending-request regression starts with offset 19, requests a second
nonzero-roll event twice before movement, then requires offset 34.39 and
vertical offset 20.634 from the retained raw manager state.
Existing CANM/CKAN and real stage-resource cases continue to pass.

The original-camera runtime, real Mario walk, real Gateway spin checkpoint,
and Gateway demo-scene executables all pass. The walk executes the actual
normal/bound Mario vector accessor regression, then moves 325.684 units on
the 14,521-triangle stage collision mesh, playing Wait -> Run -> Wait with
12 Run model packets. Actor recreation and retirement still pass. The spin
checkpoint remains a bounded progress fixture; it does not prove the full
bunny chase.

`verify-source.py` passes for the four byte-identical Game imports, original
accessor source contracts, and recorded retail accessor instruction ranges.
`git diff --check` passes. Raw build/runtime logs stay local alongside this
note and are not committed.
The final walking run also captured `mario-walk.png`; visual inspection
shows Mario framed on the planet. Existing bright background surfaces
remain visible. This image checks the unchanged gameplay camera path,
while FixedPoint behavior is verified by the event-camera numeric cases.
