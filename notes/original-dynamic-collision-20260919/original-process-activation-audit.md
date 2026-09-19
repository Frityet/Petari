# CollisionArea activation audit — 2026-09-19

Read-only follow-up after parent checkpoint `c09a26827`. No actor descriptor, production source or executable was changed by this audit. The current J3D ownership investigation remains independent.

## Exact registration and authored placements

The canonical factory has **one** entry: `CollisionArea` -> `createCenterOriginCube<CollisionArea>` (`decomp/src/Game/NameObj/NameObjFactory.cpp`, lines 592–594; helper selects `AreaForm::Type_Cube1`). The two Gateway instances are placements of this same type, not two shape variants.

The eventual native change is one include of `Game/AreaObj/CollisionArea.hpp` and one `AreaObjPlacementDescriptor` in `src/scene/AreaObjRuntime.cpp`:

```cpp
AreaObjPlacementDescriptor{
    .object_name = "CollisionArea",
    .object_creator = create_area_obj<CollisionArea, AreaForm::Type_Cube1>,
    .manager_name = "CollisionArea",
    .retail_manager_order = 61,
    .manager_capacity = 0x40,
    .manager_creator = create_area_obj_manager,
},
```

The manager descriptor is already present. `src/scene/nameobj/NameObjFactory.cpp::area_obj_create_table` derives its creators from the placement descriptors, so no separate creator table row is needed. Adding a manager alone does not support the actor.

The real-disc dump `../gateway-placement-audit-20260919/placements.json` records both instances in `HeavensDoorMysteriousZone`, zone 5, holder 1, `jmp/placement/common/areaobjinfo`:

| Row | l_id | Authored local position | SW_APPEAR |
| --- | --- | --- | --- |
| 4 | 18 | (14892.1123, -8893.3838, 6202.6870) | 1015 |
| 5 | 19 | (14407.5039, -8874.3535, 6343.5576) | 1015 |

All eight object arguments are -1; the other switches are -1. `MR::getJMapInfoArgNoInit` ignores -1 values, so CollisionArea's defaults remain `_5C=0` (actual player radius) and `_60=-1` (all six faces allowed). Both polygons are constructed even while the appearance switch is off, then their collision membership is invalidated. The old dump's support flags describe its earlier binary and must not be used as current support results.

Switch 1015 also belongs to the authored tower/Rosetta phase. This audit does not establish its complete activation call chain; do not flip it from a debugger to claim story progression.

## Actual process owners

These are source/order findings, not a successful AreaPolygon execution result:

- Original `GameScene::init` calls `SceneFunction::initForNameObj` and `initForLiveActor`. The latter constructs SensorHitChecker, CollisionDirector, AreaObjContainer, StageSwitchContainer, SwitchWatcherHolder, SleepControllerHolder and the other original scene objects. GameScene creates CameraDirector before placement.
- `StageDataHolder::initPlacement` initializes Mario before high-priority and ordinary placement. AreaPolygon's `MR::isPlayerElementModeTeresa` therefore has the real Mario owner. The actual player center, radius, Teresa disappearance and area velocity APIs are present; no temporary player mode provider is needed.
- `PlacementInfoOrdered::initPlacement` sets the authored current placement zone before construction/init and clears it afterward. This satisfies `create_generated_collision_parts`' requirement for a valid original zone; both instances use zone 5.
- `OriginalSceneSupport` owns the active StageCollisionService, actual Game heap domain, original holders and execution binding. The GameSystem/controller/FileLoader/resource-language owners are supplied by normal process startup. The failed standalone probe's absent FileLoader is not evidence that OriginalProcess needs a replacement loader.
- AreaPolygon makes an actual eye sensor named `body`; generated parts use its real host, original CollisionParts::init/KCollisionServer, category 0, current zone and camera-code context. Its model-less init requires no new object archive.
- Original scene retirement releases LiveActor runtime sidecars while the scene collision service remains alive. The generated registration is disabled before the parts/server are destroyed; retained generated arrays live until the service releases its source records. This order has focused lower-boundary coverage, but has not yet run for a fully initialized AreaPolygon in OriginalProcess.

An `nm -C` check found CollisionArea init/movement/hitCheck, AreaPolygon init/setSurfaceAndSync and DynamicCollisionObj::createCollision definitions in `build/macosx/arm64/debug/libsmg-pc-game.a`. The same definitions were absent from the current main executable, as expected while no factory entry pulls them from the archive. Archive presence and the earlier probe's successful linkage are not live initialization evidence. No additional missing owner was identified by this bounded source audit.

## Concrete execution gates

1. **Full owner integration before support is advertised.** Run the actual OriginalProcess to a fully initialized GameScene with the real disc, fresh save, original Mario and normal renderer. A test-only/native diagnostic hook outside Game may construct an owned temporary AreaPolygon using an original AreaFormCube initialized from one real placement iterator, within the actual Game allocation domain and scoped authored placement zone. Use the existing NameObj child ownership mechanism and restore the placement scope. This replaces only the old probe's inadequate process setup; it must not construct a substitute GameSystem, FileLoader, CollisionDirector, sensor or player. Run the probe synchronously at a safe guest boundary, with no intervening gameplay frame while the temporary part is enabled, and retire it before resuming. This is an API integration exercise, not gameplay progression.

2. **Six actual face writes and both query paths.** Through the original `AreaPolygon::setSurfaceAndSync`, exercise 0/+X, 1/-X, 2/+Y, 3/-Y, 4/+Z and 5/-Z using the initialized real Mario mode. After every write require the same file, CollisionParts, server, two local prism slots and global surface IDs; compare the resulting vertices/normals with the actual form world matrix/size and original 10-unit non-Teresa expansion. Cast through the interior of each face away from the shared diagonal. Original KCollisionServer arrow traversal and native StageCollisionService geometry must agree on prism/part identity, position and fraction. In a populated scene, select/filter this part explicitly rather than assuming it is the nearest object in an unfiltered world query. Exercise original invalidate/validate membership and prove that disabled parts cannot be queried. This extends the committed lower-boundary tests through actual AreaPolygon::init and DynamicCollisionObj::createCollision.

3. **Retirement under the same live process.** Release the temporary actor via its real owner. Require actor collision sidecars and zone membership to disappear before parts/server storage is destroyed; queries must not return its old surface IDs. The typed KCL identity may remain while the scene service retains the source, and must disappear after normal scene-cache retirement. A normal same-process scene transition/reload should produce a new service generation and no stale source identity. Process exit 0 alone is not proof of same-process scene retirement. If a normal scene transition is not yet available, record that part as unverified rather than adding fake scene owners.

4. **Enable the single descriptor, then validate real placements.** A fresh normal `--original --stage HeavensDoorGalaxy --scenario 1` must initialize exactly two authored CollisionAreas with real child polygons/sensors and two prisms each in zone 5. On the initial off switch, membership must be disabled. Use original controls and authored demo progression to reach SW_APPEAR 1015, observe actual movement -> hitCheck -> setSurfaceAndSync, and confirm original collision is active at the tower. Read-only trace/debug-guarded logging can record observed faces. A short waking/dialogue run will not cover tower activation; observing fewer than six faces naturally does not substitute for the separate six-face API integration gate.

No part of this audit claims that these execution gates have passed. The standalone full actor probe remains blocked as documented in README.md; the committed resource tests and generated-only lower-boundary case remain the established collision validation.
