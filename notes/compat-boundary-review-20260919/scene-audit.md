# Scene, placement, factory and original-startup boundary review

Read-only production audit on 2026-09-19, parent HEAD `01f8c899fc33e7c9e7953d3dff9bc8902895173c`. No production edits, builds, GPU runs or gameplay writes. Line numbers refer to this working-tree snapshot. This is a bounded source review, not a claim that every compatibility provider is original-equivalent. Debug controller tools are intentionally outside this review.

The current `--original` scene path uses the original `GameSystem`, `GameScene`, `SceneDataInitializer` and `StageDataHolder`. I found no Gateway-name, rabbit-ID, story-switch or placement-row override in its reviewed startup/scene-support/placement-resource boundaries. There **are** explicit substitute gameplay/selection owners in the older showcase route; those must not be confused with the original path that reached Rosalina.

## Actionable architecture findings

### 1. Legacy showcase policy still occupies the shared scene library

`src/scene/GatewaySpinCheckpoint.cpp:125–145` defines replacement `LiveActor` subclasses named Tico and Rosetta rather than using their original actors. At `:159–167` it selects exact stage rows. At `:318–347` it implements its own 500-unit proximity trigger, player-control change, 90-frame fade and direct start of the spin demo part. At `:351–362` it invokes the spin explanation at a selected sheet part. Reading retail sheet data does not make this substitute control flow a general compatibility service.

Reachability is limited and important: its production callsite is `src/showcase/Showcase.cpp:785–797`; that caller explicitly advances selected-file story progress to 10. `:733–739` separately selects the post-castle checkpoint and `HeavensDoorGalaxy`. `src/app/main.cpp:55–57` sends `--original` directly to `run_original_game`, before the alternate service graph. There is no `GatewaySpinCheckpoint`/`GatewayDemoScene` call in `OriginalGameApplication` or `OriginalSceneSupport`.

The older title/File Select route is also a substitute: `src/scene/TitleFileSelectRoute.cpp:57–88` owns the title/far/blank-selection state machine and A-to-launch decision. `src/scene/FileSelectFarVisual.cpp:55–66` creates replacement slot hosts, `:143–175` creates blank planets/numbers, and `:238–276` reimplements the scale controller. Its only non-test caller is `src/showcase/Showcase.cpp:392`. The comment at `FileSelectFarVisual.cpp:143–147` explicitly chooses different behavior from the current donor `FileSelector::control` (`decomp/src/Game/Map/FileSelector.cpp:212–226`); the donor's suspicious cumulative translation needs decompilation verification before any original-path implementation, not reproduction through this replica.

These files are nevertheless compiled into the shared `smg-pc-game` archive by `src/Game/xmake.lua:99`. This is source/target separation debt, not evidence they execute during `--original`. They are also stale: `Showcase.cpp:762–765` now rejects the legitimately enabled Mario factory before its Gateway route can initialize.

Recommended action: remove the superseded scene replicas, or first move any retained demonstration-only code and its tests into an explicitly separate diagnostic target outside the shared scene library. Preserve reusable ownership/resource mechanisms, not stage-specific sequence owners. Do not repair the old fake route by weakening the current Mario factory.

### 2. Two process/scene lifecycles remain selectable; the faithful path is opt-in

`src/app/main.cpp:55–63` defaults to `Application`/the host service graph unless `--original` is present. That alternative graph has its own `GameSystemService`, `GameSystemSceneControllerService` and `StageInitializationService`; `src/scene/GameSystemSceneControllerService.cpp:113–128` destroys/recreates scenes synchronously and sets its own lifecycle phases. `StageInitializationService.cpp:316–339` separately schedules initialization; `:384–407` keeps its own required SceneObj list.

This is general host orchestration rather than a Gateway-specific hack, but it duplicates responsibilities of the original `GameSystemSceneController`/`GameScene` and makes “the game entry point” ambiguous. The successful original-process evidence does not establish that default path behaves equivalently.

Recommended action: make original Game ownership the normal product entry and retain any indispensable standalone host only as a clearly named diagnostic target. Gradually replace callers of the duplicated orchestration instead of carrying both as equal production implementations. This does not require changing original gameplay state machines.

### 3. Original placement currently treats known-but-unlinked content like unknown factory names

`src/scene/nameobj/NameObjFactory.cpp:127–130` explicitly declares a compiled subset of the retail table; `:833–844` returns null when a creator is unavailable. The original `src/Game/Scene/PlacementInfoOrdered.cpp:87–105` skips null creators. That skip is **canonical**, matching `decomp/src/Game/Scene/PlacementInfoOrdered.cpp:87–105`; it is not an invented Gateway workaround. However, retail's full table recognizes names that this subset does not, so a native stage can start successfully with required content absent.

The strict completeness preflight exists in `src/scene/StageInitializationService.cpp:647–670` / `AuthoredPlacementInstantiator.hpp:21–25`, but the original process does not use that host lifecycle: `src/compat/SceneInitializationCompat.cpp:36–37` calls the original initializer, whose `src/Game/Scene/SceneDataInitializer.cpp:48–49` invokes original `StageDataHolder::initPlacement`.

This is a generic completeness/observability gap, not proof of a newly introduced gameplay rule. The factory's availability descriptions (`NameObjFactory.cpp:726–759`) and existing actual-disc inventory already distinguish missing dependencies. Recommended action: expose the same stage-wide availability inventory at the original loading boundary, with explicit unknown versus known-unlinked classifications and optionally a strict validation mode. Keep the original null-creator behavior for genuine unknown names; do not synthesize dummy actors or impose a Gateway allowlist. A successful startup/catch sequence is not proof that every active stage placement exists.

## Intentional platform and ownership exceptions

