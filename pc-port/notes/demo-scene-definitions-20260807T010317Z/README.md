# Demo scene definition ingestion and cast membership

Date: 2026-08-07 UTC

## Scope

This slice moves source-style scene demo discovery and cast/action registration into the PC compatibility layer. It intentionally does not add the demo clock, start arbitration, active-part queries, or Action-row dispatch yet.

No production file under `pc-port/src/Game` was edited for this work. The only `src/Game` paths visible in the working tree (`SaveIcon` and `TriggerChecker`) were pre-existing user work and were left untouched.

## Compatibility design

- `DemoSceneRuntime` is a scene-owned `NameObj`, scheduled as `MovementType_DemoDirector` after stage placement resolution and before actor initialization.
- It scans retained `demoobjinfo` placements in stable placement order.
- Every primary `DemoGroup` owns its exact `(zone_id, l_id)`, CP932-decoded `DemoName`, `TimeSheetName`, five stage-switch IDs, source location, and cached `DemoSheetRuntime` loaded from `/ObjectData/DemoSheet.arc`.
- Missing or empty Time definitions remain dormant. No name, route, or sheet aliases are invented.
- Scenes with no primary definitions do not load `DemoSheet.arc`.
- `DemoSubGroup` rows are stored independently and own no sheet.
- Automatic cast registration checks primary definitions first by exact `(actor zone, DemoGroupId)`, then exact subgroup identity. A subgroup retains base membership and forwards to the first primary with the same localized name; it reports failure if no primary exists.
- Explicit-name registration checks primaries first by exact localized name. Only if no primary matches does it join an exact subgroup, without forwarding.
- One actor may belong to multiple primary definitions. Re-registering it in one definition retains the union of Action rows matched by every registration/CastId, matching `DemoActionKeeper::initCast` behavior while keeping one generalized executor membership.
- Callback storage is per primary definition, actor, and exact Action row. Null part names select every targeted row; empty names remain exact empty-string selectors. Functor and nerve fields coexist.
- Runtime installation/removal is stack-safe. Scene teardown removes the scheduler scope first, destroys the demo manager next, then destroys actors.
- Existing programmable-demo global behavior remains available when no scene registry exists, but source-style cast registration requires an active scene registry.

There are no production compatibility checks for specific story routes, actors, or demo names.

## Retail-data observations used for the model

The pre-implementation retail placement audit found:

- 119 unique active demo-object rows: 110 primary `DemoGroup` rows and 9 `DemoSubGroup` rows.
- 325 total scenario occurrences.
- No duplicate resolved `(zone_id, l_id)` identities in one scenario.
- No retail row missing `DemoName` or `TimeSheetName`.
- Three primary definitions have no sheet family (`SpinGetDemo`, `CageBroken`, and `SkeletalFishBossDemo`) and therefore must remain dormant rather than aliasing another sheet.
- 13 co-active duplicate names are primary/subgroup pairs, supporting the primary-first and subgroup-forwarding rules.

The focused real-data test resolves `HeavensDoorGalaxy` scenarios 1 and 2 from `RMGK01.iso`. It verifies three active primaries in scenario 1, the zone-5/link-0 `TicoGuideDemo` sheet with 36 Action rows, a dormant zone-5/link-1 definition, and the sole zone-5/link-2 `ReadStar` definition in scenario 2.

`dolphin-tool header` reports both the local ISO and Korean RVZ as `game_id=RMGK01`, `revision=0`. `RMGK02` is the alternate decomp executable configuration and shares this scene-data format, so the focused suite does not repeat the same assertions against a second multi-gigabyte image.

## Verification

See `verification.log` for the exact commands and captured results.

Final results:

- demo-scene runtime: 7/7 passed, including the optional real-disc case
- self-contained demo-scene runtime from `/tmp`: 7/7 passed with the real-disc case cleanly skipped
- demo-sheet runtime: 11/11 passed
- object-name table: 4/4 passed; extracted RMGK01 and RMGK02 checks passed
- Aurora native: 27/27 passed
- full PC target: built successfully
- route smoke: title, file select, picturebook, and gateway handoff all passed
- `git diff --check`: passed

See `route-smoke.md` and `route-smoke/` for the copied manifest, validator logs, placement report, and visually inspected screenshots. Large SQLite traces and verbose application logs remain in the temporary run directory named by the manifest.

## Deferred follow-up

- Scene-bound demo start arbitration and `isDemoExist`
- Clock advancement and active-part/step queries
- Action-row execution and `isRegisteredDemoActionAppear`/`isRegisteredDemoActionNerve` shims
- Camera/player/wipe/sound sheet dispatch

These are deliberately separate from definition ingestion and registration so subsequent work can use the scene-owned data model instead of route-specific workarounds.
