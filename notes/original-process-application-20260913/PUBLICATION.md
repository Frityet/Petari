# Original process, scene and sequence publication

Frozen against parent `c518e3af2f41e136b8193adfdff2a75f972dd06f` and reference
`d776af541a8007a37122373a36c5396a5494353d`. No commits or shared builds performed
by this lane. Root owns publication order and the running application.

## Whole owned files

Take the complete current contents/diffs of these files:

```
src/app/OriginalGameApplication.cpp
src/app/OriginalGameApplication.hpp
src/scene/OriginalSceneSupport.cpp
src/scene/OriginalSceneSupport.hpp
src/scene/GameSceneBinding.cpp
src/scene/GameSceneBinding.hpp
src/scene/SceneExecutionBinding.cpp
src/scene/SceneExecutionBinding.hpp
src/scene/SceneNameObjRegistry.cpp
src/scene/SceneNameObjRegistry.hpp
src/runtime/MessageHolderOwnership.cpp
src/runtime/MessageHolderOwnership.hpp
src/compat/EffectSystemOwnership.cpp
src/compat/OriginalParticleResourceLookup.cpp
src/compat/SceneInitializationCompat.cpp
src/compat/SceneLifetimeCompat.cpp
src/Game/Scene/GameScene.cpp
src/Game/Scene/SceneDataInitializer.cpp
src/Game/Scene/SceneDataInitializer.hpp
src/Game/Scene/MultiSceneActor.cpp
src/Game/Scene/MultiSceneEffectKeeper.cpp
src/Game/Scene/MultiSceneEffectKeeper.hpp
src/Game/Screen/ScenarioSelectLayout.cpp
src/Game/Map/ScenarioSelectStar.cpp
src/Game/Map/ScenarioSelectStar.hpp
src/Game/System/PlacedHiddenStarScenarioTable.cpp
src/Game/Util/SequenceUtil.cpp
```

`GameScene.cpp`'s complete remaining diff is only the TARGET_PC child-retirement
include/call; its disabled-audio and message-board capture boundaries are already
in HEAD. `MultiSceneEffectKeeper.hpp` adds its actual vector declaration include.
`GameSceneFunction.cpp` is unchanged original source, activated through the build
list. Other unchanged existing original headers are already in HEAD.

Remove both duplicate files:

```
src/compat/OriginalGameSceneFunction.cpp
src/compat/SequenceUtilCompat.cpp
```

## Exact shared-file scope

`publication-shared-hunks.patch` contains only this cohort's four shared-file
changes, based on the parent above. It is for partial staging/overlay against
HEAD, **not** applying again over the already changed working files.

- `src/app/main.cpp`: OriginalGameApplication include, algorithm include, and
  `--original` dispatch after opening the disc. Root's ProcessRequest include,
  terminal catch and DiscLifetime guard are necessary companion changes, owned
  by root and intentionally outside this patch.
- `src/compat/SceneObjHolderCompat.cpp`: GameSystem/SingletonHolder and
  SceneDataInitializer/StageDataHolder/SceneUtil includes; original-only message
  alias timing conditional; two actual factory cases for SceneDataInitializer
  and StageDataHolder. Preserve the pending Talk/DrawSync/Event/StarPiece/shadow
  changes separately; the scene support uses their final lifecycle interfaces.
- `src/compat/SceneNameObjUtilCompat.cpp`: three actual process includes and
  `scene_objects()` borrowing the original controller's NameObjHolder. The
  TalkRuntime include and pauseOffTalkDirector removal belongs to Talk activation.
- `src/compat/StorySequencePlatformCompat.cpp`: remove StageSessionState include
  and only `isExecScenarioStarter`, `requestChangeScene`, `requestChangeSceneTitle`.
  Root's removal of `isStarCompleteAllGalaxy` belongs to whole EventUtil activation.

Root's two required shared build changes are already on disk:

- `src/app/xmake.lua`: add `OriginalGameApplication.cpp` to `smg-pc-app`.
- `src/Game/xmake.lua`: remove only exclusions for `Scene/GameSceneFunction.cpp`
  and `Util/SequenceUtil.cpp`. Other new Game files use the existing glob;
  OriginalSceneSupport uses the existing `../scene/**.cpp` glob in Game/xmake.lua. Do not stage the
  whole Game build file solely for this cohort.

## Behavior and dependencies

The app provides `--original`, bounded actual `--max-frames`, and optional
`--stage NAME --scenario N` selection through the real GameSequence transition
after stationed resources and original data owners are ready. No selection
preserves original boot. The scene support borrows real controller holders and
heap, survives the actual init-worker/main-thread handoff, and retains the
real-disc planet catalog before stage placement. Message and particle lookup
prefer actual GameSystem owners; GameScene keeps original scene-message timing.
Sequence and initializer methods now execute their original owners. Scenario
selection imports the actual layout/star/multiscene actor/effect graph.

This is **not independently linkable with only HEAD plus these files**. The
successful shared application build also uses these pending companion providers:

- Root's `src/app/ProcessRequest.hpp` and main terminal/disc-lifetime changes;
  actual Aurora ProcessRequest/reset implementation is already published.
- Actual JUT display/MainLoop and renderer integration; OriginalProcess directly
  constructs/tears down original JUTDirectPrint/JUTVideo/JUTXfb services.
- Process agent's `destroy_star_pointer_director` in StarPointerDepthOwnership,
  plus real GameSystem pointer/WPad query routing. App does not create substitutes.
- DrawSyncManagerLifetime and SceneObjHolderBinding::prepare_retirement from the
  DrawSync/Talk cohort. OriginalSceneSupport calls these during actual retirement.
- Root's pending `src/scene/nameobj/PlanetMapCatalog.cpp` include plus
  `PlanetMapCreatorFunction::isLoadArchiveAfterScenarioSelected` forwarding
  definition; OriginalSceneSupport now supplies the retained catalog it requires.
- Whole Talk, layout/NW4R, Event and screen owner activation already used by the
  shared build, including ScenarioSelectLayout's actual text/material graph and
  SceneDataInitializer's original Luigi-letter resource requests.

Publish those companions first or in the same coherent checkpoint. Do not omit
required interfaces or restore their former proxy providers to separate commits.
No tests or fixture migrations are included in this publication scope.

## Existing evidence and next boundary

Root reports the sixth full application build passed with this source cohort,
except the subsequently added planet lifetime change. That exact change passed
its narrow native compile (`OriginalSceneSupport-planet-catalog-compile.log`).
Earlier original compiler recovery evidence for ScenarioSelectLayout and
MultiSceneEffectKeeper remains in `notes/original-scenario-select-20260912/`;
sequence sources compile unchanged against reference. This manifest adds no
new testing or recovery claim.

Root's live run reached the real layout path and is fixing that concrete runtime
fault. No new dependency was implemented while preparing publication. After that
fix, the real authored stage load will expose the next boundary; unique,
force-low, and optional-submodel planet creator paths remain explicitly
unavailable in the existing factory rather than falling back to ordinary planets.
