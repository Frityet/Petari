# Demo scene clock and MR query compatibility

## Outcome

The scene-owned `DemoSceneRuntime` now advances each primary definition's parsed
DemoSheet using a generalized compatibility implementation of the original
`DemoTimeKeeper` and `DemoSubPartKeeper`. The public MR time-keep start, active,
part/step, pause/resume, and natural-end surfaces are routed to that runtime.

All production changes are under `pc-port/src/compat`. This slice did not edit
`pc-port/src/Game`, the root decompiled `Game`, stage route policy, title, file
select, or picturebook code. The pre-existing dirty `SaveIcon` and
`TriggerChecker` files were left untouched.

## Implemented behavior

- Main parts start selected at step `-1`, expose steps `0..TotalStep-1`, select
  the next row at step zero, and end on the following movement.
- `SuspendFlag` ends on its total-step boundary before a following row can
  dispatch.
- Paused updates retain the original early correction (`-1 -> 0 -> 1`) and then
  freeze. SubPart dispatch is skipped while paused.
- `DemoTimeKeeper::end` clears the selected part and counters but deliberately
  preserves the pause flag.
- SubParts update in BCSV order, decrement before testing their trigger, can be
  triggered from a main part or an earlier SubPart, span main-part boundaries,
  and use first-row lookup for duplicate names.
- Main-part lookup has precedence over a same-name SubPart.
- TimeKeeper movement happens before SubPart movement. A natural end returns
  before SubPart or any future keeper dispatch.
- The original one-frame-final pause edge is retained: pausing at final step
  zero corrects to one; resuming reaches step two, fails the source's
  `TotalStep >= currentStep` end comparison, retains the final part pointer,
  and continues until an explicit end. This is covered and traced rather than
  normalized away.
- `isDemoExist` is definition-based, including dormant definitions with missing
  or empty Time tables, and excludes subgroup-only definitions.
- Actor queries use the first primary membership in registration order.
- Registered starts preserve the original two-stage lookup: actor membership is
  found first, then its exact demo name is resolved through the ordered holder.
  Duplicate names therefore resolve to the first matching definition.
- Ordinary registered starts select Mario-puppetable ownership when the
  actor-selected executor has any parsed Player rows. The explicit Mario
  overload always selects it.
- Actor pause/resume mirrors `findDemoExecutorActive`: it scans ordered actor
  memberships whose demo name compares active. With duplicate names, this can
  pause an inactive same-name executor exactly as the source does.
- Active-part predicates guard inactive state. Rates use `step / TotalStep`
  without changing the denominator or clamping valid values.
- Clock natural end and explicit end both release shared MR demo activity and
  puppetable player-control ownership. Like `DemoDirector`, explicit end treats
  its owner and name arguments as informational and cannot leave a clock locked
  by a mismatched caller.
- The void time-keep APIs retain the established programmable/global fallback
  when no scene registry exists; try APIs still reject. If a registry exists,
  invalid definition, Time, or part requests do not fall back.

## Safe host normalizations

Several original direct helpers assume valid pointers and can crash on invalid
names or absent executors. The compatibility layer keeps valid-input behavior
but uses stable sentinels for host safety:

- unknown/inactive direct part step: `-1`
- unknown/inactive direct part total: `0`
- invalid/zero-total step rate: `0.0f`
- null or unknown current-main-part query: `nullptr`
- null actors/names and missing/empty Time tables: rejected/no-op

Failed safe starts do not claim global demo activity. These normalizations are
generic and are not keyed to any stage, actor, route, or localized demo name.

## Source and data oracle

The implementation was checked against:

- `../src/Game/Demo/DemoTimeKeeper.cpp`
- `../src/Game/Demo/DemoSubPartKeeper.cpp`
- `../src/Game/Demo/DemoExecutor.cpp`
- `../src/Game/Demo/DemoFunction.cpp`
- `../src/Game/Demo/DemoDirector.cpp`
- `../src/Game/Util/DemoUtil.cpp`

The optional real-data tests load the extracted RMGK01 `DemoSheet.arc`, validate
all 138 Time families, and exercise the real TicoGuide schema/row boundaries.
The RMGK01 extract and root RMGK02 extract are the same hard-linked file (inode
`469944`, size `43456`, SHA-256
`975b6c99ec882c5e56f24e4684859692fc98613dc0e021834e9895b46eae8b6d`), so they
are not independent regional data evidence.

## Verification summary

- DemoSheet runtime: 11/11 passed.
- DemoScene runtime: 12/12 passed.
- Aurora-native compatibility regression suite: 27/27 passed.
- Full debug `smg-pc` executable build passed.
- Route regression passed at title frame 90, file-select frame 1900,
  picturebook frame 7600, and Gateway-handoff frame 10350. All four captures
  are byte-identical to the prior approved route baseline.
- Independent source-fidelity review approved the final tree with no remaining
  high- or medium-priority findings.
- `git diff --check` passed for the scoped files.

See `verification.log` for command-level evidence and `oracle-matrix.csv` for
the behavior-to-source/test map. See `route-smoke.md` for route artifacts and
`review.md` for the independent review result.
