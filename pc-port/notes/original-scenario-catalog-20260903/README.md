# Original scenario catalog accessors and collision ownership boundary

Restored four missing methods in root `src/Game/System/ScenarioDataParser.cpp`. This is original-source recovery only: no PC source import, native collision activation, replacement catalog, or fabricated camera owner was added.

## Evidence

The verified RMGK01 rev0 DOL has SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`. Run from the repository root:

```sh
python3 pc-port/notes/original-scenario-catalog-20260903/verify-source.py
```

The script uses the repository's GC3.0a3 compiler, Shift-JIS wrapper, and `configure.py` Game flags, splits the verified DOL, compares original objects with objdiff, and validates every restored instruction and relocation. Evidence is in `source-evidence.json`; reproducible compiler commands, objects, and comparison output are under ignored `build/original-scenario-catalog-20260903/`.

| Method | Retail address / bytes | Objdiff | Verification |
| --- | --- | --- | --- |
| `getScenarioNum` | `0x803A8CE8` / 152 | 99.73684% | Relocated compiled bytes equal retail |
| `getPowerStarNum` | `0x803A8D80` / 152 | 99.73684% | Relocated compiled bytes equal retail |
| `getScenarioDataIter` | `0x803A8ED0` / 220 | 88.454544% | Complete instruction correspondence after one verified inline expansion |
| `getValueBool` | `0x803A9468` / 164 | 100% | Relocated compiled bytes equal retail |

The iterator's current compiler context inlines `JMapInfo::getValue<s32>`, adding five caller instructions. The verifier checks the complete 100-byte retail callee at `0x800B8BE8`, exact argument identities, search/read/negative-index behavior, and the precise ten-instruction expansion before comparing all remaining caller instructions. Its ignored boolean return and private ABI save/restore disappear when inlined. No algorithmic instructions or branch conditions are discarded. The named local iterator retains the original iterator copy and stack behavior.

String relocation bytes were read directly from the DOL: `IsHidden` at `0x805DBF63`, `PowerStarId` at `0x805DBF6C`, and `ScenarioNo` at `0x805DBF81`. Together the restored methods cover 688 retail bytes / 172 instructions.

## Preserved semantics

- Scenario lookup uses `findElement("ScenarioNo", scenarioNo, 0)`, returning the first matching authored ID independently of physical row order. Missing IDs return the map's end iterator. The original template ignores the per-row getter's boolean result, so valid data must contain the `ScenarioNo` field.
- Counts iterate numeric scenario IDs from 1 through the scenario table's entry count. Scenario count excludes rows with `IsHidden`; star count counts nonzero `PowerStarId`. Locals start at false/zero and the original count methods ignore getter success.
- `getValueBool` returns false without changing the output when the field is absent. A present field delegates to original `JMapInfo::getValueFast(bool*)`, which tests the masked raw word. There is no added iterator validity check, row fallback, type conversion, or synthesized field value.

## Minimal genuine native collision construction chain

The root `CollisionDirector` constructor, keeper constructor/zone allocation, `CollisionZone`, and direct ZoneList-count path already exist. The preceding root recovery checkpoints supply keeper sphere/thickness/line broad phase, KCL traversal/narrow phase, and part query bodies. The missing native boundary is owner construction and publication, not an alternate collision algorithm.

1. Install a real scenario catalog through the original GameSystem/SceneController/parser ownership path. `MR::getZoneNum` obtains the current galaxy's `ScenarioData` and reads its complete `mZoneList` entry count. `ScenarioDataParser` enumerates `/StageData`, discovers scenario archives, creates actual `ScenarioData` entries, and sorts the catalog. Each `ScenarioData` retains its galaxy name, actual archive, and two actual `JMapInfo` tables attached to `ScenarioData.bcsv` and `ZoneList.bcsv`. The recovered iterator also closes ordinary current-scenario field/layer lookup.
2. Native `StagePlacementResolver` currently decodes ZoneList locally for placement provenance; that local table is not a retained original `ScenarioDataParser` catalog. Its placed-zone count must not substitute for the complete authored catalog. Native `StageHostScene` already creates an actual `PlacementStateChecker`, and its compatibility accessors read/write that real object.
3. Install a real scene `CollisionDirector`, which creates the collision code and four categorized keepers and registers the original collision movement category. Native `SceneObjHolderCompat`/`StageHostScene` currently do not construct that director. The existing native `MR::getCollisionDirector` expects the actual scene object. Keeper zones are lazily allocated against the original authored zone count (the original owner has a 32-zone array).
4. Construct real placed `CollisionParts` from retained typed KCL/JMap resources. Existing `KCollisionResource` ownership is useful here, but native registered triangles still do not establish this full part lifecycle. Original initialization establishes sensor, current/previous/inverse matrices, keeper/category, zone, farthest radius, and camera polygon-code registration. Validate/invalidate and movement order must remain original; triangle part pointers/local prism IDs must remain valid until consumers release them.
5. Satisfy actual camera-code registration ownership before invoking original part initialization. That initialization unconditionally calls camera-code collection with the sensor host name and placement zone. Root `CameraPolygonCodeUtil` and `GameCameraCreator` are already complete: collect codes below 255, create zone/name/code group IDs, and create parameter chunks. A chunk constructor queries its actual `CameraHolder` default index. The original holder creates the complete camera/translator table; the native selected-controller wrappers are not that holder or a complete `CameraDirector`. A dummy director, partial holder, fabricated default index, or no-op collector would bypass authored camera registration.

The native original resource-holder migration is concurrent parent-owned work. The legacy `ResourceHolderCompat.hpp` boundary must not be confused with a complete real archive/model/resource owner. Original scene destructors also assume scene-heap ownership; typed part/zone/catalog construction needs retained storage and destruction order rather than null or stack-cast stand-ins.

No native build or runtime claim belongs to this source-only checkpoint. Remaining point/area/same-host query methods and full camera/director startup are separate closures.
