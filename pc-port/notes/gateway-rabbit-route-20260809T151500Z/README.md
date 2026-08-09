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
- The generalized Talk/FLW source tranche is ready for focused verification. MessageService retains MessageId order and the parsed FLW/FLI graph; a scene-owned `TalkRuntime` now owns controller, callback, presentation, input-edge, and demo-pause state. Exact rabbit/factory activation remains deliberately gated on the focused Talk tests and SceneObj 0x19 composition.
- The authored-stage placement/session seam previously passed its focused gate. The current exact `PlacementInfoOrdered` ordering/lifetime tranche is source-only and awaits the serialized rerun recorded below.

## Route and lifecycle audit

The existing sequence code already has the correct generic transition. `SceneTransitionRequestService` turns `MR::requestChangeStageInGameAfterLoadingGameData()` into retail StorySequence move type 6. At story progress 5 (`ピーチ城浮上後`), `StorySequenceExecutor::overwriteGalaxyNameAfterLoading` selects `HeavensDoorGalaxy`, scenario 1, StartInfo 0. Progress 5 is therefore the selected blank-file seed: progress 0 replays the prologue/Peach Castle path, while the checkpoint's progress 10 incorrectly marks `チコガイドデモ終了` complete and skips the chase.

The selected-file `GameDataHolder` and its `ScopedGameDataHolderOverride` must be owned above both title/file-select and the stage scene. The current `GatewaySpinCheckpoint` cannot be that owner: its constructor creates a private holder, forces progress 10, and teleports Mario to `MarioDemoPos2`. Integration needs a mode that borrows the route/session holder and the StageHost-owned `DemoSceneRuntime`, performs no story mutation or teleport, and arms only at the authored part-14 suspend boundary. Its temporary Tico/Rosetta casts must disappear once the exact placed actors own those demo memberships.

This earlier audit identified the following general ownership surface; the authored-data and placement-instantiator portions are now integrated, while the selected-file and extension ownership remain future work:

- retained placement tables, placement rows, general positions, and selected StartInfo in one scene-lifetime data owner;
- one reusable placement-instantiation pass shared by StageHost and bounded development scenes;
- an optional scene-owned extension/companion factory invoked after placement and destroyed before the demo runtime and actor roots it borrows;
- a selected-file game-data session above scene changes.

StageHost now retires authored placement actors and StartInfo roots before dropping its demo runtime. Any future route companion that references the demo runtime must still be explicitly destroyed before that owner; merely appending it to `_roots` is unsafe. `SceneLifecycleService` currently exposes only generic `Scene*`, root, and stage identity, so an externally attached checkpoint would also have an unsafe raw lifetime. Prefer a StageHost-owned generic extension factory rather than a Gateway-aware lifecycle getter.

## Historical initial authored-placement seam checkpoint

New disjoint files:

- `src/scene/StageAuthoredData.{hpp,cpp}` owns resolved placement tables, copied placement/JMap rows, general positions, and selected StartInfo. It returns placement/start contexts whose iterators point into that stable owner.
- `src/scene/AuthoredPlacementInstantiator.{hpp,cpp}` classifies every active row, preserves the retail common/layer and high-priority-table phase order, forwards each exact placement iterator to archive preload and `NameObjLifecycleService::construct_and_init`, owns the actors, and exposes a separate single `initAfterPlacement` pass and explicit teardown.
- `tests/AuthoredPlacementInstantiatorTests.cpp` is a synthetic contract test for strict non-mutating rejection, development-only supported-subset reporting, retail order, retained JMap identity, archive evidence, post-placement ordering, teardown, and partial-failure cleanup. It is wired as the isolated `smg-pc-authored-placement-instantiator-tests` target.

The seam contains no stage-name or object-name decisions. Production uses `Strict`: any nonignored blocked row rejects before archive or actor mutation. `SupportedSubsetForDevelopment` is named and documented as a bounded fixture mode only; it reports blocked and ignored rows while constructing ready rows through the same lifecycle. Actor display names returned by a caller are copied into stable report storage before construction because the retail object may retain the supplied pointer.

