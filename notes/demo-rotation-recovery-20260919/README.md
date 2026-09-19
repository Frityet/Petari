# Gateway orientation and crater recovery investigation

User report: only the starting guide bunny fails to turn; Mario has unusual
orientations while walking/chasing on curved ground, and the crater bubble
recovery behaves poorly. These are separate observations, not assumed to share
one cause.

## Confirmed corrections

- Restore the original `TVec3<f32>` value comparison in shared JGeometry.
  Without it, implicit SDK pointer conversion makes the unchanged
  `NPCActor::calcAndSetBaseMtx` Euler-cache comparison compare addresses and
  overwrite the control quaternion every frame. See `orientation/` for retail
  assembly, existing native-object proof, and regression details.
  The same missing equality also resets `MarioActor::updateForCamera`'s up-axis
  blend timer every update, including stable-head updates. The restored shared
  comparison fixes both original consumers without editing either one.
- Restore the original multiplier 5 in `Mario::saveLastSafetyTrans`. The native
  import used 30 while both canonical source and retail use 5. This is an
  original-source correction, not a new recovery policy. See `recovery/`.

## Validation in progress

`owners-green-final.json` records clean exit 0 for both final owner tests:
120 original frames for all three real DemoRabbit owners, and 360 original
frames for Mario, including stable-head camera timer expiry and reset after a
changed head direction. Shared rotation, XanimeCore, and J3D joint traversal
tests also pass in `equality-green.json`. Its first NPC run passed all checks
but aborted after scene teardown because the new diagnostic retained a vector
allocated in the scene heap. The corrected harness keeps its persistent vector
and exception strings in host memory; the final owner record is authoritative.

`recovery-red.json` records the actual player-owner regression failing: the
saved return point differs from the retail weighted centroid by 52.5351 units.
`recovery-green.json` records the corrected 360-frame original-process test
passing, including translated geometry, the 50-unit displacement cap, original
surface normal, recovery target inputs, delay/ground gates, ordinary subsequent
frames, and normal retirement. This bounded arithmetic probe does not establish
that all crater behavior is correct.

`orientation-before2.json` records a completed 4,000-frame fresh-save ordinary
Gateway run, using only controller input. The binary contains the safety-point
correction and read-only trace expansion, but still lacks vector equality. The
guide's stored quaternion and rotation matrix remain fixed while its gravity
and facing change. Mario's actual model base axes are now observed separately
from movement axes and placement Euler values. `orientation-before` was a
failed launch because its controller file had not been created; it provides no
gameplay evidence.

`run_original.py` and `run_checks.py` preserve commands, hashes, process results,
and bounded completion. The copied operator scripts are notes-only test drivers
that read actor observations and write ordinary, expiring controller commands.
No actor position, nerve, stage switch, or catch is written by these drivers.

## Scope

Do not attribute Mario's separate orientation report to the NPC equality fix.
Finite orthogonal matrices and successful compilation cannot establish retail
visual or collision parity. The full demo still has the previously documented
59 known-unlinked placements and broader content gaps. See the prior
`demo-system-verification-20260919` evidence for those limits.
