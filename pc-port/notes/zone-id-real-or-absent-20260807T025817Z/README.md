# Zone IDs: real or absent

## Scope

Removed the synthetic zone-ID allocator from `StagePlacementResolver`. Placement
data now carries only IDs that exist as row indices in the active scenario's
`ZoneList.bcsv`.

No file under `pc-port/src/Game` was changed.

## Retail audit

RMGK02's generated assembly and the root decomp show the following path:

- The top-level `StageDataHolder` is constructed with zone ID `0`.
- `StageDataHolder::createLocalStageDataHolder` reads each `StageObjInfo` name
  and calls `GalaxyStatusAccessor::getZoneId` before constructing the child
  holder.
- `ScenarioData::getZoneId` scans `ZoneList.bcsv` in row order with a
  case-insensitive name comparison and returns the matching row index.
- Retail returns `0` when the scan misses. That value is ambiguous with the
  real root row, so it is not evidence for allocating any new ID.

The removed port-only path instead started at `ZoneList`'s row count and
assigned increasing IDs to names that were not present. No equivalent exists
in the audited retail path.

Relevant source/oracle locations:

- `src/Game/System/ScenarioDataParser.cpp`, `ScenarioData::getZoneId`
- `src/Game/Scene/StageDataHolder.cpp`, child-holder creation call sites
- `build/RMGK02/asm/Game/Scene/StageDataHolder.s`,
  `createLocalStageDataHolder__15StageDataHolder...`
- `build/RMGK02/asm/Game/System/ScenarioDataParser.s`,
  `getZoneId__12ScenarioDataCFPCc`

## Resulting policy

- Zone lookup returns `std::nullopt` for an empty or missing name.
- The requested root is accepted only if `ZoneList` resolves it to row `0`,
  matching the retail top-level holder identity.
- A missing scenario archive or `ZoneList.bcsv` yields no placement tables.
- A `StageObjInfo` child whose name is absent from `ZoneList` is skipped before
  its archive can contribute tables, transforms, GeneralPos rows, or actors.
- Valid root, layer, child-zone, transform, rail, and GeneralPos traversal is
  otherwise unchanged.

## Verification

`xmake run smg-pc-demo-scene-runtime-tests` passed all 16 tests, including:

- deterministic `ZoneList.bcsv` row-index, case-folding, and missing-name
  coverage;
- existing root-before-child GeneralPos ordering and composed-transform
  coverage;
- the real RMGK01 HeavensDoorGalaxy scenario-1 traversal, which retained all
  three demo definitions and all seven required GeneralPos rows.

See `verification.log` for the command output.