Teardown and partial-construction rollback run in reverse construction order. Later actors can retain non-owning child/group/demo/follow references to earlier peers, so forward destruction is not a safe generalized scene-ownership boundary. The synthetic contract covers both a normal four-actor teardown and rollback after two actors have been created.

This replacement was subsequently completed: Gateway's former complete-AreaObj, `scene_visual_kind`, and separate point-gravity creation loops and StageHost's private placement loop now use the shared instantiator. Exact planet/sky/gravity/start queries remain evidence assertions only, never creation policy.

## Exact production blockers found by the audit

The most recent Gateway scenario report still has 242 actor-bearing rows: 47 complete, 187 blocked, and 8 proven non-actor helpers. Production StageHost must not gain a skip/allowlist to get around those rows. Route-critical blocked roots include Tico, three DemoRabbit rows, RunawayRabbitCollect (with seven retained child rows), Rosetta, TicoBaby, and both HeavensDoor step actors.

Factory archive discovery also needs the retail placement-aware callback table rather than only fixed archive records: DemoRabbit chooses TrickRabbitBaby by CastId 0, Tico/TicoBaby selects baby versus Tico/Middle/Low by object name, RunawayTico selects by arg0, and Rosetta adds Rosetta/Middle/Low plus its argument-selected LightDome/DomeHalo closure. Creator and archive callback support must be enabled atomically only after each actor's runtime closure is real.

The chase is not only an NPC closure. `RunawayRabbit::attackSensor` catches only when its `Catch` sensor overlaps a player sensor. The scheduler has general pair overlap dispatch, but the current Gateway Mario walk slice omits `MarioActorSensor.cpp` and its PC initialization skips `setupSensors()`. Mario therefore needs the production player-sensor slice, and the showcase-only Mario source closure must move into the production game/factory target before StageHost can construct StartInfo `Mario`.

The remaining reusable compatibility gaps are ordinary engine services, not Gateway cases: talk graph/request/branch progression, NPC joint and behavior controllers, group arrays, shadows, base-matrix follow, walker runaway/blow-damage state, actor/stage cameras, sub-BGM state, and Camera/Sound demo-row dispatch. The exact TicoGuide time sheet already provides the route: parts 0-10 introduce/start the chase, parts 11-14 lead to the high tower, part 14 is a suspend boundary, and the existing spin checkpoint begins at part 15.

## Earlier focused verification

These results predate the current exact five-pass/order/lifetime source tranche and must not be read as its final gate:

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

## Talk/FLW source checkpoint

The implementation boundary is generic and scene-scoped:

- `MessageService` retains MessageId table order and the full parsed `BmgFlowData`; root lookup scans FLW type-1 nodes for the resolved MessageId index, matching `MessageData::findNode`.
- `TalkRuntime` is a `NameObj` installed explicitly by `init()` at `MovementType_TalkDirector`. Its current-scene pointer is non-owning; graphs, actor-owned controllers, callback clones, arbitration, input state, and active presentation all live in the scene object.
- `TalkMessageCtrl` keeps the retail constructor requirement: it calls `MR::createSceneObj(SceneObj_TalkDirector)` and then registers with the active scene runtime. SceneObj 0x19 still needs to compose `new smgpc::compat::TalkRuntime()`; no fallback process owner is created.
- `TalkMessageInfo.hpp` and `TalkNodeCtrl.hpp` are byte-for-byte source diffs of the root headers. `TalkMessageFunc.hpp` retains the same declarations and vtable, with only the GNU `NO_INLINE` attribute moved before the return type because host GCC rejects the root template-definition suffix position. Because the callback base intentionally has no virtual destructor, the host owner reclaims the existing trivially-destructible `TalkMessageFuncM` clone storage without changing its Game vtable.
- The full `TalkUtil.hpp` traversal surface is implemented, including current/next branch choice, reset/forward helpers, state queries, AtEnd edges, callback registration, and message arguments. Branch/event routing remains data-driven; unsupported PowerStar/Astro providers fail explicitly rather than choosing a scene-specific result.
- A `TalkPresentation` snapshot captures the message at open time. Advancing the controller during an active talk therefore does not replace the displayed message: DemoRabbit can present 825, immediately advance to 826, and still finish the 825 presentation correctly.
- Event/normal talks require a release followed by a fresh A edge and never auto-dismiss. Short talk lifetime follows continued actor requests. Time-keep event talk pauses and resumes the active `DemoSceneRuntime` in TalkDirector-before-DemoDirector scheduler order.

