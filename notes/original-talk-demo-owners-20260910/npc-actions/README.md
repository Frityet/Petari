# Original NPC action/reaction recovery — 2026-09-10

The complete current reference `NPCUtil.cpp` and header were copied byte-identically into native `src/Game/Util/` after reference recovery. Fourteen missing shared MR helpers were recovered first in `decomp/`. No actor-specific workaround, native model substitute, forced reaction result, or conditional Game body was added.

Source is frozen. This subtask performed no root Xmake build, commit, push, or linked gameplay run. Parent integration owns duplicate removal, linking, and runtime validation. The four pre-existing user-dirty paths were untouched.

## Evidence

- Baseline complete Wii TU compile: exit 0.
- Final complete Wii TU compile: exit 0.
- All fourteen newly recovered MR functions: 97.846535–100% objdiff against the original object.
- The corresponding 2,696 bytes in the saved retail assembly independently match the actual RMGK01 `main.dol`; see `retail-byte-proof.json` for every address and the DOL SHA-1.
- Final complete native source compile with LLVM 23: exit 0. This is isolated compile evidence, not a linked or executed NPC test. Parent's ordinary Game compiler rule remains responsible for CP932 literal encoding.
- `source-manifest.json` records identical source and header hashes across reference/native. `wii-results.json` and `native-compile.json` retain exact commands and exit codes. `objdiff-before.json.gz`, `objdiff-recovered.json.gz`, and `objdiff-summary.json` preserve baseline and final scores. The summary also includes a newly emitted SDK inline subtraction; that is not counted among the fourteen recovered MR APIs.

| Recovered MR function | Retail bytes | Match |
| --- | ---: | ---: |
| decidePose | 180 | 100% |
| followRailPose | 152 | 99.47369% |
| followRailPoseOnGround(NPCActor*, float) | 8 | 100% |
| followRailPoseOnGround(NPCActor*, const LiveActor*, float) | 252 | 99.7619% |
| isActionLoopedOrStopped | 76 | 100% |
| startMoveAction | 148 | 100% |
| tryStartTalkAction | 168 | 100% |
| tryStartMoveTalkAction | 344 | 99.88372% |
| tryStartTurnAction | 228 | 100% |
| tryStartReaction | 808 | 97.846535% |
| tryTalkNearPlayerAndStartTalkAction | 52 | 100% |
| tryTalkNearPlayerAtEndAndStartMoveTalkAction | 52 | 100% |
| tryStartReactionAndPushNerve | 88 | 100% |
| tryStartReactionAndPopNerve | 140 | 100% |

The non-perfect rail/move-talk differences are constant-pool mapping and equivalent register assignment. `tryStartReaction` has eight extra bytes around an early return from the active-action exclusion and a different shared epilogue order. Its predicate evaluation order, animation calls, and return values agree with retail. No score-driven changes were made after passing the guide's functional/high-fuzzy requirement.

`decidePose` has a `NO_INLINE` declaration, retaining the observed retail call from `followRailPose`. The ground helper uses the existing decomp `#pragma dont_inline` convention to preserve its retail vector constructor/scale/subtraction calls. These affect compilation, not the Game algorithm. The small anonymous inline reaction predicates express the repeated original old/current event comparisons and reproduce their compiled boolean return values.

## Original contracts retained

- Reaction selection is gated by `_128`, then prioritizes new trample, new hit/reaction, new spin, and current pointing. The pairs are respectively `!_DD && _E2`, `!_E0 && _E5`, `!_DE && _E3`, and pointing `_E4`.
- New trample or hit stops BCK and starts the selected action, returning true. Spin and newly started pointing use `tryStartAction`. Ongoing pointing does not interrupt the trample/reaction/spin action names. It may restart a finished/looped pointing action without reporting a newly started reaction.
- An absent or empty selected action can still report a reaction edge when both actual scale-controller and joint-delegator pointers exist. That fallback is present in retail and does not fabricate either object.
- The push wrapper pushes only on a true reaction result. The pop wrapper re-pushes the just-popped current nerve when a new reaction starts, keeps it during scale animation, and pops it only after the original animation completion predicate succeeds.
- `isActionLoopedOrStopped` checks BCK stopped for frame-controller attribute zero and BCK looped for every nonzero attribute. It does not substitute one predicate for both modes.
- Talk-action selection follows original turn flags and turn completion. The wrapper always tries the action before calling the talk request, and returns the talk request's result.
- Move-talk follows an existing rail only. A long active talk turns in place; short talk can retain rail motion and apply `_118` as its BCK rate. The rate is set even if `tryStartAction` reports the same action was already active. Missing/empty action names return false without setting that rate.
- Rail movement adjusts speed, advances the real rail rider, chooses ground or free pose, and reverses direction at the goal in that order.
- Ground following starts with the rail position, copies actor gravity, and casts from `railPosition - gravity * 10` along `gravity * 1000`. It deliberately ignores the hit boolean: the initialized rail position remains the fallback when there is no hit. It then uses negative gravity as up, the rail direction as front, and the original position/orientation blending rates.
- `decidePose` first blends position. It creates orientation directly only when both up/front rates equal exactly one; otherwise it calls the original quaternion blend helper.

