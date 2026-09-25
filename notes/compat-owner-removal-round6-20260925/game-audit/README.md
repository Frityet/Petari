# Read-only lighting closure audit

The recommended round6 closure is the complete original lighting owners, not just moving the three active compat source files. `source-inventory.json` records 29 inspected native files, eight donors, symbol locations, hashes, and baseline status. No build or runtime was executed for this audit. Root subsequently authorized the implementation under `../lighting/`.

## Exact owner and removal scope

Restore the complete current donor TUs at `src/Game/Map/{LightDirector,LightFunction,LightDataHolder,LightZoneDataHolder}.cpp`, `src/Game/Util/LightUtil.cpp`, and `src/Game/LiveActor/ActorLightCtrl.cpp`. Restore LightDataHolder and LightZoneDataHolder declarations/schema; adjust LightDirector/LightFunction/ActorLightCtrl headers only for necessary native retirement. All original functions are present in decomp at 1a126cb5d; new decompilation is not needed.

Delete `src/compat/{LightDirectorCompat.cpp,LightFunctionCompat.cpp,LightFunctionCompat.hpp,LightUtilCompat.cpp}`. Delete the parallel data/cache owner `src/render/light/LightData.{cpp,hpp}` and `src/scene/StageLightSceneBinding.{cpp,hpp}`. Remove their light-specific hook/member from `OriginalSceneSupport.{cpp,hpp}` and the call in `SceneInitializationCompat.cpp`. Original GameScene already calls LightFunction::initLightData after scenario selection and before actor placement. Change the two ActorRuntimeRegistry unregistration calls once controller destruction owns borrowed-pointer retirement. Preserve LightAreaHolder's native unregister behavior at its actual owning field.

Root build edits: stop excluding Map/LightDirector.cpp, Map/LightFunction.cpp, Util/LightUtil.cpp; remove the explicit render/light/LightData.cpp/header registrations. The remaining canonical TUs are already covered by the Game wildcard. Deleting the compat/scene source files removes their wildcard registration. No extra floating point flag is inferred for Game code; preserve the existing Game target flags.

## Why the entire closure matters

- LightDataHolder::initLightData is empty. LightZoneDataHolder lacks all original catalog implementation and its header returns null. The compat functions return null/zero for archive/parser access and route area queries to StageLightData.
- StageLightData duplicates CSV decoding, zone discovery and defaults. It chooses a root-stage fallback for an absent child ID; original LightZoneInfo instead returns the literal default light name, then LightDataHolder performs its original name search/first-row fallback. Existing test expectations must follow the original behavior rather than preserve the cache policy.
- The compat blend clamps t and blends positions without converting between world and view space. Original blendActorLightPos uses the destination's follow-camera flag and transforms the source into that space. The compat white-light RuntimeContext branch also uses a different position and ambient reset.
- The RuntimeContext branch sends lights to SceneLightService instead of GX. Actual original DrawBuffer emits GX light state after the material list; direct GX is already supported by Aurora and by the --original-gx-only test. No new renderer bridge is required for original Game.
- ActorLightCtrl contains a no-op resetLightSort, although canonical DrawBuffer::resetLightSort now exists. Restore its complete original algorithm and remove the handwritten ActorLightInfo assignment when restoring the donor struct declaration.

## Native boundaries to preserve

The current LightPointCtrl has generation-qualified borrowed actors, destruction of three owned records, PPC NaN brightness/conversion handling and the evidenced zero-duration transition. Keep those semantics; a rename of point record fields or a narrow LightFunction integration change must not replace them with an unchecked donor copy. Coin record inheritance/byte fields differ from the donor's composition/GXColor; restore that schema coherently with its uses rather than casting packed bytes.

Keep the LightDirector's real ownership cleanup and extend the data holders' native destructors to retire their actual arrays. JMapInfo strings are cached in its shared DataCompat, so original parser allocations must remain alive while light names are borrowed. JMapInfo already registers a native heap disposer; do not delete parsers immediately after extracting names. The scene heap owns their lifetime. CP932-wrap the original Japanese literal names.

Move player-controller unregister behavior into the actual ActorLightCtrl/LightDirector lifetime, retaining correct replacement behavior and scene retirement order. Remove the compat-only active/registered diagnostics; tests can inspect the actual director field. Do not add a global replacement cache.

## Tests and source gaps

`tests/PointLightRuntimeTests.cpp` / `smg-pc-point-light-runtime-tests` contains useful native actor-pointer/ABA, clamp, zero-duration, exact GX command, and eight-lifetime checks. Retain those meaningful checks. Its default successful standalone RuntimeContext construction is obsolete (the original FileLoader requires GameSystem), and its fallback/SceneLightService assertions encode the retired implementation. Migrate integration coverage to OriginalStageResourceProcessFixture and actual LightDirector/LightPointCtrl/GX output. Add `smg-pc-app` and `aurora-main` dependencies if the target uses that fixture.

`tests/AreaObjRealOrAbsentTests.cpp` / `smg-pc-area-obj-real-or-absent-tests` directly instantiates the soon-deleted cache and binder in test_rmgk01_zone_light_data_resolves_child_tables. Move useful root/child authored-row checks to a real original-process lighting test; remove cache deduplication/conflicting synthetic metadata expectations. The original process loads every zone from the actual galaxy catalog.

Useful new regression: a camera translation with world-to-view and view-to-world blend endpoints; destination flags/padding remain preserved. Verify original root/child/default identities against the retained JMapInfo parser and exact GX light/ambient registers. Source equality should prove all donor definitions present and single-owned. Required integration validation remains a fresh 600-frame original run and focused ownership/GX tests; this audit makes no runtime claim.

`src/compat/OriginalModelLightAccess.cpp` should remain outside this closure: four overloads belong to the absent 643-line ModelUtil.cpp, while getLightAmbientColor belongs to excluded LiveActorUtil.cpp. Restoring those complete owners requires a separate provider collision/resource closure, not a new partial LightUtil shard. Matrix providers are a separate alternative with donor-versus-earlier-retail-evidence discrepancies; see the companion matrix audit.