Deferred general providers are explicit: programmable non-timekeep talk demos, talk camera types 0/1, recursive camera-row registration, sub-message extraction, and balloon rendering/position. None is touched by DemoRabbit's 825/826/827 flow (camera type 2); the retained presentation API is the renderer seam.

## Shared scene integration checkpoint (source-ready)

`StageHostScene` and `GatewayDemoScene` now have one placement creation policy: retained `StageAuthoredData` plus `AuthoredPlacementInstantiator`. StageHost selects `Strict`, performs the non-mutating placement and StartInfo audits, ranks and requests all five placement holders, constructs StartInfo Mario at the exact boundary, then constructs the planned placement groups. Gateway selects the explicitly bounded `SupportedSubsetForDevelopment`, retains every blocked row and reason in its public report, and creates every ready row through the same placement-aware `NameObjLifecycleService` adapter. Both invoke the seam's `initAfterPlacement` pass once and clear actors with lifecycle hooks and actual `unique_ptr` deletion in reverse construction order.

Gateway has no Mario owner in this development scene yet. It therefore calls the explicit preload API and immediately calls the construction API, with the missing player insertion documented as the development-scene exception. The DemoRabbit/player tranche must expose the prepared scene to its external Mario owner at that boundary and ensure placement actors retire before that Mario; it must not fold archive ranking back into actor construction.

Gateway's exact StartInfo, mysterious planet, child-zone gravity, and VROrbit queries are retained only as evidence pointers into `StageAuthoredData`. The old complete-AreaObj loop, `scene_visual_kind` creation loop, and manual `PointGravityCreator` path are removed. Visual-kind inspection now runs only after generic construction to populate the existing evidence view and assert exact Game actor types.

`SceneObj_GroupCheckManager` and the now-live `SceneObj_TalkDirector` provider are pre-created beside the other required stage services in both owners, before light initialization and before any authored placement actor is constructed. `TalkMessageCtrl` retains its idempotent retail create call, but it is no longer the ownership trigger.

Gateway's bounded scene also owns the ordinary `StageSessionState`/`StageSessionBinding` pair for scene `Game`, `HeavensDoorGalaxy` scenario 1, and initial/restart `JMapIdInfo(0, 0)`. Scenario metadata is resolved through the same general resolver as StageHost; no story progress, event flag, or switch is mutated. The binding exists before SceneObjs/Talk and authored actor construction, remains active through their reverse teardown, then restores any prior nested session. The focused Gateway proof now installs a real non-null outer session, verifies Gateway shadows it, and verifies teardown restores that exact pointer and identity.

Both scene owners publish `PlanetMapCatalog` before resolving `StageAuthoredData`. Ordinary PlanetMap factory support is catalog-derived: resolving first made the exact mysterious-planet row look blocked even though the generic factory was ready. The corrected order is session (Gateway), planet catalog, one authored-data resolution, light/demo/collision services, then placement construction. A shared source-order test locks this invariant for StageHost and Gateway without naming any planet row as policy.

### Exact `PlacementInfoOrdered` source tranche

