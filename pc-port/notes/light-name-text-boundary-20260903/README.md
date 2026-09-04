# Game-facing light-name byte identity

Staged only, 2026-09-03. No production Game/native source, xmake target, shared
build, compiler filter or owner selection was changed. The parent owns the
pending original CP932 execution charset and the generic TextEncoding encoder.

## Boundary and patch

`render/light/LightData.cpp` previously decoded both main-table and zone-table
AreaLightName fields from CP932 into UTF-8 before storing them. The resulting
AreaLightInfo names and default-name API therefore differed from the original
BCSV bytes and pending CP932 Game literals. Both eager conversions are removed.
The existing owned strings, reserve-before-publication storage, lookup order,
default selection and fallback remain unchanged. The producer now publishes
raw authored CP932 bytes at every Game-facing name boundary.

The smallest production delta is that source plus a contract comment on the
StageLightData accessors. No display-cache facade, encoding auto-detection,
UTF-8/CP932 mixed comparison, actor-specific workaround or Game edit is added.
Decode only an explicitly known CP932 value at a host presentation sink, using
the existing `resource::decode_cp932`. No runtime UI/log sink currently displays
these light names. Five host test comparisons previously assumed UTF-8 names;
they now decode explicitly before comparing with their UTF-8 display literals.
The new resource test separately verifies raw identity, so presentation tests
do not replace the Game-byte contract.

The inaccurate TextEncoding.hpp advice to decode resources for compatibility
comparison was reported to the parent. The parent is incorporating the corrected
comment with the generic encoder work. This patch deliberately excludes that
shared header to preserve its new declarations.

## Consumer trace

| Consumer | Required representation / behavior |
| --- | --- |
| Main BCSV AreaLightName → `_area_light_names` → AreaLightInfo::mAreaLightName | Owned raw CP932, stable until load/reset |
| Zone BCSV AreaLightName → `_zone_area_lights` | Owned raw CP932 used for exact main-table identity |
| `_default_stage_area_light_name`, default_area_light_name() | Same raw CP932, including fallback publication |
| LightFunction::getAreaLightInfo / getDefaultAreaLightName | Return the existing service object / raw name without conversion |
| Original and current LightDataHolder::findAreaLight | Bytewise string lookup; original Game literals must use CP932 |
| LightDirector and ActorLightCtrl | Retain/use AreaLightInfo and its numeric lighting payload; no name display |
| StageLightSceneBinding | Configures authored zone IDs/names and load/reset lifetime; no area-name display |
| AreaObjRealOrAbsentTests | Explicit decode for five human-readable Japanese-name expectations |
| FileSelectFarVisualTests | Only non-null/non-empty default-name check; no encoding change required |
| PointLightRuntimeTests, PlanetMapActorRouteTests, MarioGatewayWalkTests | Read light payload/object presence, not display names |

`consumer-audit.json` records the source search and all direct name uses in the
current native source tree. The original root LightDataHolder/LightZoneDataHolder/
LightFunction methods were read as authority; this scoped patch does not claim
to restore their incomplete native owner implementations.

## Validation

The standalone CPU fixture opens the actual RMGK01 RVZ through the existing DVD
service, reads actual LightData.arc, and runs the staged StageLightData through
the current Game LightFunction accessors. It parses all 200 main names, each
non-ASCII CP932, and all 264 authored zone tables containing LightID (748 rows).
It checks every resolvable row against the exact source bytes, published object
identity, explicit UTF-8 presentation, unchanged name storage, root/child zone
selection, the selected numeric light payload, and reset/unpublication.
The 746 resolved rows cover 179 distinct main names; the remaining 21 main
records are parsed without a matching public zone query.

Two rows in lightfoofightermap.bcsv refer to names absent from the main table:
light 0 `ダミー` and light 1 `テスト２`. Neither is a decoded Unicode alias of a
main name. The test verifies their unchanged first-main-row fallback; it does
not claim those two source names are published as main AreaLightInfo identities.
Unused main records are parsed but are not presented as independently queried
through the public service API.

Staged LightData, the focused fixture, and the updated existing area test all
compile. The focused fixture links and runs without GPU or RuntimeContext
construction. The full existing area test is compile-checked only. This proves
the light-name boundary under the current data service; it does not prove full
original LightDirector initialization, scene lighting ownership, rendering, or
the pending whole-Game CP932 compiler transition. No decoder implementation or
invalid-encoding fallback policy changed.

## Reproduce / integration

```
PYTHONDONTWRITEBYTECODE=1 python3 pc-port/notes/light-name-text-boundary-20260903/stage.py
SMGPC_REAL_DISC=/path/to/disc.rvz PYTHONDONTWRITEBYTECODE=1 python3 pc-port/notes/light-name-text-boundary-20260903/verify-native.py
PYTHONDONTWRITEBYTECODE=1 python3 pc-port/notes/light-name-text-boundary-20260903/freeze.py
```

`native.patch` contains four files: LightData.cpp/.hpp, the updated existing
AreaObjRealOrAbsentTests.cpp and the focused LightNameTextBoundaryTests.cpp.
There are no Game file edits. `native-manifest.json` includes exact destination
and baseline hashes. Parent owns source application and test target selection.
The ROM and executable remain outside versioned notes.
