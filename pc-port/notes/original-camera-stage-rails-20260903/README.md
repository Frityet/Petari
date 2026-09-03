# Original camera stage and rail APIs

This root-only recovery restores the five requested stage/camera APIs and the private authored-rail lookup helper they require. It also corrects a proven return-type error in two existing `StageDataHolder` methods. No PC `Game` source, native service, registry construction, camera selection, or build configuration is changed.

The edited files are:

- `src/Game/Scene/StageDataHolder.cpp`: `getCommonPathInfoElementNum` and the two path-iterator return corrections.
- `include/Game/Scene/StageDataHolder.hpp`: the corresponding two return declarations; no class fields or layouts change.
- `src/Game/Util/SceneUtil.cpp`: the private rail helper, three public rail queries, and current-scenario start CANM lookup. The separate earlier `MR::getStartPosNum` recovery in this file is covered by `../original-camera-registration-20260903/`.

## Original compiler and current retail evidence

`verify-original.py` compiles both complete, real root translation units with GC 3.0a3 and `configure.py`'s `cflags_game` for RMGK01 / VERSION=0. It uses the real include hierarchy without a synthetic replacement header. A separate original-compiler layout probe verifies `JMapInfoIter` size 8 and offsets 0/4, both member-function return signatures, and the StageDataHolder path-table/zone field offsets used by retail.

The verifier resolves every function call, string address, and the GameSystem singleton SDA13 load against `config/RMGK01/symbols.txt` and the current DOL. It checks the actual startup instruction pair establishing r13=`0x806B9620`, verifies all five literal string byte sequences at their actual addresses, then compares every compiled instruction after relocation to the DOL with SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`.

| Method | Retail address | Bytes | Objdiff |
| --- | --- | --- | --- |
| `StageDataHolder::getCommonPathPointInfo` | `0x803473C8` | `0x74` | 99.31035% |
| `StageDataHolder::getCommonPathPointInfoFromRailDataIndex` | `0x8034743C` | `0x8C` | 99.42857% |
| `StageDataHolder::getCommonPathInfoElementNum` | `0x803474C8` | `0x44` | 99.411766% |
| private `getRailInfoFromRailId` | `0x803F70D4` | `0x44` | 100% |
| `MR::getPlacedRailNum` | `0x803F7AF0` | `0x54` | 100% |
| `MR::getCameraRailInfo` | `0x803F7B44` | `0x5C` | 100% |
| `MR::getCameraRailInfoFromRailDataIndex` | `0x803F7BA0` | `0x74` | 99.655174% |
| `MR::getCurrentScenarioStartAnimCameraData` | `0x803F7CA0` | `0x98` | 99.73684% |

All **209 instruction words / 836 bytes** are exactly equal to current retail after relocation. The small objdiff deductions are solely compiler-generated string label names versus the retail split object's labels; the verifier explicitly rejects other mismatch categories and confirms the relocated bytes. `source-evidence.json` retains source/header/compiler/object hashes, commands, all resolved references, and the full per-function results. It does not interpret nonexistent ELF BSS initializer bytes as the live GameSystem singleton value.

## Authored rail behavior and necessary root correction

`StageDataHolder.cpp:154` searches the zone's `CommonPathInfo` for authored `l_id`, starting at row zero, then passes that iterator's physical row index to the second method. `StageDataHolder.cpp:160` formats `CommonPathPointInfo.%d` using that **row index**, writes the point-table pointer, and returns `JMapInfoIter(CommonPathInfo, index)`.

Before this recovery, both declarations incorrectly returned `JMapInfo`, and the second implementation returned `*pInfo`. That copied a table's data/name fields and lost the row index. Retail instead returns the table pointer and row number in r3/r4. The private helper copies that exact pair into `JMapInfoIter` fields 0/4, and the caller then uses the iterator's authored row. The corrected declarations and constructor return compile to all the original instructions; no cast or synthetic result structure is used.

`getCommonPathInfoElementNum` reads the named `CommonPathInfo` entry count for **one zone**. It does not recursively count child zones. The original `JMapInfo::getNumEntries` returns zero for an absent data pointer, but the method assumes the named JMapInfo object exists, as retail does. `MR::getPlacedRailNum` first tests `isPlacedZone`; it returns zero for an unplaced zone and delegates to the selected placed zone's count otherwise.

`MR::getCameraRailInfo` performs the authored `l_id` lookup in the requested zone. `getCameraRailInfoFromRailDataIndex` instead fills the iterator and point-table output from the physical row, then returns `MR::isEqualRailUsage(iter, "Camera")`. Outputs remain populated even when usage is not Camera. There is no remapping of link IDs to camera rail arguments and no fallback rail.

The existing original `CameraRailHolder.cpp:9` visits each zone and physical row, filters through this usage query, then accepts nonnegative authored rail argument 0. Its second pass constructs the actual `RailRider(linkId, zoneId)` and sorts by authored rail argument 0 (`CameraRailHolder.cpp:29`). These different identifiers must remain distinct when the complete registry is activated.

## Scenario animation lookup and owner boundary

`SceneUtil.cpp:236` formats exactly `StartScenario%d.canm` in a 64-byte local buffer using the actual `GameSystemSceneController::getCurrentScenarioNo`. It queries the root stage holder's retained stage archive. A present resource produces its original archive-reported size; an absent resource returns a null pointer and size zero. It does not use the current start zone or selected-scenario UI number, and it does not substitute another scenario's animation.

`CameraDirector.cpp:823` calls this API and declares its original start event animation only when size is positive. The returned pointer is borrowed from the existing stage archive; this helper neither allocates nor assumes ownership. Actual complete CameraHolder/Director and archive lifetimes remain prerequisite work described by `../original-camera-registration-20260903/README.md`. This recovery closes source gaps without constructing a partial holder or activating native camera behavior.

Reproduce with:

```sh
python3 pc-port/notes/original-camera-stage-rails-20260903/verify-original.py
```

Ignored compiler/layout/disassembly/objdiff artifacts are under `build/original-camera-stage-rails-20260903/`. The script requires the verified DOL, configured original compiler, and existing retail split objects. No native/shared build or runtime test was performed for these root-only rail changes.
