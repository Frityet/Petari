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

## Validation

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

## Corrected live run

`after.json` records 12,000 completed original frames, exit 0, process 33155
reaped, and an unchanged bundle SHA
`4658f1cc3d770857e5e5c1a5d36d64103dab2e6a61bffb8f62c8bfd3a9f475f6`.
Elapsed time was 250.949 seconds. The source correction is published as
`a37e1c799398e9f20f4ffc6651d6d40964b174ed` on `origin/pcp-aurora`.

The notes operator initially failed because its optional manual-input file was
missing. Its first attempt published no nonempty command. The native trace
nevertheless records ordinary live controller input before the resumed
operator's first accepted command at frame 6829. The file was then created and
the same notes-only driver resumed from observed scene state. This is a mixed
ordinary-input run, not an identical-input before/after replay. The user
confirmed they were not controlling the window when asked later in the run.

| Original observation | Frame |
| --- | ---: |
| Bush rabbit caught | 7350 |
| First original Tico dialogue completed | 7720 |
| Pipe rabbit caught | 8970 |
| Second original Tico dialogue completed | 9340 |
| Crater mode-3 MarioWarp first observed | 9770 |
| Last active warp sample, within 0.00846 units of target | 9920 |
| Normal Mario state resumed | 9930 |
| Hole rabbit caught | 10590 |
| Third original Tico dialogue completed | 11080 |
| Rosetta alive and model not hidden | 11550 |

The corrected guide now changes its control quaternion and model basis while
retaining its authored Euler placement, and stays aligned with local gravity.
`after-frame1800.png` visibly shows it upright and facing Mario during Talk0.
The read-only matrix analysis observes no inverted grounded Mario model basis
in this sampled run, and the original camera-up timer now reaches zero for a
stable head direction. This establishes the repaired cache behavior; it does
not prove every Mario animation, camera transition, or GX upload matches Wii.

Exactly one crater recovery occurred. Its destination matches the original
saved point plus cached KCL normal times 160 within 0.0003874 units; Mario
reached that target and resumed normal motion outside the authored cylinder.
The original unbound ground flag first returns at frame 9940, with actual
collision host 722 / prism 2076. No second recovery was observed through frame
11990. See `recovery/` for the
sampled trajectory and grounding limits. Rosetta's scene state is verified;
this run does not include an unobstructed Rosetta pixel capture. A requested
late live screenshot failed with Computer Use `timeoutReached`, so no image
from that attempt is claimed.

Raw trace/log hashes and compressed evidence are listed in
`runtime-artifacts.json`. Raw diagnostic logs retain their original whitespace.
Both live operators and all game processes have ended; the controller file is
neutral. Earlier unrelated staged notes and workspace edits were preserved.

## Scope

Do not attribute Mario's separate orientation report to the NPC equality fix.
Finite orthogonal matrices and successful compilation cannot establish retail
visual or collision parity. The full demo still has the previously documented
59 known-unlinked placements and broader content gaps. See the prior
`demo-system-verification-20260919` evidence for those limits.
