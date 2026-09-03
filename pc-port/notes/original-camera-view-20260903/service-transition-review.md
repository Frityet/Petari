# Service transition review and regression

The bounded view integration review found two concrete service ownership gaps:

- Creating the original view owner only during the first event calculation discarded an already published manual game view. The first positive interpolation rate is zero, so its previous matrix, watched position, and FOV must already be seeded from that scene view.
- Ending an event over a manual game pose left the event view cached indefinitely because only authored game controllers entered the return view phase. The existing manual pose must be adapted to a `CameraPoseParam` and passed through the same original finish interpolation.

The root agent corrected both in `CameraSystemService`, keeping raw manager data separate from the interpolated rendered view. The review also checked pending versus calculated event ownership, direct target identity, original cut/finish gates, typed Binder/filter lifetime, and view output caching. No additional concrete defect was found in that bounded review. Full CameraDirector and priority/FIFO closure remain outside this tranche.

`tests/CameraViewServiceTests.cpp` uses a real owned `CameraTargetObj`, actual CANM resource decoding and `CameraAnim` execution, real scene area/gravity registries, an empty built collision service, and the persistent original `CameraViewInterpolator`. There are no production fixture hooks or camera-name branches.

The entry test supplies a manual eye `(0, 0, 600)`, watch `(0, 0, 0)`, FOV40 and an event eye `(100, 0, 1200)`, watch `(100, 0, 0)`, FOV80. The original four-frame switching calculation recursively uses squared rates `0`, `1/16`, `1/4`, and so on. It checks rendered eye/FOV values `(0,600,40)`, `(6.25,637.5,42.5)`, and `(29.6875,778.125,51.875)` while the raw event view has already reached its complete authored pose. Repeated phases and a paused phase cannot advance target movement or rendered interpolation; resuming the skipped phase advances exactly once.

Two finish tests check that a zero-frame finish restores the manual eye, direction, FOV, and projection metadata; and that a positive finish first retains the event view, then moves toward the manual view without advancing the ended event target. Direction is checked independently from watch distance because a rendered view matrix need not face the controller's undamped watch position.

The registered native target is `smg-pc-camera-view-service-tests`; its xmake test name is `camera_view_service`. Native builds and execution are serialized by the root agent. This subtask performed `git diff --check` only; it does not claim a passing native run before the root reports one.
