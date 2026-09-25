# Scene factory/catalog removal boundary

Read-only audit, 2026-09-25. No source edits, builds or tests. Inventory was 36 files still present under `src/scene`; lifecycle/execution ownership is assigned separately, and StageCollisionService is assigned to the collision lane. Initially deleted scene files are not evidence of a remaining production requirement.

## Immediate deletions: no replacement needed

- `StagePlacementResolver.cpp/.hpp`: no production consumers remain. This is a second implementation of stage archive discovery, layer/scenario filtering, holder occurrence traversal, placement matrices, start/general-position lookup and factory-support classification. Its remaining callers are fixtures. The actual `StageDataHolder`, `SceneDataInitializer`, `PlacementInfoOrdered`, `ScenarioDataParser` and JMap utility owners now supply those operations. Delete the service and retire its obsolete fixture portions rather than transplanting this algorithm.
- `PlacementZoneScope.cpp/.hpp`: only two fixtures use it. Original `StageDataHolder::initPlacementMario` and `PlacementInfoOrdered::initPlacement` already set/clear the actual `PlacementStateChecker`. The scope is not needed by production. A retained fixture can set/restore that actual checker locally.
- `nameobj/ObjectNameTable.cpp/.hpp`: no production consumer. Actual `StageDataHolder::initTableData` attaches `/StageData/ObjNameTable.arc` / `ObjNameTable.tbl`; `getJapaneseObjectName` performs the original `en_name` lookup and returns `jp_name`. Delete the duplicate unordered-map/string catalog and service-specific fixture. Keep the actual parser's current lifetime/CP932 representation.

These six files can go independently of the full factory import.

## Live closure: restore the actual Game factory and planet owner together

Delete these only when the canonical providers are enabled:

- `nameobj/NameObjFactory.cpp/.hpp`
- `nameobj/NameObjArchiveTable.inc`, `nameobj/OriginalNameObjNames.inc`
- `nameobj/PlanetMapCatalog.cpp/.hpp`
- `AreaObjRuntime.cpp/.hpp`

The scene factory supplies the real `NameObjFactory::*` and `MR::*ModelChangableObj*` symbols today. Its extra native APIs are used by fixtures, the orphan resolver, and debug placement coverage. The parallel area descriptor table is used only by this factory and fixtures. Original Game `NameObjFactory.cpp` already contains the area constructor/form entries and complete archive metadata; keep those as the sole authoritative rows.

The existing native factory has 94 explicit supported rows, a separate area table, eight dynamic archive callbacks, and a two-class unique-planet allowlist (`SimpleMapObj`, `RotateMoveObj`). It also manually blocks six actor names. None of that is a complete representation of current compiled Game capabilities. For example, existing archive definitions include `Flag`, `FurPlanetMap`, `RailPlanetMap`, `OceanBowl`, `OceanRing` and other original constructors beyond that allowlist. Availability must follow actual implementation closure, not perpetuate old reason strings.

### 1. PlanetMapCreator

Import `decomp/src/Game/Map/PlanetMapCreator.cpp` and its current header into their canonical Game paths. Existing `GameScene::init` already requests `SceneObj_PlanetMapCreator` before the scene data initializer; set its actual availability from 0 to 1. Remove OriginalSceneSupport's `_planet_map_catalog` construction/field/reset rather than adding another publication site.

This restores actual `mPlanetMapData`, the original lookup and archive list, current stage/scenario selection, and original `PlanetMapCreatorFunction` queries. The current catalog categorically rejects any row containing a force-low scenario; the donor tests the current `stage_scenario` string and uses ordinary/unique creators on nonmatching scenarios. This is a real behavior correction.

Dependencies and native lifetime details:

