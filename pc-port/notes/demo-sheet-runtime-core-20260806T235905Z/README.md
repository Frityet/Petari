# DemoSheet runtime core

Captured 2026-08-06 UTC from repository commit
`1a562d904d92eea6d76c2918ffcd2b2e588d38bb`.
Revised 2026-08-07 UTC against `9136b4194` after archive-wide schema
validation.

## Result

Added a compatibility-owned, data-driven `DemoSheetRuntime` core. It does not
dispatch actors and does not modify the existing `DemoCompat` or `MR` APIs.
The new boundary is deliberately limited to parsing source data and tracking
main-part time:

- resolves `Demo<TimeSheetName>{Time,SubPart,Player,Camera,Action,Wipe,Sound}.bcsv`
  by case-insensitive RARC basename;
- treats every table as optional and records the difference between a missing
  table and a present zero-row table;
- validates nonempty tables by BCSV field hash and compatible field type,
  without depending on descriptor or entry-field order; zero-row tables need
  no descriptors;
- requires only the fields consumed without a source constructor fallback:
  Time `PartName`; SubPart `SubPartName`/`MainPartName`; Player
  `PartName`/`PosName`/`BckName`; and `PartName` for Camera, Action, Wipe, and
  Sound;
- preserves source constructor defaults for absent optional columns, including
  one-step Time/SubPart rows, Camera/Action sentinels, UTF-8
  `フェードワイプ` with frame -1, and Sound wipeout frame -1;
- preserves all source fields required by the original time, subpart, player,
  camera, action, wipe, and sound keepers, including signed `-1` sentinel
  values;
- converts source CP932/Shift-JIS strings to UTF-8 through the reusable
  resource-layer `decode_cp932` helper, using Windows conversion APIs on
  `_WIN32` and iconv elsewhere, so original Japanese part names compare
  exactly with PC source/API strings; the existing lossy UTF-8/UTF-16 helpers
  remain unchanged;
- rejects attempts to start a missing or empty Time table and reports a
  missing exact part name without indexing invalid storage;
- exposes active, paused, current index/name/step/total, first-step,
  last-step, last-part, and final-demo-step queries.

`advance()` mirrors the source `DemoTimeKeeper` frame boundary: a part exposes
steps `0..TotalStep-1`; the boundary update selects the next part at step 0;
and the update after the final dispatchable step ends without exposing another
row. A `SuspendFlag` row ends at its own duration boundary rather than
continuing into the next row.

## Real-data verification

The focused test uses the extracted RMGK01 archive when it is available at
`container/orig/RMGK01/files/ObjectData/DemoSheet.arc` (SHA-256
`975b6c99ec882c5e56f24e4684859692fc98613dc0e021834e9895b46eae8b6d`).
The timing, missing/empty-table, case-insensitive lookup, and malformed-schema
tests use generated BCSV/RARC fixtures as well, so the test target remains
self-contained when proprietary disc data is absent. It verifies:

- all seven TicoGuideDemo tables resolve despite lower-case archive paths;
- all 138 Time-sheet families discovered in the real archive load through the
  same generalized parser;
- exact row counts: Time 27, SubPart 0, Player 5, Camera 2, Action 36,
  Wipe 3, Sound 0;
- representative signed and string fields from Action, Player, Camera, and
  Wipe;
- exact part-name lookup selects Time row 15;
- rows 15 through 26 expose exactly 2551 dispatchable frames, with row 15
  steps 0 through 419 and final row 26 step 0 visible before clean end;
- the lock-only SpinGetDemo export rejects start as `missing-time-table`;
- a descriptorless, zero-row Time table parses and rejects start as
  `empty-time-table`;
- a self-contained CP932 Japanese Time row decodes to UTF-8 and receives the
  source `TotalStep=1`/`SuspendFlag=false` defaults;
- minimal Camera, Action, Wipe, and Sound tables preserve every source
  constructor default when optional columns are absent;
- a nonempty Time table missing required `PartName`, and a Camera table whose
  present optional frame field has the wrong type, produce contextual schema
  errors;
- pause mirrors the original early-step correction (`-1` to `0` to `1`),
  then freezes until resume;
- `is_last_part` and the derived final-demo-step query are false while paused,
  including on the final valid step;
- a suspended part exposes its final valid step and ends on the following
  update before the next row can dispatch.

Commands and results:

```text
$ xmake build smg-pc-demo-sheet-runtime-tests
build ok

$ xmake run smg-pc-demo-sheet-runtime-tests
[ok] optional real Tico guide schema and rows
[ok] optional real archive all Time families
[ok] case-insensitive lookup and exact part name
[ok] CP932 Time part and source defaults
[ok] sparse optional columns use source defaults
[ok] spin subset timekeeper boundaries
[ok] pause matches source early-step correction
[ok] paused final step is not last part
[ok] suspend ends before following dispatch
[ok] missing and empty Time reject start
[ok] malformed schemas are contextual errors
11 DemoSheet runtime test(s) passed

$ /workspaces/pcport/pc-port/build/linux/x86_64/debug/smg-pc-demo-sheet-runtime-tests  # from /tmp
[skip] extracted RMGK01 DemoSheet schema check
[skip] extracted RMGK01 archive-wide DemoSheet check
11 DemoSheet runtime test(s) passed

$ xmake build smg-pc
build ok
```

## Scope left for the next slice

This core intentionally does not register casts, dispatch table rows, drive
Mario/camera/wipe/audio, or install Game-facing demo queries. Those consumers
can now use one parsed representation and one source-faithful clock without
introducing Gateway, Tico, Spin, route, or localized-name special cases.

Current source hashes:

```text
f3737e73fa02d01dd1453e09b92867a204b7db5dee260b8d0e6d20fbfabb6bc6  src/compat/DemoSheetRuntime.cpp
db800e3ecf434d83786967ce589be8dbcc7029cc1b139d371cc76e187db5802e  src/compat/DemoSheetRuntime.hpp
d19749abe8b22f869835467902ddfe92a38a04d96479867ad01264b52c1eb174  src/resource/TextEncoding.cpp
bad39cfeed64ad73cb4ce03db69b6bcb78b86bb2aafe1ab8c02f1f7475fa29ca  src/resource/TextEncoding.hpp
802b7851a191b70e1d674c6d19ea6be68311099f75ed37966ddfcd775ae74935  tests/DemoSheetRuntimeTests.cpp
```
