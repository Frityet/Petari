# Actor base transform and retained scale regressions

The focused extension to `tests/LiveActorUtilRealOrAbsentTests.cpp` reuses its
existing real-disc Tico actor, runtime, and archive. No production hook or new
resource API was added. It independently parses the same authored Tico model
and Wait animation and passes them to the original joint-tree owner. The
owner's numeric behavior is established separately by the nine traversal
groups; here it is an oracle for the actor/model/renderer argument plumbing.

The actor test verifies:

- A supplied matrix containing nonuniform scale and shear is copied literally
  by the raw-Mtx and TPos setter paths and returned through the actor's original
  base pointer. It is neither normalized nor multiplied by `actor.mScale`.
- `MR::setBaseScale` changes retained model scale without modifying actor scale
  or the stored matrix. Actual authored AllRoot and Body joint queries use
  that independent scale. Changing it to unit scale produces a different real
  root matrix, so the comparison cannot pass because the resource ignores it.
- The explicit scale override survives another base-TR assignment. Both named
  joint matrices retain their addresses throughout refreshes.
- A test actor's original virtual base-matrix method changes `mScale` after
  calling the ordinary base implementation. Direct `calcAnmMtx` still publishes
  the scale captured before that virtual call. Its next call publishes the new
  scale. Previously retained joint pointers are inspected before another joint
  query can refresh them, covering the scheduled cache-refresh path as well.

The helper restores actor position/rotation/scale, BCK phase, and calculated
matrices before the existing stopped-animation and GPU packet tests continue.
It introduces no extra actor/model resource load.

`tests/AuroraNativeTests.cpp` now expects `(13,22,29)` from an actor at
`(10,20,30)` rotated 90 degrees around Y with actor scale `(2,3,4)` and local
offset `(1,2,3)`. Root `FixedPosition` retains the actor's raw base TR matrix,
so actor model scale does not affect this offset. A separate explicit scaled
matrix produces `(22,26,28)` and demonstrates the original normalization
contract: translation is calculated before normalization, default basis lengths
become one, and setting `mNormalizeScale` false preserves lengths `(2,3,4)`.

Both test files passed `git diff --check`. Native build/execution is serialized
by the parent task; its final checkpoint logs record these regression results.

## Unrelated stale GamePad expectation exposed by the full runner

The new live-Tico regressions passed in the parent run. The full Aurora-native
runner then exposed a stale test that expected every GamePad query to throw
without `RuntimeContext`. The existing production `GamePadUtilCompat.cpp`
already reads Aurora's global `WpadService` directly; that production code was
not changed by this tranche.

Only the stale test was replaced. It feeds actual Aurora button/stick state
without a title runtime and checks first-frame triggers, sustained holds without
retriggering, button release, and stick-direction reversal through the MR APIs.
The original requirement for a real camera during world-stick projection remains
an explicit failure assertion. The test restores the prior Aurora service state
on exit. This update aligns the test with the existing shared input provider;
it is not a new input implementation or a relaxed camera requirement.
