# FileSelect scenario metadata lifecycle correction

Captured: 2026-08-08T20:38:40Z

## Result

The Aurora host keeps `FileSelect` on the normal `Game` stage lifecycle. It no
longer rejects the retail `FileSelectScenario.arc` merely because its
`ScenarioData.bcsv` has no `Comet` column.

`resolve_stage_scenario_metadata` now maps a missing `Comet` field to the
semantic retail result `StageCometType::None`. This is schema-driven for every
stage: there is no `FileSelect` name check, fabricated field, substitute
archive, or special scene bypass.

## Retail boundary

The root sources establish the behavior:

- `StorySequenceExecutor::moveGalaxy` selects stage `FileSelect`, scenario 1,
  while the host scene remains `Game`.
- `GameSystemSceneController` creates the `Game` scene for that request, so
  FileSelect still uses stage data, placement, session, and audio lifetimes.
- `ScenarioData::getValueString` returns false when a field is absent.
- `GalaxyStatusAccessor::getCometName` converts that result to `nullptr`.
- `CometEventKeeper::isStartEvent` returns false for a null comet name.

The host metadata resolver therefore records the proven no-active-comet result
instead of requiring a galaxy-only schema column at scene construction time.
Unknown archives, missing `ScenarioData.bcsv`, missing scenario rows, undecodable
values, and unknown non-empty comet strings still fail explicitly.

## Regression coverage

`RestartStageSessionTests.cpp` now opens the real RMGK01 disc and resolves both:

- `HeavensDoorGalaxy` scenario 1, whose `Comet` field is present and empty; and
- `FileSelect` scenario 1, whose `Comet` field is absent.

Both produce `StageCometType::None`, while the test retains the distinction
between resolver-proven metadata and an unresolved/default metadata object.
The FileSelect case additionally traverses the exact five comet predicates and
`AudStageBgmWrap`; all predicates are false and the exact stage table proves
there is no initial FileSelect stage BGM.

## Integrated result

The strict FileSelect StageHost probe now parses the scenario and all four
placement rows. Its preflight report records one complete object and three
honestly blocked actors. Construction stops at the existing runtime boundary:

```text
Unsupported placement objects for FileSelect: 3 blocked;
first=InvisibleWall10x10 in jmp/placement/common/objinfo
```

The blocked rows are `InvisibleWall10x10`, `FileSelector`, and
`SphereSelectorHandle`; `GlobalPlaneGravityInBox` is complete. The default route
smoke reaches the same placement preflight at frame 1 and no longer reports a
missing `Comet` field. A visual title/file-select/picturebook route remains
honestly unavailable until those exact placement/actor closures (especially
`FileSelector`) exist.

No `pc-port/src/Game` file was changed.

## Verification

See `verification.log` for the focused test, builds, strict StageHost outcome,
route-smoke outcome, and diff check.
