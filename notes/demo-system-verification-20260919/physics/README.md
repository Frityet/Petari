# Physics execution audit and original Binder restoration

This is bounded source and ownership verification, not proof that all demo
physics match the Wii. No fresh build or process test has run for this snapshot
yet; the root agent owns the ongoing baseline gameplay run.

## Confirmed changes

`LiveActor::updateBinder` now has the exact canonical body from
`decomp/src/Game/LiveActor/LiveActor.cpp`. The removed `ActorMotionCompat`
provider added a dead-actor early return absent from the original. Public direct
calls therefore differed. Normal `LiveActor::movement` already checks death
after `control`, so this audit does not attribute the observed route or terrain
behavior to that guard. The utility regression distinguishes the public call
from the ordinary movement guard, including clearing disabled Binder contacts.

Canonical `decomp/src/Game/LiveActor/Binder.cpp` was missing the complete
previously recovered response path. Restored it from the byte-identical native
file and corrected its comparator declaration to `const HitInfo*`. The native
source itself is unchanged. Its SHA-256 is
`7bd6ede6772644e0bedefbb16cba5833b92472e4a38c10aa5751da50b3355b51`, exactly the
source recorded in the historical retail evidence at
`notes/original-binder-reaction-20260903/runtime-evidence.json`.

That historical original-compiler verification covers nine methods and 816
instructions, with raw objdiff scores of 97.340065–100%, and explicitly describes
the remaining instruction differences. This is source identity with existing
evidence, not a new compilation result. The PC-only destructor remains only in
the native header/provider because it retires the native owned plane allocation.
The original one-projected-retry limit is preserved; the retail loop clears
`canMoveMore` at 0x8015DC2C. It is not a new workaround.

Removed `StageCollisionService::move_sphere` and `StageCollisionMoveResult`.
All callers were tests. The API switched a native service pointer before
calling Binder, but could not supply original Keeper membership. With the
restored sphere chain, this was an obsolete ownership model. The two old mixed
test targets are retired, with their complete source snapshots retained here;
`test-migration.md` maps retained and replacement assertions explicitly.

## Source execution evidence

Run `python3 notes/demo-system-verification-20260919/physics/check_sources.py`.
It compares explicitly selected definitions and removes only valid literal-only
`CP932(...)` wrappers from token comparison. It records file hashes and leaves
all changed identifiers, constants, branches and expressions visible. The
pre-change comparison remains in `selected-source-checks.json`; the current
comparison is `current-source-checks.json`.

20 of 24 selected definitions are now token-exact against canonical sources:

- Mario update/action/physical writeback/stick input/timers and MarioActor
  movement/control/control2/controlMain/base matrix;
- restored `retainMoveDir`, LiveActor matrix calculation and `updateBinder`;
- Spine update, scene movement/animation category order, GameSystem frame loop,
  frame-control setup and MainLoopFramework retrace wait.

The four selected differences are recorded without claiming token equality:

- `Mario::updateGroundInfo` uses `MarioStatus_13` instead of its equal-valued
  `MarioStatus_Recovery` alias (both 0x13 in the actual headers).
- `Mario::mainMove` reads `getTable()->mStickHeavyMaxAngle` directly instead of
  storing it in a local. `getTable()` directly returns the selected array member;
  the compared expression has no intervening mutator.
- `Mario::calcMoveDir` uses early returns/variable aliases and the native packed
  word mask for the corresponding original bit field. This was reviewed as
  source structure, not established by a new retail/native numerical replay.
- `LiveActor::movement` calls the narrow native lifetime-owned nerve and sensor
  adapters. They call the actual Spine and HitSensorKeeper methods. The observed
  phase order matches the canonical method: model/animation update, gravity,
  sensor messages, death gate, nerve, death gate, control, death gate, Binder,
  effects, camera, lighting, sensors, sound and shadow.

The original SceneExecutor executes collision actors' animation/matrix commits
before CollisionDirector and then Player movement. Native category dispatch in
`SceneScheduler::category_entries` reads the actual original allocated category
arrays; it does not substitute the scheduler's separate convenience sorted
snapshot for this path. `LiveActor::calcAnim` commits collision matrices after
its model animation calculation. This establishes source order, not numerical
moving-platform parity.

## Fixed per-frame simulation versus wall time

`OriginalProcess::frame` publishes one input state and calls one
`GameSystem::frameLoop`. The outer application increments completed frames only
after that call and Aurora presentation. The original frame loop renders,
updates, calculates animation, finishes and waits for VI. Selected original
Mario methods consume per-update velocities/timers rather than native elapsed
wall time. The separate `SimulationClock`/`FixedStepClock` helper is not used by
this original-process path; its old standalone tests cannot prove process
cadence.

The new read-only player-owner observation checks the actual original
`MarioActor::_378` movement counter over at least 100 consecutive process
frames. The canonical counter increments at movement entry and otherwise only
resets in construction/init. It must advance by exactly one each observed
frame. It does not measure or promise 60 wall-clock frames per second, catch-up
simulation, or emulator numerical parity. The utility test separately checks
one nerve step and one displacement per direct original movement invocation.

## Remaining limits

- The collision agent owns restoration/testing of the sphere Keeper/Parts/KCL
  chain. The previous adapter labelled every sphere contact as face (`_88=1`),
  which loses information consumed by MarioCollision. This audit did not observe
  a specific gameplay failure attributable to that loss.
- Synthetic original-query fixtures construct real original owners and valid
  KCL resources, but are not authored placement coverage. They do not insert
  fake actor registry membership. Moving-platform transport through native
  Triangle lifetime lookup still needs actual placed-owner evidence; the
  synthetic reaction checks alone are insufficient.
- No complete Wii/native trace comparison of Mario collision response,
  floating-point rounding, moving surfaces, camera-relative input or every
  actor's matrix pipeline has been performed here.
- A completed long gameplay run is useful route evidence. It does not establish
  that every unseen collision feature or every omitted placement is correct.

Build and runtime results will be recorded separately after the coordinated
source freeze; all new tests currently remain pending.
