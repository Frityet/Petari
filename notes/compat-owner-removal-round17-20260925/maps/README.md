# Round 17 Map / MapObj / Ride first pass

Read-only comparison covered the 310 newly imported units in these directories and their corresponding donor headers. No build or test was run by this lane; root owns the integrated build and reports concrete failures.

Restored complete existing donor declarations in BenefitItemObj, BenefitItemLifeUp, ChipBase and ClipArea. This removes a duplicate BenefitItemOneUp definition, undefined legacy destructor declarations, stale field spellings and missing actual inline methods. The separate real BenefitItemOneUp header remains authoritative; root added that explicit include to its ObjUtil.cpp caller.

WhirlPoolAccelerator's old native file implemented only construction, an empty destructor and calcInfo. Its complete existing donor source/header now supplies real point construction, initialization, movement, draw-plane and material behavior. The only source difference from the donor is the existing Aurora Revolution GX umbrella include. No fabricated behavior or new decompilation was introduced.

Normalized unavailable specialized Revolution GX headers to the existing Aurora revolution/gx.h umbrella in owned Map/MapObj/Ride sources. GravityDust uses std::bit_cast for its float user-work bit pattern in both directions, preserving the original representation without host reference aliasing.

Closer inspection confirmed that MtxUtil.hpp's apparent vector setMtxTrans body is commented out. BenefitItemObj.cpp remains the sole actual definition and was deliberately preserved.

27 owned files have exact before/after snapshots and a lane-only patch. Before states include root's central donor import this round; those imports are separately recorded in the root manifest. No shared utility headers, xmake files, tests, index, or Git state were edited. PlanetMapCreator/PlanetMapWithoutHighModel/PlanetMap headers remain the factory lane's scope. First-pass sources are frozen pending concrete compile errors.

## Aggregated compiler diagnostics repairs

The root supplied diagnostics for 85 Map/MapObj/Ride units. This lane repaired local source integration and declarations, leaving shared JGeometry restoration and SDK utility additions to their assigned owners. Total owned paths are now 50. Root's separately recorded Functor include additions in previously snapshotted sources are preserved; new snapshots were taken after its include batch.

Explicit includes now expose CameraTargetArg, GeometryBindUtil, MutexHolder, TriangleFilter, DirectDrawUtil, SpringValue, SequenceUtil and GDGeometry. The GD call sites remain display-list recording calls; root supplies missing SDK GD encoders. ElectricBall and GeneralMapParts replace obsolete MSL standard-library adapters with equivalent ordered loops/lambdas.

Original Revolution s32 is signed long (decomp/libs/RVL_SDK/include/revolution/types.h:6), so original startBckPlayer(...,0L) selects the s32 overload. Native calls now explicitly pass s32(0), avoiding LP64 ambiguity without switching to the separate animation-name overload. PalmIsland similarly selects its original integer random-range overload. Other fixes use correct zero palette indices/counts/booleans, a scoped switch temporary, invocation of the Sandstorm predicate, an f32 vector literal and actual matrix-array dereference.

MapPartsFunction and MapPartsRotatorBase now declare methods provided by imported donor TUs instead of duplicating inline native bodies. The misplaced MapPartsBreaker TVec2::squared body is removed because the canonical JGeometry template already implements it. GreenCaterpillarBig declares its actual LodCtrl pointer type.

No builds, tests, xmake, index or Git operations were run. Production is frozen for root's second aggregated compiler pass. Shared pending dependencies at handoff: full MercatorTransformCube donor header, donor Color10 GXColorS10 conversion, GDPosition3f32/GDColor4u8/GDBegin actual SDK writers, plus the shared math lane changes.

## Link closure

Restored six complete existing donor owners: SpiderThread, SpinDriverPathDrawer (including its draw initializer and MR helpers), ChipGroup, BlackHole, MercatorTransformCube and OceanSphere. The former Mercator throwing placeholder is gone; original sphere coordinate/rotation and rail subdivision now execute. OceanSphere uses the donor's actual GXColor data instead of unprovided per-byte externs. BlackHole regains its constructor/base-matrix update and donor clipping calculation. The previous source fragments contained no native lifetime adaptations to retain.

Restored matching SpinDriverPathDrawer, ChipGroup and BlackHole headers; updated ChipHolder's group-ID field access to the donor's named field. SpinDriver path color packing explicitly shifts its RGBA bytes to preserve big-endian numeric color on little-endian hosts. Remaining changes against donor are CP932 narrow literals, available GX umbrella include and explicit Functor declarations. No duplicate utility providers were found for these methods. Existing scene draw helper implementations were retained verbatim inside their complete owners.

59 total owned paths now have refreshed snapshots/hashes/patch. All six owners already participate in the Game source glob; no build wiring or tests changed. No independent build was run. These are donor-available restorations, not new decompilation or stubs.
