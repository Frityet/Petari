# Original planet creator lookup and full factory readiness

Frozen 2026-09-03. Root recovery is complete for this bounded creator tranche.
Native files are a compile proposal only. No shared build, GPU run, native
production edit, or activation was performed.

## Recovered root behavior

`PlanetMapCreator::getCreateFunc` first selects the actual
`PlanetMapWithoutHighModel` creator for a forced-low scenario, otherwise returns
the first matching entry of the original 39-entry unique table, and otherwise
returns the actual `PlanetMap` creator. The previous root loop broke on a
non-match, and both fallback creators and 18 unique entries were absent.
All 39 name/function pairs are restored from retail relocations. The original
table is static initialized data; there is no missing runtime creator-list
initializer in this translation unit.

`PlanetMapCreatorFunction::getPlanetMapCreator` now performs the original
SceneObjHolder lookup (ID 116) and forwards to that actual object's lookup.
The default second `PlanetMap` constructor argument is null, and
`PlanetMapAnimLow(name)` is the actual inline base constructor. The local
`PlanetMapFarClippable` class adds no fields, uses the original 0xAC-byte PPC
allocation and returns 50.0f for far clipping. Its full vtable is verified.
The inline `findUniquePlanet` expresses the recovered search and produces the
exact original lookup machine code; it is not a second runtime factory.

`verify-original.py` compiles the complete root TU with the original compiler.
All 27 selected methods/wrappers, totaling **1,928 retail instruction bytes**,
are identical after verified relocations. This includes all 23 distinct
allocation wrappers, the two lookup methods, and the derived destructor and
clipping accessor. Raw objdiff reports 100% for 26 functions; the scalar
accessor's 97.5% is solely its relocated constant label. The proof resolves
and compares all four bytes of the actual 50.0f constant. It also verifies all
78 table pointers and every relocated/unrelocated byte in the derived vtable.
The DOL remains outside the notes and patches.

## Full original NameObjFactory audit

This audit compiled `src/Game/NameObj/NameObjFactory.cpp`, including its complete
original tables and `NameObjFactoryStubs.hpp` aggregate, never the native
`scene/nameobj/NameObjFactory.cpp` subset. All four tables match retail:

| Table | Rows | Distinct referenced functions |
| --- | ---: | ---: |
| Creator table | 1,183 | 546 |
| Extra archive names | 441 | — |
| Archive callbacks | 91 | 33 |
| Player archive names | 8 | — |

All **4,365 relocated pointers**, full names, and unrelocated table bytes match.
There are **zero null creator entries** in the actual creator table.
All six existing original lookup/archive methods are also relocation-aware
byte exact, totaling **860 bytes**. No root factory source was changed.
`audit-factory.py` reproduces these checks and records every table row and
callback, the entire direct undefined-symbol inventory, and retail provider
source mapping. The full compiler object has **641 direct undefined symbols**,
whose Game symbols span **452 original provider TUs**. Source-file existence
in that inventory does not assert that each body is recovered or executable.

## Native compile proposal and ownership boundary

`stage-native.py` creates the complete original PlanetMapCreator source and
two root headers under `build/original-planet-map-creator-20260903/staged`.
Only umbrella includes are replaced with precise existing Game headers; all
Game method/class/table bodies are literal root source. A narrow ObjUtil
header addition declares the archive-name CSV overload from the preceding
PlanetMap data package. Current native headers are used first, then **root
class-header fallback**. This full planet creator TU compiles. It is not a
closed native link and its fallback headers are not a production import set.

`planet-native-dependencies.json` captures all 75 direct undefined symbols of
that object and current definition presence in the game/common/render native
archives. Twenty-eight are absent from those three archives, including seven
system/ABI symbols normally supplied by system libraries, and 21 Game symbols.
The latter include MapObjActor and specific planet constructors, SimpleMapObj
and RotateMoveObj vtables, and the two utility providers already delivered by
the preceding data package. A present native symbol is not proof of its
original body or lifetime. The complete dependency list is the activation
frontier, not a request to add null-returning constructors.

The whole original factory native compile fails before object generation.
`factory-native-compile.log` preserves the bounded first 20 diagnostics:
missing TVec4f, ShadowVolumeDrawer base interface mismatch, missing scalar
TMatrix direction setters, address-of-temporary bodies, and opaque class
fields expressed as console total size minus sizeof(native base). The audit
lists all 36 such opaque declarations found in root Game headers. They require
actual typed layout recovery; adjusting padding arithmetic would not prove
the original objects. No synthetic class or unsupported-null entry was added.

`native-proposal.patch` is deliberately not activation-ready. After complete
class/provider closure, select the full original PlanetMapCreator TU and
remove `compat/OriginalPlanetMapData.cpp` atomically to prevent duplicate data
methods. Preserve the original archive CSV/model-existence providers and the
existing general ResourceHolderService. Publish the actual PlanetMapCreator
through the existing SceneObjHolder transaction. The full original factory
must then replace `scene/nameobj/NameObjFactory.cpp` and its PlanetMapCatalog
creator authority, while retaining needed general scene services separately.
There must be a single actual factory and actual SceneObj authority.

## Stage and camera readiness

The recovered StageDataHolder constructor and raw JMap/archive lifetime work
are available from earlier checkpoints. Their existence does not yet close
`StageDataHolder::init` for zone zero: it immediately builds and sorts
PlacementInfoOrdered. Sorting calls the complete factory archive queries and
all relevant original archive callbacks. Although sorting does not invoke
actor constructors, its original table contains the actual creator pointers,
so the complete static table retains their link graph. RequestFileLoad and
initPlacement subsequently use those creators directly. Model-changeable
placements also call the separate actual ModelChangableObjFactory and its
archive-name helpers; that graph is not covered by this planet proof.

The actual CameraDirector constructor builds CameraRailHolder, which traverses
actual zone rail data via getPlacedRailNum and getCameraRailInfoFromRailDataIndex,
and constructs real RailRiders. A zeroed or synthetic StageDataHolder/collector
cannot satisfy this dependency. CameraDirector::init itself is originally
empty; constructor/rail readiness must not be confused with that method.
After construction, GameCameraCreator scanArea/scanStartPos and real collision
camera-code collection are additional real-object dependencies.

The next useful bounded root recovery is PlanetMap::init/initClipping, whose
current bodies still discard placement argument results and have suspect
clipping selection/vector logic; the present lookup proof does not validate
those actor bodies. In parallel, full factory import needs the actual opaque
class layouts and complete provider graph. StageDataHolder and CameraDirector
runtime activation remain gated; no constructor runtime fixture is claimed.

## Reproduce and files

Run with `PYTHONDONTWRITEBYTECODE=1`:

```
python3 pc-port/notes/original-planet-map-creator-20260903/verify-original.py
python3 pc-port/notes/original-planet-map-creator-20260903/stage-native.py
python3 pc-port/notes/original-planet-map-creator-20260903/audit-factory.py
python3 pc-port/notes/original-planet-map-creator-20260903/freeze.py
```

The prior `original-planet-map-data-20260903` proof and real-disc runtime package
remain frozen and were not regenerated against this changed TU.
