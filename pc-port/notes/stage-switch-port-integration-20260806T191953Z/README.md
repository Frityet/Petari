# Zone-aware StageSwitch PC integration

Updated: 2026-08-06T19:19:53Z

## Outcome

The PC port now runs the original zone-aware stage-switch implementation instead of the inherited raw integer-keyed compatibility map. HeavensDoor scenario 1 constructs and schedules its two `SwitchSynchronizerReverse` placements and one `GroupSwitchWatcher` through original factory entries. A clean 360-frame FileSelect-to-HeavensDoor run exited normally with no fatal/crash match.

This removes three placement blockers and reduces the real HeavensDoor blocker frontier from 27 to 24 without adding an actor-name, stage-name, or route-specific compatibility branch.

## Source-close game code

The following PC files are direct, source-equivalent copies of the current root decomp sources (apart from an inconsequential final-newline difference in one translation unit):

- `Game/Map/StageSwitch.{hpp,cpp}`
- `Game/Map/GroupSwitchWatcher.{hpp,cpp}`
- `Game/Map/SwitchSynchronizer.{hpp,cpp}`
- `Game/Map/ActorAppearSwitchListener.{hpp,cpp}`
- `Game/Map/SwitchWatcher.{hpp,cpp}`
- `Game/Map/SwitchWatcherHolder.{hpp,cpp}`
- `Game/Util/ActorSwitchUtil.{hpp,cpp}`
- `Game/Util/SwitchEventFunctorListener.{hpp,cpp}`
- `Game/Util/JMapIdInfo.{hpp,cpp}`
- `Game/Util/Array.hpp`
- `Inline.hpp`

The watcher units were audited against RMGK02 immediately before import and are 100% across all functions, code, and data. The StageSwitch root audit is recorded separately in `stage-switch-decomp-20260806T190606Z`.

`SleepController.{hpp,cpp}` and `SleepControllerHolder.hpp` are also original copies. The PC `SleepControllerHolder.cpp` preserves the original holder and LiveActor behavior but intentionally omits the unreferenced `AreaObj` overload until the general AreaObj/JGeometry substrate is ported; it does not fake area behavior.

The existing PC `ObjUtil` no longer owns switch records, a process-global raw-ID state map, custom listener matching, or switch traces. Original `ActorSwitchUtil` delegates to `LiveActor::mStageSwitchCtrl`, and original `SwitchWatcherHolder` performs edge delivery. Original name-object listener construction remains in `ObjUtil`, matching the game boundary.

## General compatibility work

### Canonical placement-zone identity

`JMapInfo` now retains a placed-zone ID as host metadata. `StagePlacementResolver` loads the scenario archive's `ZoneList.bcsv` once and resolves child zone names to the same row IDs used by original `ScenarioData::getZoneId`. Every copied/transformed placement row retains that ID, and the narrow PC `MR::getPlacedZoneId` bridge returns it to original `JMapIdInfo` code.

If a stage lacks a ZoneList entry, one resolver-wide, case-insensitive name map assigns monotonic fallback IDs after the canonical range. This avoids the previous `entry_index + 1` collision across layers and nested parents.

RMGK01 and RMGK02 contain byte-identical HeavensDoor scenario and zone archives. The verified mapping is:

| ID | Zone |
| ---: | --- |
| 0 | `HeavensDoorGalaxy` |
| 1 | `HeavensBlackHoleZone` |
| 2 | `HeavensDoorInsideZone` |
| 3 | `HeavensDoorLargeZone` (listed but unplaced in scenario 1) |
| 4 | `HeavensDoorMiddleZone` |
| 5 | `HeavensDoorMysteriousZone` |
| 6 | `HeavensDoorSmallZone` |

The debug placement report and semantic placement trace now expose `zone_id`, making this general compatibility invariant observable.

### Host standard-library boundary

Metrowerks' standard library names its member-function adapter `std::mem_func`; modern host C++ exposes `std::mem_fn`. `compat/MetrowerksStdCompat.hpp` supplies the former in terms of the latter and is force-included for the PC game target. This keeps the exact watcher loop intact and provides a reusable boundary for later original sources.

### Scene objects and lifetime

The PC scene-object holder now lazily owns IDs `0x0A`–`0x0C`, matching the original `StageSwitchContainer`, `SwitchWatcherHolder`, and `SleepControllerHolder` IDs. The general runtime scene-registration scope removes their scheduler registrations before roots and holders are destroyed; no switch actor teardown special case was added.

After every root receives `initAfterPlacement`, `StageHostScene` invokes the original `SleepControlFunc::initSyncSleepController` boundary at the same lifecycle point as `GameScene`. This initializes any registered sleep listener from its current switch value and is independent of stage or actor name.

## Automated verification

```text
xmake build smg-pc
[100%]: build ok

xmake test
[ok] stage switch zone identity and edges
10 Aurora-native test(s) passed
100% tests passed
```

The new native test proves:

- local switch 7 in zone 1 does not alter local switch 7 in zone 2;
- global switch 1007 shares one bank across those zones; and
- a `SwitchWatcher` emits exactly one rising and one falling callback, with no steady-state duplicate.

## Real-disc runtime evidence

Artifacts: `/tmp/smgpc-switch-clean.aAsXiT/`

```text
input_sent=1
app_result=0
wrapper_result=0
FileSelect scheduler cleanup: 78 -> 2 (76 removed, including the sleep holder)
HeavensDoor placement: objects=242;created=146;ignored=72;blocked=24
SwitchSynchronizerReverse: 2, status=created, support_reason=original_factory
GroupSwitchWatcher: 1, status=created, support_reason=original_factory
observed zone pairs: Galaxy=0, BlackHole=1, Inside=2, Middle=4, Mysterious=5, Small=6
fatal/segmentation/crash/abort matches: 0
```

The remaining blocker set is now:

| Count | Object |
| ---: | --- |
| 8 | `CollisionBlocker` |
| 8 | `Steam` |
| 3 | `DemoRabbit` |
| 2 | `RailCoin` |
| 1 | `YellowChipGroup` |
| 1 | `StarPieceGroup` |
| 1 | `FlipPanelObserver` |

These are separate actor/runtime slices. `RunawayRabbitCollect` is not in that blocked table because the current placement resolver still aliases it to an inert model; the collector/Tico/rabbit stack remains the larger source-close gameplay dependency documented in `runaway-rabbit-collect-decomp-20260806T190520Z`.
