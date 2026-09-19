# Original provider cleanup

The user authorized the compatibility-boundary review fixes. This change removes the reviewed gravity/easing approximations and restores the canonical source owners for Mario's state lifecycle. It does not change authored actor decisions or stage routing.

## Production changes

- `LiveActor::movement` again calls `MR::isCalcGravity` and `MR::calcGravity` at the original point, before the original dead-actor return. Removed `update_live_actor_gravity` and its host `sqrt` renormalization, finite-value gate, different cutoff, and extra dead-actor gate. The velocity/binder helper in the same compatibility file is outside this bounded numerical change.
- `onCalcGravity` and `offCalcGravity` use the exact donor bodies. A live actor's immediate query precedes setting the calculation flag; a dead actor skips that immediate query but still receives the flag. `calcGravity` already stores the original manager result and preserves prior gravity for a near-zero result.
- `calcNerveEaseInRate` uses the exact donor call to `getEaseInValue(calcNerveRate(...), ...)`, restoring shared JMath table behavior instead of host `std::cos`.
- `Game/Player/MarioState.cpp` is now byte-identical to the canonical TU and is compiled normally. Deleted both MarioState compatibility providers. Returned the six base defaults to their canonical `MarioWait.cpp`, `MarioActorSpecialDraw.cpp`, and `Mario.cpp` owners; these empty/default bodies are original behavior.
- Removed the duplicate compatibility definition of `connectToSceneCrystal`; the canonical `Game/Util/ObjUtil.cpp` definition remains.
- Removed the unreachable recursive flag evaluator from `StorySequencePlatformCompat`. Active exported queries still use the original GameDataHolder/checker. No flag or progression outcome was changed.

`source-equivalence.json` checks the complete MarioState TU and the nine restored helper/default function bodies. No identifier, literal, statement, or API-call rewriting is permitted by that comparison.

## Focused numerical and semantic probe

`tests/OriginalActorUtilityTests.cpp` uses the original GameSystem scene controller, scene executor, SceneObjHolder, PlanetGravityManager, and ClippingDirector. A small test gravity source exercises the original manager. It checks 128 manager output vectors bit-for-bit, calculation-flag ordering, dead/live movement phase behavior, zero/near-zero/no-field retention, and 27 nerve easing samples including clamping and nonpositive durations. A test actor ends its own control phase to avoid running unrelated model/audio/shadow work; it does not replace gravity calculations or any original scene owner.

`isolated_probe.py` compiles only this test object and links pre-existing archives. It records the archive hashes and exact commands so the baseline can be preserved independently of the concurrent production cleanup. It does not invoke a shared Xmake build or replace the game executable. It also restores the Abseil archive paths recorded in the baseline compile database, because those existing Aurora objects still depend on that package while the working build graph is changing.

Initial attempts are preserved: `baseline` missed MSL math declarations in the test (fixed by its compatibility include); `baseline2` omitted the baseline's recorded Abseil libraries; `baseline3` linked but its fixture omitted the actual ClippingDirector required by actor registration. The debugger confirmed null `ClippingDirector::registerActor`; that test process is gone. These are fixture/tool invocation failures, not successful numerical red results. The corrected fixture creates the real clipping owner rather than weakening registration.

The corrected `baseline4` compile and link passed against the unchanged baseline archives. Its test exited 1 with four intended regression failures:

| Check | Baseline result |
| --- | --- |
| Immediate gravity query before flag assignment | Fails: callback observes the flag already set |
| Exact manager output storage | Fails: 65 of 128 vectors differ in float bits |
| Gravity phase before dead-actor movement return | Fails: prior gravity remains instead of the manager result |
| Original JMath easing wrapper | Fails: 5 of 27 samples differ in float bits |

No-field, inactive-field, zero/near-zero, disabled-calculation, and dead-actor immediate-setter cases pass in that same run. This is a genuine before-fix execution result, distinct from the earlier setup failures. The process returned normally with test status 1 and is gone. See `baseline4.json` and `baseline4-run.log`; the former records library hashes and invocation details. The run uses no disc, renderer, or game-loop simulation.

Green execution against the rebuilt production archive passed (`green-focused-results.json`): **0/128 gravity vector bit mismatches and 0/27 easing bit mismatches**, with all setter ordering, dead/live movement phase, disabled and absent-field checks passing. The new generic original-placement coverage test also passed. Both test executables returned 0 normally.

## Shared build and ownership validation

The first normal build exposed two mechanical consequences of the larger runtime cleanup. `OriginalGameApplication.cpp` needed its own `RuntimeServices.hpp` include for the complete DVD/save service types. `RuntimeContext.cpp` contained a local variable collision with its guest execution scope and an orphaned duplicate retirement tail; the local scene-service name was made explicit and the unreachable duplicate tail removed. The earlier failed `green1`/`green2` logs remain. Their JSON executable hashes describe the **old executable left on disk after failure**, not a newly successful artifact.

