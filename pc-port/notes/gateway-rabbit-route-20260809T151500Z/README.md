# Gateway rabbit route checkpoint

Date: 2026-08-09 UTC

## Stable base

- Merged live `upstream/master` (`711968a8f`) into `pcp-aurora` as `8063dc0a8` and pushed it.
- Committed and pushed Aurora tagged depth snapshots and GX display filtering as `87849d1` on `pcp-general-pcm-voices`.
- Committed and pushed the authored title, File Select, Gateway sky/air/planet/lighting/BrightSun milestone as parent commit `ed43d0745`.
- The final serialized visual matrix was green before this rabbit tranche. The known user-owned SaveIcon, TriggerChecker, and RFL changes remain outside both commits.

## Exact authored chase facts

- The four `RunawayRabbit` child placements represent **three** completion groups, not four catches.
- Their `Obj_arg0` values are `{1, 2, 0, 1}` and their switch IDs are `{1112, 1113, 1114, 1118}`.
- Group 1 has two alternate hide locations. `RunawayRabbitCollect::noticeAppearRabbit` disables the other rabbit in the same group, and completion counts unique nonnegative group IDs.
- The production route therefore completes after three catches.
- To enter the guide/bunny sequence without replaying the Star Festival, the selected-file story boundary should be progress 5 (after Peach's castle rises), with the Tico guide completion event still unset. The existing progress-10 spin checkpoint remains a verification slice, not the stage-owned save state.

## Ranked generalized closure

1. Activate exact `DemoRabbit` as a vertical NPC/runtime proof.
   - Real SearchTurtle attribute-group registration through `GroupCheckManager`.
   - Real actor shadow-controller ownership/query.
   - Real BMG/FLW talk-node traversal and demo pause/resume. DemoRabbit's exact flow key is `HeavensDoorMysteriousZone_DemoRabbit000`.
2. Activate exact SwitchArea/EarthenPipe behavior that controls authored hide locations and chase gating.
3. Activate exact `RunawayRabbitCollect` and `RunawayRabbit`, including reusable Walker runaway/blow-damage states, marks/footprints, catch camera, talk, and child-placement ownership.
4. Activate exact `RunawayTico`/Tico guide flow with persistent event flags and animation-camera support.
5. Activate Rosetta and the exact Heavens Door demo once the shared joint-controller and demo-talk paths are real.

## Architecture boundary

The route must not add Gateway-name conditions to compatibility or rendering code. The intended shared seam is scene-owned authored-stage data plus one placement instantiator that:

- keeps resolved placement rows and `JMapInfoIter` storage alive;
- owns one `DemoSceneRuntime` for the whole scene;
- preloads archive lists through the factory;
- constructs supported actors through `NameObjLifecycleService` in retail phase order;
- runs one `initAfterPlacement` pass;
- reports unsupported active rows explicitly until the production strict preflight reaches zero blockers.

The title-selected `GameDataHolder` should eventually outlive the title and stage scenes. Gateway's current checkpoint-local progress holder must not become that session owner.

## DemoRabbit message-flow evidence

RMGK01 `Message.arc` resolves the exact flow key to message index 824. Its graph is:

```text
node 267: message 824, FlowTalk sentinel
node 268: message 825, event talk, camera type 2
node 269: message 826, short talk
node 270: message 827, event talk
end
```

`TalkNodeCtrl::createFlowNodeDirect` skips the FlowTalk sentinel, so the initial current message is 825. The exact actor then uses the same controller as follows:

- demo part 4 presents 825 and advances the controller to 826;
- the free guide/wait path can present 826 as the nearby short hint;
- demo part 8 advances first, then presents 827;
- re-entry after the Tico-guide event flag advances 825 to 826.

This flow needs no branch node itself, but the Talk runtime must retain general branch/event traversal because `RunawayTico` and the caught-rabbit talk paths use it immediately afterward. The scene-owned runtime must arbitrate controls, advance only on fresh input, and pause/resume time-keep demos at the real talk boundary; it must not auto-dismiss or store process-global state.

## Current work split

- General GroupCheckManager/SearchTurtle and actor-shadow ownership is source-frozen and green. Membership uses the retail name-keyed lookup, requires a pre-created scene owner, and the shadow list remains externally owned without widening `LiveActor`.
- Exact rabbit/Talk runtime implementation is in progress. MessageService now retains the parsed FLW/FLI graph and message-index mapping, while the scene-owned Talk runtime/controller rewrite is still deliberately factory-gated.
- The authored-stage placement/session seam is green in `StageAuthoredData` and `AuthoredPlacementInstantiator`; integration into StageHost/GatewayDemoScene is now source-only and serialized.

## Route and lifecycle audit

The existing sequence code already has the correct generic transition. `SceneTransitionRequestService` turns `MR::requestChangeStageInGameAfterLoadingGameData()` into retail StorySequence move type 6. At story progress 5 (`ピーチ城浮上後`), `StorySequenceExecutor::overwriteGalaxyNameAfterLoading` selects `HeavensDoorGalaxy`, scenario 1, StartInfo 0. Progress 5 is therefore the selected blank-file seed: progress 0 replays the prologue/Peach Castle path, while the checkpoint's progress 10 incorrectly marks `チコガイドデモ終了` complete and skips the chase.

The selected-file `GameDataHolder` and its `ScopedGameDataHolderOverride` must be owned above both title/file-select and the stage scene. The current `GatewaySpinCheckpoint` cannot be that owner: its constructor creates a private holder, forces progress 10, and teleports Mario to `MarioDemoPos2`. Integration needs a mode that borrows the route/session holder and the StageHost-owned `DemoSceneRuntime`, performs no story mutation or teleport, and arms only at the authored part-14 suspend boundary. Its temporary Tico/Rosetta casts must disappear once the exact placed actors own those demo memberships.

`StageHostScene` is already the correct production owner for session binding, required SceneObjs, stage light, collision, one demo runtime, retail placement phase order, StartInfo Mario, post-placement initialization, camera, and audio. The missing general ownership surface is:

- retained placement tables, placement rows, general positions, and selected StartInfo in one scene-lifetime data owner;
- one reusable placement-instantiation pass shared by StageHost and bounded development scenes;
- an optional scene-owned extension/companion factory invoked after placement and destroyed before the demo runtime and actor roots it borrows;
- a selected-file game-data session above scene changes.

The StageHost destructor currently drops its demo runtime before roots. Any route companion that references the demo runtime must be explicitly destroyed first; merely appending it to `_roots` is unsafe. `SceneLifecycleService` currently exposes only generic `Scene*`, root, and stage identity, so an externally attached checkpoint would also have an unsafe raw lifetime. Prefer a StageHost-owned generic extension factory rather than a Gateway-aware lifecycle getter.

## Shared authored-placement seam (source-ready, not integrated)

New disjoint files:

- `src/scene/StageAuthoredData.{hpp,cpp}` owns resolved placement tables, copied placement/JMap rows, general positions, and selected StartInfo. It returns placement/start contexts whose iterators point into that stable owner.
- `src/scene/AuthoredPlacementInstantiator.{hpp,cpp}` classifies every active row, preserves the retail common/layer and high-priority-table phase order, forwards each exact placement iterator to archive preload and `NameObjLifecycleService::construct_and_init`, owns the actors, and exposes a separate single `initAfterPlacement` pass and explicit teardown.
- `tests/AuthoredPlacementInstantiatorTests.cpp` is a synthetic contract test for strict non-mutating rejection, development-only supported-subset reporting, retail order, retained JMap identity, archive evidence, post-placement ordering, teardown, and partial-failure cleanup. It is wired as the isolated `smg-pc-authored-placement-instantiator-tests` target.

The seam contains no stage-name or object-name decisions. Production uses `Strict`: any nonignored blocked row rejects before archive or actor mutation. `SupportedSubsetForDevelopment` is named and documented as a bounded fixture mode only; it reports blocked and ignored rows while constructing ready rows through the same lifecycle. Actor display names returned by a caller are copied into stable report storage before construction because the retail object may retain the supplied pointer.

Teardown and partial-construction rollback run in reverse construction order. Later actors can retain non-owning child/group/demo/follow references to earlier peers, so forward destruction is not a safe generalized scene-ownership boundary. The synthetic contract covers both a normal four-actor teardown and rollback after two actors have been created.

GatewayDemoScene can later replace its three creation policies (complete AreaObj loop, `scene_visual_kind` loop, and separate point-gravity creator) with this one instantiator. Its exact planet/sky/gravity/start queries remain evidence assertions only, never creation policy. StageHost can replace its private duplicated phase/preflight/construct loops with the same seam while retaining strict zero-blocker behavior.

## Exact production blockers found by the audit

The most recent Gateway scenario report still has 242 actor-bearing rows: 47 complete, 187 blocked, and 8 proven non-actor helpers. Production StageHost must not gain a skip/allowlist to get around those rows. Route-critical blocked roots include Tico, three DemoRabbit rows, RunawayRabbitCollect (with seven retained child rows), Rosetta, TicoBaby, and both HeavensDoor step actors.

Factory archive discovery also needs the retail placement-aware callback table rather than only fixed archive records: DemoRabbit chooses TrickRabbitBaby by CastId 0, Tico/TicoBaby selects baby versus Tico/Middle/Low by object name, RunawayTico selects by arg0, and Rosetta adds Rosetta/Middle/Low plus its argument-selected LightDome/DomeHalo closure. Creator and archive callback support must be enabled atomically only after each actor's runtime closure is real.

The chase is not only an NPC closure. `RunawayRabbit::attackSensor` catches only when its `Catch` sensor overlaps a player sensor. The scheduler has general pair overlap dispatch, but the current Gateway Mario walk slice omits `MarioActorSensor.cpp` and its PC initialization skips `setupSensors()`. Mario therefore needs the production player-sensor slice, and the showcase-only Mario source closure must move into the production game/factory target before StageHost can construct StartInfo `Mario`.

The remaining reusable compatibility gaps are ordinary engine services, not Gateway cases: talk graph/request/branch progression, NPC joint and behavior controllers, group arrays, shadows, base-matrix follow, walker runaway/blow-damage state, actor/stage cameras, sub-BGM state, and Camera/Sound demo-row dispatch. The exact TicoGuide time sheet already provides the route: parts 0-10 introduce/start the chase, parts 11-14 lead to the high tower, part 14 is a suspend boundary, and the existing spin checkpoint begins at part 15.

## Focused verification

- `xmake build -j2 smg-pc-authored-placement-instantiator-tests`: pass.
- `xmake run smg-pc-authored-placement-instantiator-tests`: pass, `[ok] shared authored placement instantiator contract passed`.

## SwitchArea source checkpoint

- Added byte-identical PC mirrors of `SwitchArea.hpp/.cpp`.
- Registered `SwitchCube`, `SwitchSphere`, and `SwitchCylinder` through one generic AreaObj descriptor table and the retail order-0 `SwitchArea` manager (`0x40` capacity).
- No stage name or rabbit switch ID is present in the implementation.
- The first focused AreaObj build was deliberately stopped before this code was reached because the concurrent GroupCheckManager tranche had not yet copied its exact `Game/Player/GroupChecker.hpp` header. No workaround was added.
- After that shared source boundary closed, `smg-pc-area-obj-real-or-absent-tests` rebuilt and passed all 13 cases against real RMGK01. The two MysteriousZone rows prove the shared B gate blocks activation, each authored A switch turns on only after entry with B enabled, and `Obj_arg0=-1` keeps the switch latched after exit.

## Green generalized substrate gates

Run serially at `-j2` from `pc-port/`:

- NPCActor group ownership: 5/5.
- Actor shadow/controller physics: 8/8 (including the final exact `水面丸影` literal rebuild).
- ActorRuntimeRegistry: 5/5.
- SceneObjHolder: 7/7.
- AuthoredPlacementInstantiator: strict/subset/order/rollback contract passed.
- AreaObj/SwitchArea real-disc suite: 13/13.

These are subsystem gates only. `DemoRabbit` remains unavailable in the factory until the scene-owned Talk graph/request/pause lifecycle is green; no partial actor activation is being hidden behind these passes.
