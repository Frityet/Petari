# Retail DemoSimpleCast closure

## Outcome

The PC scene-owned demo runtime now implements the retail `DemoSimpleCastHolder`
registry used by `GlobalGravityObj::init`. `MR::registerDemoSimpleCastAll` is no
longer an unconditional unavailable placeholder when a real scene demo owner is
installed.

This slice did not edit `pc-port/src/Game`. The three retail entry points are
provided by `compat/DemoCompat.cpp`, and their state belongs to the existing
scene-scoped `DemoSceneRuntime` compatibility owner.

## Retail evidence

RMGK02 `build/RMGK02/main.elf` and the matching decompiled sources establish:

- `DemoDirector` constructs `DemoSimpleCastHolder(0x200, 0x40, 0x80)`.
- The holder has separate append-only arrays for `LiveActor*`, `LayoutActor*`,
  and `NameObj*`; repeat registrations are retained.
- `movementOnAllCasts` traverses those arrays in that exact type order and calls
  `MR::requestMovementOn` for every entry.
- `DemoDirector::startDemo` marks the director active and calls
  `movementOnAllCasts` before starting the selected executor.
- The public `MR::registerDemoSimpleCastAll` overloads route through the
  installed director; there is no global success/default registry.

Addresses and representative instructions are recorded in
`retail-disassembly.txt`.

## Compatibility implementation

- `DemoSceneRuntime::Impl` owns three typed simple-cast lists, reserved to the
  retail holder's intended capacities.
- All three `MR::registerDemoSimpleCastAll` overloads require the active
  scene-owned demo runtime and append to the corresponding list.
- A real Time-sheet demo start resumes the three lists before starting its
  executor sheet. Simple casts remain distinct from DemoGroup cast membership.
- LiveActor destruction/release removes all matching entries from every simple
  list through the existing actor-runtime lifecycle hook.
- Scene teardown destroys the complete registry with its `DemoSceneRuntime`
  owner before stage roots are destroyed.
- Null registration is rejected explicitly; missing director ownership still
  throws. There is no route-specific exception, fallback transition, or fake
  success path.

`GlobalGravityObj` itself is not linked by this slice. Its simple-cast call is
now backed by the real generalized scene registry; the remaining gravity,
base-matrix follower, and math closure is tracked independently.

## Verification

See `verification.log`.

- Focused scene demo suite: **19/19 passed**.
- Existing talk/demo real-or-absent suite: **4/4 passed**.
- The built game archive exports all three typed
  `MR::registerDemoSimpleCastAll` definitions.
- `git diff --check` passes for every file in this slice.
