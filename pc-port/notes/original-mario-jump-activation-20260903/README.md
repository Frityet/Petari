# Original Mario phase and jump activation frontier — 2026-09-03

This tranche identifies the next coherent original player subsystem and stages
its phase bodies. It does not activate them or claim working jumping. Parent
integration owns the native model/animator/audio providers and build lists.
No shared build, GPU run, actor-specific shortcut, new jump impulse, or native
movement edit was made here.

## The phase to restore

The smallest coherent **phase replacement** is original Actor initialization,
movement/control, Mario construction/update/actionMain/mainMove, and the existing
Jump/Collision/Gravity algorithms together. That phase is not yet a closed link
group. Restoring only `tryJump` leaves the active native loop bypassing it;
restoring only Actor movement dereferences owners the native initializer omits.

```text
MarioActor::movement
  LiveActor::movement
    MarioActor::control -> control2 -> controlMain -> updateBehavior
      updateGravityVec(false, false)
      tryJumpRush / tryThrow / tryStandardRush gates
      Mario::update
        stride + Binder import + relative ground + walls + original floor rays
        inputStick -> actionMain
          grounded: mainMove -> tryJump -> procJump(true) -> jumping flag
          airborne: procJump(false) -> doAirWalk / doLanding
        physical writeback -> timers -> extra services
    LiveActor Binder phase
  displacement/collision reconciliation -> animator update
  recordRelativePosition -> camera -> calcAnimInMovement -> front-vector history
```

`initForJump` is the already recovered four-instruction actor flag reset. It is
not the launch routine. `tryJump` calls `procJump(true)` before setting the jump
flag; preserving this order is required. Landing consumes the original floor
phase, so retire the current native Actor post-Binder ground/shadow overwrite
with the original phase rather than combining the two ground decisions.

## Exact staged methods

`stage-phases.py` copies complete method bodies literally from root into
`build/original-mario-jump-activation-20260903/staged/`, preserving required
includes and TU-local constants/macros. `phase-evidence.json` records each root
source line, full body, body SHA256, native compile command, and result.

The 23 isolated objects are:

| Root unit | Complete bodies staged |
| --- | --- |
| MarioActor | init2, movement, control, control2, controlMain, updateBehavior, calcAnimInMovement |
| MarioActorSensor | initForJump |
| MarioActorRushMsg | tryStandardRush |
| Mario | constructor, initAfterConst, update, actionMain, updateGroundInfo, updateTimers, updateAndClearStrideParameter, writeBackPhyisicalVector |
| MarioMove | mainMove |
| MarioJump | tryJump, initJumpParam, procJump, doAirWalk, doLanding |

These are staging/inspection objects, not additional production providers. At
activation replace the corresponding native branches with the root bodies;
do not add duplicate definitions beside those branches. Dependent helpers in
the same root units must also use their original implementations. Archive
symbol presence alone does not establish that.

## Owners that cannot be omitted

| Original owner/initialization | Ordinary consumer and implication |
| --- | --- |
| Full `Mario` constructor: 37 real movement states | Stride reset dereferences Wall, timers dereference Hang, launch calls Swim, and airborne code reads Swim state. Virtual lifecycle still needs each original concrete class. |
| `Mario::initAfterConst` | Calls Move::initAfter, Foo::init, Swim::init and resolves authored hip-drop animation strings. Current native suppression of these calls must retire. |
| Actual MarioAnimator, both original XanimePlayers and authored tables | Jump/air/landing query and change authored animations; the renderer's separate one-track object does not satisfy this owner. Parent handles this integration. |
| MarioEffect and its actual effect keeper | Actor updateEffect and original takeoff/landing invoke effect services. A compiled playEffect symbol alone is insufficient. |
| CollisionShadow (`_214`) and placed CollisionParts | Floor/shadow rays and relative motion require original Triangle owners with current/previous/inverse matrices, including moving-ground provenance. |
| Actor joint matrix array (`_C28`) and action/joint controllers | Original animation-in-movement and Bee-wing code use calculated joint data even in ordinary player mode. |
| Original `setupSensors`, MarioMessenger (`_1BC`) | Body/dummy sensors and rush/take message traversal need real HitSensor owners. |
| Original `initParts`, null animation, searchlight/throwing owners and attached parts | Original controlMain calls these update services every ordinary frame. Keep each method's own real mode/owner guards. |
| FixedPosition (`_498`, `_49C`), FootPrint, initialized frame buffers | Original initialization owns them; helpers query hand matrices and footprint resources. |
| Front-vector history `_F3CVec`, `_F40`, `_F42` | Original movement unconditionally writes the array and advances modulo `_F42`; original init2 allocates 30 vectors and initializes the ring size to 1. |
| Non-null disabled AudAnmSoundObject | Parent's general disabled audio boundary supports original BAS and ordinary sound calls without fabricating a JAI sound graph. |

The 37 state classes allocated by the original constructor are Flow, Wall,
Damage, Faint, Blown, Hang, Swim, Slider, FireDamage, FireRun, FireDance,
AbyssDamage, DarkDamage, Step, Bump, Paralyze, Stun, Crush, Freeze, Magic, FpView,
Recovery, Flip, SideStep, FrontStep, Stick, Rabbit, Sukekiyo, Bury, Wait, Climb,
Skate, Foo, Warp, Teresa, Talk, and Move (each with the `Mario` prefix).
Swim also creates MarineSnow, which loads a real texture. A subset of fake or
null state objects would not preserve this constructor or virtual behavior.

## Actionable original provider groups

