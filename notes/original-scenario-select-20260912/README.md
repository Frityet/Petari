# Original scenario-selection startup dependency

Imported the complete original ScenarioSelectLayout, ScenarioSelectStar, MultiSceneActor and MultiSceneEffectKeeper owners to close original startup linkage. BackButton already exists. All four native translation units compile. No new tests or shared build changes; the existing Game source glob includes these additions.

Recovered the one missing ScenarioSelectLayout screen-to-world calculation in reference first. Retail 0x8037EDC4 uses JUTVideo EFB height, the camera's vertical field of view and original table-based tangent, defaults a negative requested depth to screen depth, flips screen Y and transforms by the inverse view matrix. The new method compiles with the original compiler and matches 94.35%; no native geometry substitute was used.

Recovered the complete 11-method MultiSceneEffectKeeper in reference first. It sizes the emitter array using the real particle resource holder, registers authored effects, creates/deletes real MultiEmitters, and looks up their original 16-bit name hashes. Whole text matches retail 100%. Added the missing TVec header dependency so its existing declarations compile independently. Both recovered native sources are copied from reference unchanged.

Reference paths:
- decomp/src/Game/Screen/ScenarioSelectLayout.cpp
- decomp/src/Game/Scene/MultiSceneEffectKeeper.cpp (new)
- decomp/include/Game/Scene/MultiSceneEffectKeeper.hpp (include only)

Native paths:
- src/Game/Screen/ScenarioSelectLayout.cpp (new)
- src/Game/Map/ScenarioSelectStar.cpp and .hpp (new)
- src/Game/Scene/MultiSceneActor.cpp (new)
- src/Game/Scene/MultiSceneEffectKeeper.cpp (new) and .hpp (include only)

Logs and the two required retail comparisons are retained here. These are compilation and original-recovery results; root's integrated build/run determines the next real startup blocker. No scenario, particle or process owner was fabricated.

## Actual post-selection initialization follow-up

Imported complete original `src/Game/Scene/SceneDataInitializer.cpp/.hpp`. `SceneObjHolderCompat.cpp` now constructs that real owner and `StageDataHolder(MR::getCurrentStageName(), 0, true)` through the two original factory cases. Stage load/wait, actor archive load, placement, and post-selection initialization in `SceneInitializationCompat.cpp` now use the original SceneDataInitializer forwarding sequence. In particular, `initAfterScenarioSelected` first requests the original Luigi-letter archives then initializes the actual StageDataHolder's selected layers. Existing scheduler/category/allocation bookkeeping remains at its current native boundary, as requested by root; whole SceneFunction activation is therefore still held.

The three affected native translation units compile. The existing EffectSystemUtil → AutoEffectGroupHolder → AutoEffectGroup chain already supplies all MultiScene keeper registration overloads; no extra compatibility fragments were added. This follow-up touched only the two new initializer files and the two compatibility files named above. No new tests or shared build edits.
