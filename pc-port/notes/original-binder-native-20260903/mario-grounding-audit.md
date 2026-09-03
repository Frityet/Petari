# Mario grounding after original Binder activation

Read-only audit on 2026-09-03. No production edits or native builds were made
for this audit. The preceding run with the first reaction correction failed
the existing release-ground assertion at frame 203; that result predates the
parent's subsequent whole original Binder import. It is not a measured result
for the newly imported Binder.

## What the current player loses at rest

The PC `Mario::update` at `pc-port/src/Game/Player/Mario.cpp:2026` clears its
velocity, takes `_1` directly from `Binder::isBindedGround`, executes the PC
`mainMove`, and supplies the PC-only grounding force. At low released speed it
sets `mWalkSpeed` to zero. After thirty grounded frames with no walking input
or residual speed, it deliberately supplies no inward motion. Its comments
describe retaining the previous native solver's idle contact shell.

The PC `MarioActor::movement` at `MarioActor.cpp:802` then resets Mario's
position, ground position, and shadow position to actor position. It replaces
ground/shadow with a Binder hit only while Binder reports a floor, and clears
`mMovementStates._1` otherwise. It never executes original floor-ray logic.
Consequently, a missing Binder contact becomes a missing gameplay ground
state immediately, with no separate floor search or retained shadow geometry.

That equivalence is not the original game's contract. Original
`Mario::update` at root `src/Game/Player/Mario.cpp:1353` first imports previous
Binder planes, then runs `updateGroundInfo` **before** input and action. That
phase computes the game's own ground state and positions independently of
the latest Binder contact list. `LiveActor::updateBinder` runs later, after
actor control, and the original actor movement does not overwrite Mario's
grounded flag and shadow position with the PC assignments described above.

The field named `mGravityGrounding` is initialized to 20 in the authored
constant tables, but the current root source has no reads of it. Its only
active algorithmic use in these sources is the PC update's `* 0.1` bias and
airborne fallback. This source audit does not establish that the original
binary never reads the field elsewhere; it establishes that copying that
host force into another stage would not restore the recovered original
movement algorithm.

## Original floor and position sequence

The following bodies already exist in root; availability does not imply
that every historical reconstruction has been independently verified against
RMGK01 during this audit.

| Phase / bodies | Source | State supplied at zero stick |
| --- | --- | --- |
| `updateBinderInfo` | `MarioCollision.cpp:1344` | Previous collision reactions, wall/roof/floor triangles, contact flags. Not the final ground decision. |
| `updateGroundInfo` | `Mario.cpp:1569` | Calls `checkMap`, then assigns `_1 = checkGround()` when enabled; updates camera polygon, floor/sound codes, sand/water/poison state. |
| `checkMap`, `calcShadowPos` | `MarioCollision.cpp:237,1290` | Gravity-directed floor rays and `_45C`, `mShadowPos`, `mVerticalSpeed`; the rays are not conditional on input speed. |
| `isUseSimpleGroundCheck`, `checkGroundOnSlope` | `MarioSlope.cpp:25,63` | Actual slope/lock/player-mode selection and a separate ground ray, including original position/velocity corrections. |
| `checkGround` | `MarioCollision.cpp:1596` | Three rays around a 50-unit forward basis, rotated by 120 degrees, with an optional fourth center ray. Ray start is 100 units against gravity and ray length 500. The fourth ray is explicitly selected for a grounded idle player. |
| `setGroundNorm`, `recordLastGround` | `Mario.cpp:494`, `MarioEnforce.cpp:328` | Ground basis and collision-part-local ground position, with the real triangle and inverse matrix. |
| `checkForceGrounding` | `Mario.cpp:507` | Reconciles selected gravity/floor basis, shadow distance, and vertical velocity; it is not an unconditional gravity impulse. |
| `writeBackPhyisicalVector` | `Mario.cpp:1217` | Original movement constraints followed by copying both Mario velocity and position to the actor, before LiveActor invokes Binder. |

`checkGround` can return true from its accepted floor hits or near-floor
vertical distance even when no Binder planes exist. The slope path likewise
has its own hit result. Therefore a future original-Mario integration must
test the actual Mario grounded state and its floor/shadow data. The Binder
contact-list contract should continue to be tested separately; requiring a
Binder floor on every idle frame would force a behavior the game does not
require. This is not a proposal to weaken the current failing test while the
PC substitute update is still active.