The Game and app static archives were retired before rebuilding so removed source members could not survive incrementally (`archive-retirement.json`). `green3` built the main executable and both focused tests. Its main SHA256 was `92a09cc0d5d0a6a2d56f849ccba3c63a6bb1916df6e51c4e614f51454b92eb2b`. `restored-archive-owners.json` verifies the selected removed MarioState providers are absent and the Crystal wrapper comes from canonical ObjUtil; it does **not** claim archive-wide uniqueness.

The parallel archive-wide audit subsequently found other duplicate providers. After the other agent removed them, the Game archive was retired again (`duplicate-archive-retirement.json`) and all final targets linked successfully. `finalowners-build-results.json` records the final main SHA256 `6fd4824a3217cb18bb7f6533936202a4854c6475a7fbef895176a2ba91ffbb43` and the resource/player probe binaries. The actual-process player and main runtime smokes are owned by the parent and sibling agents; their evidence is separate from these standalone tests.

All nine requested ownership/lifecycle suites linked. Execution results are preserved in `ownership-run-results.json`; these do **not** all pass:

| Standalone suite | Result and evidence boundary |
| --- | --- |
| Original scene execution owner | PASS: deferred categories, callback retirement, 16 scene domains, layout adaptor and model visibility |
| Original JPA manager, particle resource owner, AutoEffect metadata | Initially missing explicit language owner. After a narrow test-only language binding, each still fails before its assertions because the original FileLoader process owner is absent. |
| Original effect, image-effect and full layout-group | SIGSEGV during standalone RuntimeContext message-resource construction; each debugger capture finds `FileLoader::getRequestFileInfoConst(this=null)` for the real message archive. No missing-owner fallback was added. |
| Layout real-or-absent | Initially missing explicit language owner; after that fixture correction, aborts because the original SC system configuration owner is absent. |
| StorySequence real-or-absent | Exit 77: two checks pass, full story movement is unavailable without its original GameSystem scene controller. This is a prerequisite skip, not a green full-story test. |

The four narrow resource-language fixture fixes are the sibling agent's changes; `fixed-owner-run-results.json` and logs preserve their remaining failures. The JPA debugger stack in `debugger-resource-file-owner.log` confirms the original path `ParticleResourceHolder -> MR::mountArchive -> FileLoader::requestMountArchive` with a null process owner. That inferior was explicitly killed; all diagnostic processes are gone. The three renderer debugger logs capture the same absent file owner. These old standalone fixtures need adaptation to established original-process ownership; this checkpoint does not assemble another alternate process bootstrap or weaken production guards. Full JPA iteration, standalone effect lifetime rollback and standalone image/layout renderer assertions therefore remain **unvalidated in this checkpoint**.

A search of current `src/` and `tests/` found no references to the deleted `StageInitializationService`. The replacement placement-coverage fixture exercises the generic strict boundary rather than restoring the deleted scene-specific route.

## MapUtil consolidation assessment

The exact `OriginalMapQueries` import remains deliberately unchanged. Full canonical `MapUtil.cpp` consolidation is currently blocked by genuine API closure work:

1. `MR::isFallNextMove(const TVec3f&, const TVec3f&, const TVec3f&, f32, f32, f32, const TriangleFilterBase*)` remains undecompiled. The canonical actor overload forwards to it. Retail assembly is available in `decomp/build/RMGK01/asm/Game/Util/MapUtil.s:1094-1195`.
2. `CollisionCategorizedKeeper::checkStrikePoint`, `checkStrikeBall`, and `createAreaPolygonList` have declarations but no original/native definitions. Canonical MapUtil directly requires them; `createAreaPolygonListArray` is present and is a different API.
3. Current native area/sphere query implementations in `GameMapCollisionCompat.cpp` need to be replaced at that shared keeper boundary before switching those MapUtil wrappers. Dozens of code-accessor predicates and two binder predicates also overlap that TU, plus `calcVelocityMovingPoint` in `OriginalCollisionGeometry.cpp`. The complete switch must remove all overlapping symbol owners together.

The parent approved retaining the exact query provider for this checkpoint. Merely enabling the TU and relying on dead stripping would not prove the missing APIs, and renaming copied functions would not consolidate the original owner. No successful stub, actor-specific fallback, or partial MapUtil switch was added. Recover the missing methods in decomp first and validate general sphere/point/area query semantics before completing that separate restoration.

## Scope

No decomp edits, parent commits, or index operations were made by this agent. The parent coordinates shared target builds and publication. `before/` and local test binaries/objects are scratch evidence, not curated source deliverables.
