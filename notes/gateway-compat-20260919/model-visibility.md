# Original model visibility and deferred draw membership

## Evidence

`SceneDrawBufferService` owns lifetimes but delegates membership to the original
`NameObjExecuteHolder`, `DrawBufferGroup`, and `DrawBufferExecuter` implementations.
`DrawBufferShapeDrawer::draw` draws the active packet list; it does not inspect
`LiveActor::mIsHiddenModel` on every draw. This matches the original architecture.

The compatibility implementations of `MR::onEntryDrawBuffer` and
`MR::offEntryDrawBuffer` only changed that boolean. Original
`decomp/src/Game/Util/LiveActorUtil.cpp` lines 2042–2064 instead queue temporary
connection/disconnection for live, unclipped actors before changing the flag.
Consequently a model hidden after its original connection remained in the actual
scene draw lists. This is a generalized cause of unwanted secondary model draws.

`MR::showModel` and `MR::hideModel` also omitted their original animation/view
calculation state changes. `MR::hideModelAndOnCalcAnim` in the LOD compatibility
unit set flags directly, bypassing the same original queue.

## Changes

- Restore original visibility and animation/view state transitions in
  `src/compat/LiveActorUtilCompat.cpp`, including the previously absent
  `MR::isNoCalcView` implementation.
- Have `MR::hideModelAndOnCalcAnim` use the original `hideModel`/`onCalcAnim`
  sequence, so LOD and attached models use the same membership authority.
- Extend `OriginalSceneExecutionOwnerTests.cpp` with a real original execution
  list fixture. It checks draw callback suppression, deferred changes,
  initialization-time hiding, repeated calls, dead/clipped actors, entry-only
  visibility, hidden animated models, and same-phase hide/show cancellation.
- No Game source changes and no actor/model-name conditions.

## Validation

`git diff --check` passed for the three edited source/test files. Root agent owns
serialized builds and runtime reproduction; test execution and screenshot
comparison are pending. The visual duplicate-Mario symptom is a hypothesis until
that comparison confirms it.

## Test ownership corrections

The first test run exposed stale test scaffolding: SceneObjHolderBinding now
requires the original GameSystem scene controller. The updated test constructs
the real GameSystem, GameSystemSceneController and Scene and publishes the real
fixture executor/SceneObjHolder through that Scene. The original controller's
NameObjHolder backs all scene lookups and movement flag synchronization.
`OriginalSceneControllerFixture.hpp` shares this constructor/lifetime setup with
the focused gravity query checks; production owner checks are unchanged.

LLDB then identified a second missing original prerequisite: LiveActor
construction registers with ClippingDirector. `visibility-test-debug.log`
records the null ClippingDirector stack, and the fixture now creates the real
SceneObj_ClippingDirector before the test actor.

Gravity query coverage uses `smg-pc-gravity-real-or-absent-tests --queries-only`.
This runs only the absent-manager and real-manager query cases, with original
process and scene owners. The existing broader placement/follower tests remain
available under the default invocation and are not claimed by this focused run.

## Validation results

- `smg-pc-original-scene-execution-owner-tests`: PASS, including 16 original
  scene generations, deferred membership/callback checks, and every added model
  visibility case. Root-owned run: `visibility-test3.log`.
- `smg-pc-gravity-real-or-absent-tests --queries-only`: PASS, both the
  absent-manager and real-manager query cases. Root-owned run: `gravity-test.log`.
- These tests validate scene membership and gravity queries. The broader legacy
  gravity placement/follower suite is outside the focused query claim.
