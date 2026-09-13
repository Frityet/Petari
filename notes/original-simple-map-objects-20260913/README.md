# Original SimpleMapObj / MapObjActor owner recovery

Gateway's authored `HeavensDoorInsidePlanet` creator is `SimpleMapObj` in original `PlanetMapCreator.cpp`; it must not be substituted with `PlanetMap`. The original typical configuration enables its projection matrix path. Factory/catalog activation remains blocked until the lower owners link and execute.

Recovered full `MapObjActor::initialize` from retail `0x801F695C`, 2,800 bytes. It retains all configurable branches: models/display lists, mirror/projection, sound/effects/shadows, collision/sensors, all rail/rotator/seesaw modes, material animation frames, clipping/LOD/Bloom/break models, switches and demo appearance. Original compiler succeeds; the initial source scores **99.078575%** against the retail object. Remaining differences are instruction register/allocation and literal-pool placement plus equivalent expression ordering. This is source recovery evidence, not native gameplay proof.

The actual common pointer is `MapPartsRotatorBase*`. The existing seesaw helpers had treated Seesaw fields as a different class's matrix; both now access the real Seesaw1 fields with a typed cast. Each produces **100%** retail match and lets native pointer width determine its actual class layout. `makeSubModels` is the actual empty inline virtual (retail `0x8018291C`, four bytes). The original LodCtrlFunction query needed its public declaration, already present natively. The actor overload of getMapPartsArgRailGuideType is now declared from the original call signature.

Native imports: full `MapObjActor`, `MapObjActorInitInfo`, `SimpleMapObj` and `MapPartsFunction` translation units and headers. InitInfo, SimpleMapObj and MapPartsFunction compile under both original and native compilers. MapObjActor also compiles natively after the coordinated RailRotator declaration import and an explicit MapPartsUtil include. All four imported translation units have now compiled. Seesaw/RailRotator recovery belongs to Rawls; StageEffectDataTable recovery belongs to root.

Owned paths for this checkpoint:

- `decomp/src/Game/MapObj/MapObjActor.cpp`
- `decomp/include/Game/MapObj/MapObjActor.hpp`
- `decomp/include/Game/LiveActor/LodCtrl.hpp`: add public to LodCtrlFunction only.
- `decomp/include/Game/Util/MapPartsUtil.hpp`: actor RailGuideType overload only; preserve Rawls's parallel six indexed overload additions.
- `src/Game/MapObj/{MapObjActor,MapObjActorInitInfo,SimpleMapObj,MapPartsFunction}.{cpp,hpp}`
- Declaration-only native imports `src/Game/MapObj/{MapPartsRotator,MapPartsRailMover,MapPartsRailPosture,MapPartsRailGuideDrawer,MapPartsRailGuidePoint}.hpp`. Rawls owns the subsequent RotatorBase destructor inline change in Rotator.hpp.
- `src/Game/Util/MapPartsUtil.hpp`: same actor overload only, preserving Rawls's additions.
- `src/Game/MapObj/StageEffectDataTable.hpp` was initially copied exactly from reference; root now owns its full recovery.

No NameObjFactory/catalog/index/build-list changes or tests in this checkpoint.

Remaining direct lower owners: incomplete MapPartsRotator movement/update/rotation functions, RailMover functions, RailPosture source correction, RailGuideDrawer/Holder/Point methods and scene factory. MapPartsUtil has 58 original functions, 10 current bodies; Rawls is recovering six indexed rotation readers. Its remaining cohesive restoration comprises body-sensor helpers, rail rotation coordination, shape names, clipping/shadow/guide initialization and original typed placement/rail readers. The guide factory must return the real SceneObj 0x56 MapPartsRailGuideHolder. No placeholder successes or alternate authored parameter defaults are being added.