## Integration boundary

`duplicate-providers.json` lists exactly five definitions to remove from `src/compat/NPCActorRuntimeCompat.cpp`:

- `tryStartTurnAction`
- `tryStartReactionAndPushNerve`
- `tryStartReactionAndPopNerve`
- `tryTalkNearPlayerAndStartTalkAction`
- `tryTalkNearPlayerAtEndAndStartMoveTalkAction`

Their shared `throwNPCBehaviorUnavailable` helper becomes unused. No other duplicate native definitions were found for this recovery. Existing native NPC item, placement, float, and goods providers remain outside this cohort.

The whole reference TU also contains its pre-existing TakeOutStar/FadeStarter/DemoStarter bodies. The first isolated native probe correctly failed for three absent ObjUtil declarations. Parent added the genuine declarations from reference ObjUtil.hpp for `createPowerStarDemoModel`, `appearPowerStarContinueCurrentDemo`, and `isEndPowerStarAppearDemo`; the complete native compile then passed. Their native definitions were absent at this audit. A parent link must determine whether the currently unused star-demo path is removed by normal dead stripping or requires its actual provider closure. No stub was added here, and no complete power-star/demo owner claim is made.

An optional independent review was requested from the depth agent, but it was deferred because that agent was already recovering the demo owner cohort. The branch audit here is this subtask's own retail comparison, not an independent second-review claim.

## Focused fixture extension

Parent subsequently authorized tests-only runtime coverage. `tests/NPCActorRealOrAbsentTests.cpp` extends its existing real Tico model group; the six top-level groups remain counted as before. Isolated LLVM23 compilation passes (exit 0), recorded in `fixture-compile.json`/`.log`; `fixture-manifest.json` and `fixture.patch.gz` identify the exact test source. Linked execution remains parent-owned and was not performed by this subtask.

The new reaction cases select four distinct actual BCK names from the loaded Tico ResourceHolder and require every one to exist. They check trample/hit/spin/pointing priority, protection of an active spin from pointing, initial versus ongoing pointing return values, and one-time versus loop completion bits in the actual active Xanime frame controller. A real AnimScaleController and real Body JointControlDelegator exercise the scale-only fallback and its requirement for both objects. Real probe Nerves run through the actor's original Spine to check push, reaction restart, scale-deformation hold, and exact saved-base restoration. Borrowed controller and nerve pointers are cleared/restored while their objects are still alive.

The talk/rail cases use a real registered TalkMessageCtrl with explicit talk-state inputs. They verify stationary action idempotence, empty action handling, no-rail delegation, long talk holding position, short talk retaining movement, configured moving-talk BCK rate even without a new action, and normal action/rate restoration after talk. The attached three-point BCSV is a 20-unit open line consumed by the actual RailRider/BezierRail, with speed adjustment, same-call advancement, quaternion orientation, and goal reversal assertions. This is synthetic authored test data with real Game owners, not a captured Gateway path or full dialogue run. The standalone fixture lacks MarioHolder and explicitly expects the original turn helper to report that missing owner; it makes no successful player-turn geometry claim.

This fixture audit also found five absent RailUtil providers: isExistRail, getRailCoordSpeed, adjustmentRailCoordSpeed, isRailReachedGoal, and reverseRailDirection. Parent addressed that separate integration frontier by copying the complete original RailUtil TU and removing the superseded GameRailCompat provider. This corrects the earlier report's incomplete dependency inventory. Clipping-curve sampling is not included here because it needs a separate actual clipping-owner fixture.

## Linked runtime confirmation

Parent subsequently rebuilt and ran `smg-pc-npc-actor-real-or-absent-tests` against the actual disc resources: all 6/6 fixture groups passed with unchanged assertions, including original reaction push/pop, forward rail goal reversal, and long/short talk actions. The linked run evidence is `../run-npc-tests.log.gz`. Rail correction was committed in decomp as `ec3fa56e7`; publication verification remains parent-owned. This is focused NPC/rail runtime coverage, not a completed Gateway bunny sequence.