- **Explicitly accepted UX exception:** `src/Game/Scene/LogoScene.cpp:60–65`, `:151–155`, `:217–221` skip Wii Remote strap presentation in PC builds. Parent relayed the user's explicit instruction, “we dont need the wii remote thingy btw, like the starting wii remote health thing”. This is authorized and is **not** a finding requiring restoration. It is a visible platform policy rather than an ABI-only change; document it in any source-equivalence classification.
- `src/Game/Scene/GameScene.cpp:186–189` and `:457–473` exclude Wii Message Board image capture under `TARGET_PC`. The canonical `drawOdhCapture` checks `_29` and `MR::isRequestedCaptureOdhImage`, sets the capture port, executes the Message Board draw category, captures and reinitializes GX. This is a platform feature omission, not a stage-specific shortcut. It remains a non-debug Game behavior exception. A cleaner future boundary is an explicit console capture service/capability while preserving Game's request flow; do not label the omission full capture support.
- `GameScene.cpp:77–79` calls `prepare_game_scene_retirement`; `src/scene/GameSceneBinding.cpp:52–79` caches and releases actual child owners at destruction. `src/compat/SceneLifetimeCompat.cpp:15` and `:48` bind/retire sidecars at original lifetime boundaries. These are native lifetime adaptations, not new gameplay decisions.

## Reviewed boundaries that are general or original-authored

| Boundary | Evidence | Classification |
| --- | --- | --- |
| Original process bootstrap and stage request | `src/app/OriginalGameApplication.cpp:278–318`, `:321–357` constructs/initializes real GameSystem, runs its frameLoop, validates any requested scenario in its real parser and calls `MR::requestChangeStageInGameMoving`. | Generic platform frontend. No direct story-progress/switch/nerve writes in this stage selection path. A CLI stage selection is not a replacement stage sequence. |
| Scene support | `src/scene/OriginalSceneSupport.cpp:39–60`, `:104–109`, `:132–140` binds actual scene heap/holders, shared scheduler, collision, planet catalog and authored stage lights. | Generic ownership bridging; the only scene type branch is `dynamic_cast<GameScene*>`, not a named Galaxy or actor. |
| Scene function initialization | `src/compat/SceneInitializationCompat.cpp:63–85` creates the same SceneObj list as `decomp/src/Game/Scene/SceneFunction.cpp:44–66`. Original loading/placement calls remain at native `:19–42`. | Original initialization plus generalized backend binding. |
| SceneObj factory | `src/compat/SceneObjHolderCompat.cpp:587–715` constructs actual linked types or returns null; bindings own their sidecars. Actual original-process and host-lifecycle constructors pass null factory overrides (`OriginalSceneSupport.cpp:54`, `StageInitializationService.cpp:371–372`). | A typed original factory is necessarily an object-ID switch. This is not an actor-name hack or evidence of fixture substitution in production. |
| Actor factory entries | `src/scene/nameobj/NameObjFactory.cpp:457–484`, compared with `decomp/src/Game/NameObj/NameObjFactory.cpp:3812–3864`, retain original `SimpleMapObj`/`HeavensDoorDemoObj` routing. Both `Mario` and `MarioActor` also exist canonically (`decomp/.../NameObjFactory.cpp:617–625`). | Original authored registration/catalog data, despite stage names. Do not remove legitimate rows under a keyword-based “no special cases” rule. |
| Planet catalog exception table | `src/scene/nameobj/PlanetMapCatalog.cpp:42–88` retains original unique class identities. `HeavensDoorInsidePlanet -> SimpleMapObj` at native `:60` is canonical at `decomp/src/Game/Map/PlanetMapCreator.cpp:79–80`. | Original data, not a replacement geometry decision. Ordinary planets are resolved from the actual catalog. |
| Area factory/manager split | `src/scene/AreaObjRuntime.cpp:77–140`, `:605–611` distinguish manager existence from complete exact area types; e.g. CollisionArea is the actual class, not a generic AreaObj substituted for it. Prefix lookup at `:718–725` is consistent with the donor's `strstr(...) == pName` intent (`decomp/src/Game/AreaObj/AreaObjContainer.cpp:373–385`); donor contains unresolved/uninitialized-variable artifacts, so this is not a fresh assembly proof. | Generic typed ownership and authored manager data. Missing managers/classes remain coverage gaps. |
| Placement/resource data | `src/scene/StagePlacementResolver.cpp:590–603`, `:927–944` preserve raw authored JMap rows with zone identity; descriptor world coordinates are separate. `src/compat/StageResourceBinding.cpp:77–110` retains original immediate-child/first-occurrence and active-layer/archive ordering. `:173–182` resolves authored MarioNo, rather than inventing a start. | General format/ownership bridge. Reviewed branches select table categories and authored IDs, not particular stages/rabbits/switch constants. |

Three direct source comparisons were also checked after removing only the CP932 include/wrappers: `src/Game/Screen/GameSceneLayoutHolder.cpp`, `src/Game/Screen/CounterLayoutController.cpp`, and `src/Game/Scene/ScenePlayingResult.cpp` are byte-for-byte equal to their corresponding canonical source after that normalization. No native counter/coin/HUD visibility policy was found in these files. This does not establish every layout backend operation is faithful; that is a separate provider review.

## Suggested order

1. Make the current original process the clear product path; delete/isolate superseded showcase gameplay owners.
2. Report original-path factory coverage from actual loaded placements, preserving unknown-name semantics and partial-port evidence boundaries.
3. Keep exact original factories/catalogs, raw placement ownership and scene sidecars; extend missing subsystem contracts there. Record accepted platform exceptions separately from compatibility completeness.

No production changes are proposed or applied by this note itself.