- SameIdSets use the canonical `MR::getObjectName` identifier (`type` wins even when authored empty; only a missing field falls back to `name`) plus exact `ShapeModelNo`. The identifier is shared by support, archive, creator, localized actor name, evidence, and diagnostics.
- Every authored group, including blocked/ignored/null-creator groups, participates in the retail Shell sort. The comparator is load class ascending and member count descending with the retail gap sequence/equal-key behavior. Null creators skip request and construction; real creators request every ordinary row or one generated `%s%02d` shaped archive even when PC support later skips every row.
- The five construction passes are `_FC`, `_104`, `_100`, `_108`, and deferred `_10C`. Both common holders rank before either requests; all scenario/deferred holders rank after complete common requests and before any of their requests. Every request completes before actor construction begins.
- Creator availability is rechecked once per SameIdSet at construction, localized actor names are resolved once per successful group and copied into stable row storage, and a Ready row whose injected creator disappears is normalized to an explicit blocked result so report counts remain honest.
- The resolver retains per-StageObj holder occurrence identity, parent/depth/sibling/discovery provenance, distinct duplicate transforms, and an ancestry-local cycle guard. Root placement ordering includes the root plus direct child holders exactly; deeper holders remain only for recursive evidence consumers.
- Model-changing creation remains explicitly blocked in production until the exact shape-aware creator exists. Its generalized preload seam ranks and mounts the generated archive without borrowing the ordinary creator.
- Focused synthetic evidence now covers failed-preload construction rejection, one-shot preload planning, true reverse destructor order during normal teardown and rollback, missing/empty BCSV identifiers, duplicate sibling/cycle traversal, creator-independent static archive metadata, and group-level nullable localized names.

The final serialized matrix is green:

- `smg-pc-authored-placement-instantiator-tests`: the shared five-pass/grouping/preload/rollback contract passed.
- `smg-pc-planet-map-catalog-tests`: the real RMGK01 census passed with 258 source rows, 256 named rows, and 10 scenario-selected archive rows.
- `smg-pc-sceneobj-holder-real-or-absent-tests`: 9/9 passed, including StageHost preload-before-Mario ordering, catalog-before-authored-data ordering, and the shaped mounted-prefix flag.
- `smg-pc-gateway-demo-scene-tests`: the real RMGK01/Xvfb route passed with 4 ordinary planets, 7 visual evidence actors, 6 KCL meshes / 14,521 triangles, exact start contact at 0.0271512 units, reverse placement teardown, and restoration of a real non-null outer stage-session binding.
- `smg-pc-bright-sun-route-tests`, `smg-pc-sky-actor-route-tests`, `smg-pc-air-actor-route-tests`, and `smg-pc-planet-map-actor-route-tests` all passed. Sky retained 6 Gateway packets and 8 File Select packets; Air and all four ordinary planets retained their authored scheduler/model behavior.

The shared construction route now passes `MR::getJapaneseObjectName` results to every SameIdSet creator, as retail does. Route tests therefore retain English creator/archive/model evidence separately while looking up scheduler actors by their exact runtime names (`レンズフレア用太陽`, `VR軌道`, `球状青空`, and the four authored planet names). This was a test-only expectation correction; no production scheduler category changed.

The expanded authored route also made `SwitchArea` execute in Sky/Air/Planet tests that previously had no player. The reproduced failure was an exact null player position dereference through `SwitchArea::movement` and `AreaFormCube`, not a placement or localization regression. Those harnesses now attach a finite far-away test actor for full-scheduler movement and detach it before teardown. Production retains the retail non-null-player contract; the proper playable boundary remains preload, real Mario construction, then placement construction.

### Global gravity child ownership checkpoint

The generalized teardown blocker now has a host-only ownership boundary; no `Game/Gravity` source, layout, or vtable changed. The generic NameObj factory attaches every exact `GlobalGravityObj` creator to the active `SceneObjHolderBinding` immediately after the retail creator returns and before init. A missing owner, a second uncaptured construction, capacity overflow, or duplicate adoption is explicit. A newly returned known creator is typed-deleted before wrapper unwind when adoption fails; an already-owned duplicate is not deleted.

