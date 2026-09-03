# Original OnlyCamera in the PC camera pipeline

Authored game cameras and event cameras now run through the original
`OnlyCamera` before the original `CameraViewInterpolator`. The imported Game
source and header are unchanged root copies. The original compiler comparison
verifies all 499 instructions in six methods and the complete vtable; see
README.md and the source/compiler verifiers for the exact scope.

`OriginalCameraView` retains one OnlyCamera across manager changes, together
with its arena-allocated CameraPoseParam. Actual OriginalGameCamera and
OriginalAnimationCamera managers are passed directly to `calcPose`. Manually
published native poses use a retained original CameraMan as their adapter.
Raw manager poses are not rewritten by the pose/view calculations.

The original FOV contract is preserved: OnlyCamera leaves its pose's FOV at
the CameraPoseParam constructor value of 40, while the director passes the
active manager's FOV separately to view interpolation. Front/upper offset
fields are also left untouched by OnlyCamera, as in retail. Its corrected
watch/up/position/roll are used to produce the camera matrix. The resulting
rendered inverse view is written back to the actual active manager's mMatrix.

## Director handoffs and native lifetimes

- Explicit authored-manager reset still runs after target movement and is
  deferred while paused. It now also requests the original zero-frame view
  interpolation and OnlyCamera reset. Ordinary camera/controller reactivation
  retains OnlyCamera's previous adjusted pose.
- Event entry preserves both the raw manager pose and previous rendered matrix.
  Each pending event owns those snapshots. Event-to-event changes inherit the
  calculated event manager state; wrappers assign the matrix before invoking
  original controller reset. Repeated pending changes retain their seed.
- The view target is the actual pointer returned by the controller's calc,
  distinct from its selected input. CameraAnim returns null even when its
  selected target moves and provides a transform. That null now reaches the
  interpolator and prevents invented target-motion correction for animation.
- Event exit with resetView copies the latest OnlyCamera pose and current
  rendered matrix into the returning game manager only if the finish request
  has not forced a cut. This check follows the finish-interpolation request,
  matching CameraDirector::endEvent. The game scene keeps its projection
  geometry, and the next game calculation reapplies authored camera parameters.
- Withdrawn player target publication supplies no view target, avoiding a
  borrowed pointer after player retirement while retaining the last valid raw
  manager pose. The regression keeps advancing several phases after retirement
  before installing a replacement target.
- A CameraSystemService movement phase now runs once per frame. The guard
  covers target, raw manager, OnlyCamera, view, and shake together, so duplicate
  calls cannot accumulate local offset in the manager while leaving the view
  unchanged. Paused calls do not consume a movement phase.

The original mCalcIdeal state has no internal activation in these retail
methods. The zero-frame-move flag is not read by them; the safe-pose method
clears it. No synthetic trigger or motion policy was added. Tests exercise
the original ideal-position branch by setting its existing public state.

## Validation

The new OnlyCamera target has nine groups covering close/coincident watch
positions, first versus later up recovery, ideal-position arithmetic and
boundaries, reset fields, manager FOV separation, actual matrix feedback, and
owner/manager lifetimes. The service target adds owned matrix-seed and actual
used-target regressions. Existing stage-start tests now distinguish resetView
true from false and verify the original pose handoff rather than the earlier
retained-pose approximation.

Final macOS arm64 debug build passed for the showcase and all ten camera,
walking, spin-checkpoint and scene targets. The separate feedback target also
built and passed. Runtime results:

| Target | Result |
| --- | --- |
| OnlyCamera | 9/9 groups |
| CameraViewInterpolator | 7/7 groups |
| CameraViewService | 6/6 groups |
| StageStartCamera | 14/14 groups |
| ActorEventCamera | 23/23 groups |
| OriginalCameraRuntime | Local target scope and mode gates passed |
| FeedbackRealOrAbsent | 5/5 groups, including repeated/paused shake phases |
| MarioGatewayWalk | Real-disc stand/walk/release/idle and lifetime proof passed |
| GatewaySpinCheckpoint | Real-disc authored spin-unlock checkpoint passed |
| GatewayDemoScene | Real-disc original placement/geometry scene proof passed |

The 59 camera groups passed together after the final production changes. The
walking fixture moved 325.684 units and observed Wait -> Run -> Wait, retaining
the actual floor prism and collision data. Its new 1280x960 capture was visually
inspected: Mario and the curved Gateway surface render with the original camera
pose pipeline. This capture is a walking frame and is not jump validation.
The spin fixture covers its named checkpoint, not the complete chase.

Both source and original-compiler verifiers passed. Compiler evidence covers
499 instructions after 131 checked relocations and all nine vtable words after
seven relocations. Logs, object files, disc data and images remain local and
ignored.

## Remaining scope and movement follow-up

The complete original CameraDirector/CameraManGame selection, event priority
FIFO, subjective cameras, camera target restoration, and reset of an active
event manager are not yet fully integrated. Existing numerical and moving
collision parity limits remain as recorded in the preceding camera-view
checkpoint. This work does not establish complete camera or Gateway parity.

The next movement audit is in `../mario-jump-audit-20260903/README.md`: the PC
Mario update/mainMove path currently bypasses the original jump/action
systems. The intended next step is to restore those original routines and
provide their general dependencies, replacing the grounded-only PC path.
The complete bunny chase through Rosalina still requires live verification.
