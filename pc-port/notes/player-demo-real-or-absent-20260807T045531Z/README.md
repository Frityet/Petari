# Player and demo real-or-absent boundary

## Outcome

The player utility and programmable-demo compatibility paths no longer turn a
missing player into plausible state.

- Player position, rotation, velocity, gravity, center, and base-matrix
  queries do not manufacture origin, world-down, or identity values.
- Player axes, distance, visibility, control, swing permission, animation,
  oxygen, and opening-demo teardown reject use without a genuinely attached
  `LiveActor`.
- Degenerate attached-player axes reject instead of becoming canonical world
  axes.
- Programmable/puppetable demo start requires that genuinely attached player;
  it cannot install demo control over an empty player service.
- Opening-demo completion requires the real actor before releasing demo
  control and starting the retail `Wait` animation.
- The removed stage-player-only permission restoration helper is not retained
  as a compatibility API. The retail story-event setter updates an attached
  player through the ordinary Game API.
- Debug animation evidence writes `frame_max=absent` when no parsed animation
  range exists; it no longer records a fabricated zero.

The title sequence remains explicitly unavailable until the complete real
MarioActor closure provides its binder/auto-rush event. No player-specific
state jump or global message broadcast was added.

## Changed paths

- `src/compat/PlayerUtilCompat.cpp`
- `src/compat/PlayerUtilCompat.hpp`
- `src/compat/PlayerStateCompat.cpp`
- `src/compat/DemoCompat.cpp`
- `src/compat/DemoUtilCompat.cpp`
- `tests/PlayerUtilRealOrAbsentTests.cpp`
- `tests/DemoSceneRuntimeTests.cpp`
- `tests/AuroraNativeTests.cpp`

## Verification

- RMGK02 decomp build and configured DOL SHA-1: passed.
- Aggregate PC build and the final 31-target test matrix: passed after the
  concurrent exact LodCtrl/NPCActor/StarPiece source-boundary migrations.