`NameObjLifecycleService` brackets the exact actor init after placement validation and tracing. It records both follower and target holder counts immediately before init and inspects only the synchronously appended suffix afterward, so expired external/stack followers already retained by `BaseMatrixFollowTargetHolder` are never dereferenced. The follower suffix must belong to the exact wrapper and field. A target is owned only when it is the matching newly appended target suffix; a reused prefix target remains borrowed. Success requires a non-null target, while the noexcept failure capture retains a partially appended `GraviryFollower` and `JMapLinkInfo` even when target allocation was the original failure and never replaces that original exception. Records close once, making the later destroy hook idempotent. Proven-owned followers, links, and targets are deduplicated independently.

SceneObj teardown now explicitly pops its owned objects in reverse after unbinding, reconstructs the external holder to its exact empty state, and only then reclaims gravity children. This keeps every registered field alive through `PlanetGravityManager` retirement and every follower/target alive through `BaseMatrixFollowTargetHolder` retirement. Reclamation is exact-type because neither `PlanetGravity` nor `GravityCreator` has a virtual destructor. All ten creator variants have typed field/creator deletion; Wire additionally retires every `RailPart` linear/Bezier child, the part and coordinate arrays, the copied `JMapInfoIter`, `BezierRail`, `RailRider`, and the `WireGravity` sampled-point array.

Focused tests cover all ten variants, the Wire deep graph, missing-owner and overlapping-construction rollback, duplicate rejection, an expired follower prefix with a borrowed shared target, a distinct newly owned target, exact `FollowId`, a manager query after wrapper retirement, a partial follower/link with null target that preserves the original init error, and same-holder second-generation recreate. `xmake build -j2 smg-pc-gravity-real-or-absent-tests` passed, followed by 10/10 passing focused cases. The cross-owner SceneObjHolder suite also passed 9/9, and the real RMGK01/Xvfb Gateway scene passed with its ordinary planets, 14,521-triangle collision set, gravity evidence, and reverse teardown intact. No stage name, gravity row, or Gateway-specific policy was added.

## Talk utility-provider closure

The focused Talk target previously compiled all Talk/FLW sources and stopped at link time on four omitted retail utility TUs. Those calls now terminate in generalized existing compatibility seams instead of pulling the provider-incomplete TUs into the target:

- `EventUtilCompat` routes `MessageAlreadyRead` and `MsgLedPattern` through the retained `GameDataHolder` event-value table. Message bits outside the retail `u16` fail explicitly.
- `StageSessionState` owns the retail-sized 64-entry AlreadyDone registry. Identity is exactly `(MR::getHashCode(name) & 0x7fff, u16 placed-zone ID, u16 link ID)`, duplicate lookup returns the retained value, and overflow or an absent stage session is explicit.
- `NameObjLifecycleService` installs a copied placement-zone name together with the exact `PlacementStateChecker` zone ID for actor construction. The scope is nested and restores both prior values, so `TalkNodeCtrl::createFlowNode` can build general placement keys without a stage-name lookup or process-global owner.
- `PlayerUtilCompat` supplies hidden-independent attached-player distance. Element-mode branches require an explicit `PlayerActorEntitlementBridge::read_element_mode` capability installed by the concrete player owner; the real Mario owner reads `MarioActor::mPlayerMode`, while missing, generic, and name-spoofed players fail explicitly.
- `ScreenUtilCompat` retains the Yes/No query ABI but throws until a scene-owned selector provides real state. No default answer is fabricated.

## EarthenPipe route audit checkpoint

- Real common placement rows 18 and 19 share `PipeID=1050`.
- Source switch records `SW_B1112` and `SW_B1118` establish the alternate group-1 rabbit at the destination; this is data-driven route topology, not a Gateway policy.
- Pipe state `WAIT` (`0xF0`) suppresses writes and `START` (`0xF1`) enables them. Mode 0 still starts and ends event cameras 17 and 18, and the rows carry no audio arguments.
- The generalized implementation blockers are the pipe mediator SceneObj, event-camera execution, dynamic collision matrix/enable state, blend-matrix and effect hosts, player bind/jump behavior, an authored water query, and real Mario sensor/rush handling. No EarthenPipe implementation has been added yet.