`provider-frontier.py` measures undefined references of each isolated method,
not all symbols of a large original object. `provider-frontier.json` pairs each
reference with its callers, staged provider, current archive member, and root
definition candidates. The measured archives are hashed because other agents
are integrating concurrently. It is a direct frontier, not a transitive link
completion claim; inline/header providers and virtual dispatch require owner
inspection as well.

Work can be split along these original owners:

1. **Player floor and kinematics:** original MarioJump, MarioCollision,
   MarioSlope, MarioActorGravity, MarioEnforce, plus the original helpers in
   Mario/MarioMove. This supplies Binder import, map/floor/ceiling/wall queries,
   relative motion, launch, air steering, gravity and landing. Actor movement
   also calls MarioSpecial::isOnimasuBinderPressSkip. Retain original CollisionParts
   ownership in the placed-map provider.
2. **State and behavior closure:** all constructor-owned state classes and
   their real vtables, MarioState lifecycle, MarioSpin/Bee/Press, MarineSnow,
   damage/step/bump/slider/swim/warp/front-step checks called by update/actionMain.
   The earlier 13-unit compilation tranche is a useful starting set, not this
   complete closure.
3. **Actor initialization and post-update:** MarioActorMatrix/Morph/Parts/Sensor,
   MarioEffect, MarioShadow (CollisionShadow), MarioMessenger, MarioSearchLight,
   MarioActorSpecialDraw, FixedPosition/FootPrint and the parts they construct.
   The four missing ordinary hooks below must be recovered before using the
   intact original control/post-animation phase.
4. **Actor request routing:** MarioActorRushMsg/Rush/TakeMsg and real sensors.
   Ordinary updateBehavior calls their gate methods even if no rush succeeds.
   The recovered standard rush gate exposes two further missing root methods.
5. **Shared providers:** existing original CameraUtil declarations used by init2,
   first-person eligibility, MathUtil::diffAngleAbsHorizontal, the actual
   KarikariDirector query (already returns zero when that scene object is
   absent), and original animation/effect/audio wrappers. Use scene/service
   behavior already present in root; don't replace general queries with constants.

When full units replace extracts, retire overlapping definitions from
`MarioCameraAccessCompat.cpp`, `MarioStateAccessCompat.cpp`, and
`MarioStateCompat.cpp` as appropriate. For example Jump owns `isRising` and
full State owns the lifecycle/status providers. Keep a single implementation.

## Direct missing root methods

The following have no actual root body at this snapshot. The first four are
direct calls in the ordinary Actor control/post-animation chain. The rush
children are exposed by the newly recovered gate and required by its complete
linked method, even when a particular frame has no usable rush target.

| Method | Retail address / bytes | Caller |
| --- | --- | --- |
| MarioActor::updateTakingPosition | 0x802BC750 / 0x4F8 | controlMain |
| MarioActor::updateFairyStar | 0x802BD280 / 0x178 | controlMain |
| MarioActor::updateThrowVector | 0x802BD588 / 0x1F4 | updateBehavior |
| MarioActor::calcSpinEffect | 0x802C3820 / 0x148 | calcAnimInMovement |
| MarioActor::tryStartRush(bool) | 0x802C9094 / 0x1D0 | tryStandardRush |
| MarioActor::beginRush | 0x802BDFA0 / 0x1B4 | tryStandardRush |
| MarioModule::changeAnimationNonStop | 0x802E8F84 / 0x6C | init2 and tryJump |

`Mario::isDigitalJump` and `MR::getKarikariClingNum` do have root definitions.
`MarioActor::changeAnimationNonStop` is a separate actor wrapper, not the
missing MarioModule method. Avoid accidentally substituting that overload.
These are the measured direct gaps, not a promise that their children are
fully decompiled. The root sensor file also retains larger previously known
attackOrPushSensor/tryTornadoPull gaps; a full sensor activation needs its own
frontier rather than unrelated stubs.

## Root recovery and evidence

The only source change in this tranche is root
`src/Game/Player/MarioActorRushMsg.cpp`: `tryStandardRush` recovered before
copying its complete body into the native stage. It checks the actual debug
movement bit, respects requested/nearby rush selection and the original
`ACTMES_IS_RUSH_REQUEST` message, then performs the original sensor reset,
velocity zero and beginRush on successful tryStartRush. No outcome is forced.

`verify-rush.py` compiles the complete root source with GC/3.0a3 and configured
`cflags_game`, resolves REL24 and address-half relocations to known retail
symbols/data, and compares every byte against the supplied RMGK01 DOL.
All **65 fully relocated instructions** equal retail address 0x802C88B0,
size 0x104. Objdiff also reports 100%. Evidence is
`rush-retail-evidence.json`; generated object, logs and diff are under the
isolated build directory. DOL SHA1 is
`25c5959534b3c21246c6c7e42021b916b41fb578`.

All 23 literal staged methods compile on native arm64 using the configured
MarioMove compilation arguments and existing staged header closure. This is
compile evidence, not a link or runtime jump test. Existing Jump bodies were
not newly declared retail exact here: prior doAirWalk/updateCubeCode fuzzy
scores still require their stated scope. The newer
`original-mario-binder-info-20260903` recovery supersedes the old Binder audit
score and must be used for floor integration.

Reproduce independently from repository root:

```sh
python3 pc-port/notes/original-mario-jump-activation-20260903/verify-rush.py
python3 pc-port/notes/original-mario-jump-activation-20260903/stage-phases.py
python3 pc-port/notes/original-mario-jump-activation-20260903/provider-frontier.py
```

The later live gate should exercise original A-trigger rise, held versus
released apex, air steering, fall, landing and a second jump, with authored
animation transitions and floor ownership recorded. This tranche prepares
that activation; it does not use a partial runtime object to simulate it.
