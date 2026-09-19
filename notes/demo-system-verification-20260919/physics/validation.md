# Validation attempts

The root agent owns serialized builds and process runs. These attempts are
recorded without weakening production owner checks.

- `../build-binder.log`: first new test compile failed because templated vector
  `set` received mixed integer/float arguments. Corrected only the test argument
  types and preserved the failed log.
- `../cpu-corrected.json` and corresponding logs: corrected Binder executable
  linked but its derived-movement test attempted the aggregate host scene loop.
  That also executed ClippingDirector, which correctly required CameraContext
  (decimal SceneObj 23). The test now registers/applies the original MapObj
  category and calls that category's normal callback path. It tests Binder's
  ordering within a registered callback, not a fabricated full camera scene.
- The same batch's registration executable passed its first five independent
  storage/metadata/heap-routing cases. Generated geometry then failed because
  its old local root-heap creation was nested inside the newly added original
  owner fixture. The test now borrows that actual fixture's allocation domain.
  Production heap ownership checks are unchanged.
- `../test-sphere.log`: the collision peer's original sphere query suite passed.
  This is lower-query evidence, not a full original-process physics result.
- `../focused-smg-pc-original-actor-utility-tests-test.log`: the original public
  Binder/death-guard and per-call nerve/displacement checks passed, alongside
  0/128 gravity output mismatches and 0/27 easing mismatches.
- `../focused-smg-pc-original-process-player-owner-tests-test.log`: original
  startup reached the requested stage, then aborted on the separate authored
  area registry mismatch for SwitchCube. The cadence observer has not passed;
  that failure must not be described as a physics or cadence pass.

The final Binder/storage rerun and the new read-only player cadence observation
remain pending at this point. Root's unchanged baseline completed 30,000 frames
with two rabbit catches; it did not finish the third catch/Rosalina route, and
is not evidence that the newly edited executable has run.
