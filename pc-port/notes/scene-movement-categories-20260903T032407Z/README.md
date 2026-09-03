# Scene movement categories and retained visual execution

The existing native scheduler treated NameObj's movement-off flag as an
animation, view, and draw-off flag as well. That makes scene suspension hide
ordinary world objects when the demo controller is eventually enabled.

## Source-backed behavior

- `src/Game/NameObj/NameObj.cpp::executeMovement` alone tests bit 0 of `mFlag`.
- `src/Game/Scene/SceneNameObjListExecutor.cpp` constructs the movement list
  with `&NameObj::executeMovement`, the animation list with `&NameObj::calcAnim`,
  and the draw list with `&NameObj::draw`. The latter two do not use the
  movement-off gate.
- `src/Game/NameObj/NameObjExecuteHolder.cpp::requestMovementOn/Off` visits the
  current execute records. `NameObjExecuteInfo` compares its `s8` movement
  category with the requested category narrowed to `s8`, then requests that
  object's movement flag change. This is not a persistent category policy for
  future registrations.
- `src/Game/Scene/SceneFunction.cpp::CategoryList::requestMovementOn/Off`
  forwards to `MR::requestMovementOnWithCategory/OffWithCategory`.
- `src/Game/Scene/SceneNameObjMovementController.cpp` uses those category APIs
  for the common demo exceptions, player and player-decoration exceptions,
  enemy-only talk pause, player-only pause, and resume paths.

## Changes

`SceneMovementCompat.cpp` supplies the original category entrypoints through
the active scene scheduler. Explicit scheduler bindings take priority over the
RuntimeContext fallback, preserving nested scene/test isolation. A missing
owner fails before any object flag changes.

`SceneScheduler` now applies category requests to its current registrations.
It keeps movement suspension separate from animation calculation, view/entry,
ordinary model draw, 2D-model draw, and draw-type callbacks. Dead actors,
clipping, draw connection, and actor-specific no-calc flags retain their own
existing gates. The preceding animation publication/joint-refresh fix remains
unchanged.

The PC `SceneFunction.hpp` now has the byte-identical original `CategoryList`
declaration needed to compile unchanged callers. No Game behavior was edited.

## Verification

`SceneMovementRuntimeTests.cpp` executes the original CategoryList and MR APIs
through the real scheduler. Its six cases cover missing owner rejection,
category selectivity and resume, retained virtual visual callbacks for a
paused LiveActor, dead/draw-disconnected boundaries, signed-byte category
identity, current-registration traversal without a future-category latch,
and nested binding restoration.

The focused test requires the ordinary `smg-pc-game`/Aurora dependency set and
no disc or graphics window. A passing virtual draw callback proves scheduler
dispatch, not GPU rendering.

The existing `LiveActorUtilRealOrAbsentTests.cpp` real Tico fixture was then
extended to cover both changed model-buffer selectors. It pauses the actual
NPC movement category, submits parsed 3D Tico/BCK packets while the BCK clock
stays at frame 4, and checks a changed actor transform refreshes the same
retained Body-joint pointer despite suspension. A bounded frame loop waits for
actual Aurora draw-call evidence. The same model is rendered through the
Model3DFor2D path while paused, temporarily disconnected to prove packet absence,
then resumed through CategoryList to require exactly one tick to BCK frame 5.
Recalculating animation while movement is stopped does not advance a retail
animation clock; that clock is advanced by LiveActor movement.

Source checks: `git diff --check` passed, and the restored CategoryList
declaration was compared byte-for-byte with the root header.

Build/run results are recorded below after the parent's serialized build.

## Remaining closure

This enables reusable movement-category primitives, not the complete
SceneNameObjMovementController or programmable demo system. The full controller
still needs a true scene NameObj roster (including unconnected objects), the
NameObjGroup pause exemptions, CinemaFrame, water/image-effect ownership, and
the remaining exact SceneObj references. The existing native NameObjFunction
immediate flag synchronization is also retained; retail defers global flag
synchronization to the controller boundary. No invented replacement owner or
alternate demo API was introduced.

Sensor dispatch and scene-wide actor broadcasts have their own pre-existing
suspension filtering and remain outside this visual/category tranche. Those
need a separate source-backed review before enabling complete demo pause/end.

The Gateway route audit also verified that ordinary showcase entry preserves
selected-file progress 5, while `gateway-spin` uses synthetic Tico/Rosetta
LiveActor casts at progress 10. Only DemoRabbit is active in the factory among
the bunny/Tico/Rosetta family. `DemoUtilCompat.cpp` still rejects programmable
demos; replacing those checks without the missing movement/cinema/player
ownership would falsely imply bunny catching works.

## Current macOS validation

The serialized LLVM 23 build passes. Category-runtime checks pass 6/6.
The extended real-disc LiveActorUtil test passes 6/6 and exercises 12 paused
Tico 3D packets, four paused 2D packets, actual GPU draw submission, retained
joint refresh, and animation frame `4 -> 4 -> 5` across pause and resume.
The 19/19 synthetic DemoSceneRuntime checks also pass after completing the
existing child-zone fixture with its required holder traversal identity.