Original `checkForceGrounding` cannot simply be appended to today's PC
update: its inputs are produced by the skipped map/floor phases, including
the ground basis, shadow position, vertical distance, and draw/state flags.
The PC actor's post-Binder ground/shadow overwrite would discard those inputs
again on the next frame.

## Exact activation blockers

1. **Animation state is a live dependency of ordinary floor selection.**
   `isUseSimpleGroundCheck` ends its normal flat-ground path by calling
   `MarioAnimator::isLandingAnimationRun`. That function queries real
   `XanimePlayer` animation state through `MarioModule::isAnimationRun`.
   `checkGroundOnSlope` also queries a named authored animation. The original
   `Mario::getGravityVec` has a grounded hard-landing query as well.
   Current PC `MarioAnimator::init` leaves both players and its resource
   table null (`MarioAnimator.cpp:36`), and the active PC gravity getter
   returns only `mAirGravityVec`. Importing the floor bodies and using them
   ordinarily would reach a null player; replacing those queries with false
   would change game decisions. `checkGround`'s `isAnimationRun("Run", 0)`
   query is additionally conditional inside the original indexed wrapper on
   `_A6C[0]`, so that particular call is not an unconditional null dereference.

2. **The whole ground phase includes more than geometric rays.**
   `updateGroundInfo` also needs `updateCameraPolygon`/`setCameraPolygon`,
   `updateFloorCode`, existing `updateSoundCode`, and the original
   `updateOnSand`, `updateOnWater`, `updateOnPoison` behavior. Its landing
   transition can change animation. `checkGround` can call original
   `stopWalk`, which itself changes animation/effects. These should not be
   replaced with no-op or guessed predicate providers to obtain a link.

3. **The shadow filter is not constructed today.**
   Root Mario creates a `TriangleFilterDelegator<Mario>` for
   `Mario::isIgnoreTriangle`; PC constructor line 216 assigns `_458=nullptr`.
   The predicate already exists at root `MarioCollision.cpp:71` and rejects
   near-tangent triangle normals using the chosen game gravity. The general
   line-query provider already accepts a real `TriangleFilterBase`, so the
   original predicate and owned delegator can be restored once its owner
   lifecycle is supplied. Silently leaving this filter absent changes the
   floor search. `calcShadowPos` also uses real `Spine1` joint position in
   supported bound/hang cases; current `getRealPos` and joint query bodies
   already exist, without requiring a fake position fallback.

4. **Full update still needs real state objects and the broader caller.**
   The new base `MarioState` machinery does not construct Wall/Hang/Swim/Wait.
   Original stride reset touches Wall when grounded; timers touch Hang;
   action processing and jump/water entry require the other actual states.
   The original actor caller invokes `updateGravityVec(false,false)` before
   Mario update. The root gravity function has been recovered, but PC control
   still replaces that selection and basis update with its direct projection.
   Activating the full update also requires original action and writeback,
   rather than retaining the PC `mainMove` and post-movement assignments.

Available generic geometry prerequisites include both original line-query
signatures, `getFirstPolyOnLineBFast`, `getCameraPolyFast`, triangle geometry,
and filter support in `compat/GameMapCollisionCompat.cpp`. Collision-part
matrix ownership is still missing: native registered KCL has no actual
`CollisionParts` with current/previous matrices, and `Triangle::getBaseMtx`,
`getBaseInvMtx`, and `getPrevBaseMtx` have declarations but no native bodies.
The parent's new original force-movement delegation does not supply that
matrix lifecycle. These gaps and missing Mario/Xanime ownership remain
required dependencies.

## Next bounded task

The complete original floor pipeline is **not yet a small safe activation**
with current providers. Keep this as the next coherent player milestone after
real authored `XanimeResource`/`XanimePlayer` ownership is available. Then
verify/import the original floor group and its slope/last-ground helpers,
with original-compiler checks on the historical `MarioCollision` bodies,
before replacing the PC update/post-movement assignments together.

An independently preparable source boundary is the original shadow-query
pair `Mario::calcShadowPos` / `Mario::isIgnoreTriangle`, with the typed
delegator lifecycle and a fixture explicitly calling the actual methods on
the existing live Mario. It can test zero-velocity rays, slope/tangent
filtering and retained shadow metadata. Transformed-part fixtures additionally
require the real collision-part/matrix provider first. Such a provider would
be genuine preparation, not sufficient justification to
splice a new grounding rule into the PC update. No code for that follow-up
was added during this audit.