- Retain the constructor's `MR::createCsvParser` result on the actual owner; donor drops this raw pointer. Its strings borrow the actual archive. Actual destructor must release parser, row structs, row pointer array and allocated submodel names before archive retirement. Missing models currently null the allocated name pointer in the donor; free the rejected name first or retain ownership separately on this same owner.
- Import the small existing donor `PlanetMapWithoutHighModel` owner for the force-low branch. Do not substitute ordinary PlanetMap.
- Synchronize actual declarations: donor `PlanetMap` has a default second constructor argument; current port header lacks it. Donor `PlanetMapAnimLow(const char*)` is inline, while port still declares a no-argument constructor. Donor `PlanetMapCreator.hpp` also declares inline `PlanetMapFarClippable`, absent from current header. Restore real declarations rather than inventing stub actor layouts.
- The planet file references 23 distinct `createNameObj<T>` classes overall. Only a subset currently exists; grouped headers matter (`FurPlanetMap`, `RailPlanetMap`, `PlanetMapAnimLow` live in PlanetMap.hpp; several SimpleMapObj variants share one header). Import complete existing donor families where bounded. Remaining unique planets must be explicitly unavailable in the actual canonical table until their real class closure is linked; never silently fall through to ordinary PlanetMap.

### 2. NameObjFactory / ModelChangableObjFactory

Enable `src/Game/NameObj/NameObjFactory.cpp` (currently explicitly excluded in Game/xmake). Start from current donor, preserve table order, aliases, duplicate rows, null rows, archive names and dynamic archive callbacks. Remove the scene providers in the same edit to avoid duplicate symbols.

This is not a one-line source enable:

- The donor's ordinary factory uses 438 distinct `createNameObj<T>` types plus special MR/Koopa creators, area templates and archive callbacks. Most original module umbrella headers are absent from the port. `NameObjFactoryStubs.hpp` is misleadingly named: current donor contains 151 lines of real Game header includes, not surrogate class definitions; 135 of those included paths are absent locally. Do not invent constructor-only class declarations or fake object sizes.
- Existing debug library inspection found 48 of those 438 types with an emitted constructor definition. This is only a lower bound, not a capability list: `SimpleMapObj`, `RotateMoveObj`, and other valid constructors are inline, and vtable/init dependencies must also resolve. Use actual selected source/header/vtable closure for build availability; source basename existence and nm constructor counts alone are insufficient.
- Smallest practical full-owner restoration is to retain all donor rows and metadata in canonical Game, with compile-time availability controls only around references whose real implementation is absent. Link every available actual creator and callback, and leave explicitly unavailable rows null under the native build. That is an incremental canonical factory; a second runtime allowlist/registry would retain the current problem. Missing implementations remain an explicit gameplay gap and can then be imported by actor family.
- The canonical factory also references MorphItemObjNeo nerve definitions/keep-alive references. Those require their real class owner or availability gating; importing declarations alone does not satisfy the implementation.
- Import canonical `ModelChangableObjFactory.cpp` in the same symbol handoff. Current native code reproduces its twelve rows but all creator pointers are null (including the duplicate TripodBossRotateParts row). Its ten distinct template classes and `MR::createSunshadeMapParts` have no emitted constructor/function definitions in the inspected game archive. This is a separate MapParts/TripodBoss/Mercator implementation group, not a reason to keep a scene factory.

## Debug placement coverage

`OriginalPlacementCoverage.cpp/.hpp` has one production call: a `!NDEBUG` block in `SceneFunction::startActorPlacement`. It reads actual StageDataHolder and PlacementInfoOrdered queues, emits optional JSON, and conditionally rejects known-unlinked names. Its support classification depends on the scene factory's duplicate membership table.

Fastest deletion is to remove this optional diagnostic with its invocation. If its strict missing-actor diagnostic is still wanted, keep only a narrow `!NDEBUG` check at the actual canonical factory/placement owner using actual rows and availability; do not preserve the 230-line scene reporting service or move a second registry elsewhere. Original placement order/zone traversal already belongs to Game and must not be replaced by the orphan resolver.

## Coordination and limits

- This grouping accounts for 16 scene files: six production-orphaned files, eight factory/catalog/area files, and the two debug-coverage files. Remaining scene lifetime, registration, draw/execution and child-owner files are the other agent's scope; StageCollisionService belongs to its assigned lane.
- `scene/nameobj/NameObjFactory.cpp`, `OriginalSceneSupport.cpp`, GameSceneBinding/NameObjChildOwner and collision files are already modified by current or prior work. Preserve those deltas. `SceneObjAvailability.hpp` is new in the current scene-owner batch. Coordinate its PlanetMapCreator enable with that lane.
- Old scene preflight/authored placement service files are already locally deleted. No new preflight service, test framework or exhaustive fixture rewrite is needed. Existing actual-process placement coverage is the relevant consumer after deletion; root owns the single integration build/smoke.
