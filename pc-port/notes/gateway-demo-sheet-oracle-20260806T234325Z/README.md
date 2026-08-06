# Gateway DemoSheet oracle

Captured 2026-08-06 UTC from repository commit
`6e4776b99acc25077673a6fbd6c4938602e9f7dd` and modified Dolphin commit
`ed8e44d4be114fc70258fbfaeb239f3e83b041fe`.

## Result

Gateway's spin-acquisition sequence is not an executable `SpinGetDemo`
sheet. It is a later starting part of the executable `TicoGuideDemo` sheet:

1. `HeavensDoorMysteriousZone`, scenario-1 layer A, has two `demoobjinfo`
   rows. Link/group ID 0 is demo name `チコガイドデモ` with time-sheet name
   `TicoGuideDemo`; link/group ID 1 is `スピンゲットデモ` with time-sheet
   name `SpinGetDemo`.
2. `DemoSheet.arc` contains the complete seven-table export for
   `TicoGuideDemo`. Its `Time` table has 27 ordered parts. Rows 15 through 26
   are the complete 12-part spin sequence.
3. `DemoSheet.arc` contains no `SpinGetDemo` BCSV, directory, version, or
   progress file. Its sole matching entry is the 8-byte
   `demospingetdemolockfile.txt`, whose content is ASCII `motokurk`.
4. Lock files are authoring metadata, not runtime aliases: the corresponding
   Tico guide lock is the ASCII username `sugawara_hideyuki`. Nothing in the
   runtime parser reads a lock file, and `DemoFunction::loadDemoArchive()`
   fixes the runtime source to `ObjectData/DemoSheet.arc`.
5. The original gameplay call is explicit:
   `RosettaDemoHeavensDoor.cpp:118` starts demo `チコガイドデモ` at part
   `スピンゲット[デモ1]`. There is no source reference to demo name
   `スピンゲットデモ` or sheet name `SpinGetDemo`.

The best-supported historical explanation is that `SpinGetDemo` is a stale
or unfinished editor group whose exported data was folded into the larger
Tico guide sheet. The exact authoring history is not recoverable here, but the
runtime ownership and actual sheet location are unambiguous.

## Placement mapping

`DemoGroupId` on an actor is matched to `l_id` on `demoobjinfo`, scoped by
placed zone. This is the `JMapIdInfo` comparison used by `DemoCastGroup`; it is
not a global integer key.

All non-empty HeavensDoor `demoobjinfo` tables are in
[`gateway-demo-groups.csv`](gateway-demo-groups.csv). The scenario-1 Gateway
cast of interest is:

- Group 0 / `TicoGuideDemo`: placed Tico cast 0, DemoRabbit casts 0/1/2,
  Rosetta cast -1, TicoBaby cast 0, HeavensDoorAppearStepA cast -1, plus child
  RunawayTico casts 0/1/2. A negative CastId is valid; it is a wildcard in
  action matching, not a registration failure.
- Group 1 / `SpinGetDemo`: only the placed `RunawayRabbitCollect`, cast -1.
  Its source registers as a cast but never starts a time-keep demo. It turns
  switch A on after all rabbits are caught.

## Archive inventory and exact tables

[`archive-inventory.csv`](archive-inventory.csv) is the complete set of RARC
entries beginning with `demoticoguidedemo` or `demospingetdemo`. The executable
Tico guide export contains:

- `Time`: 27 rows, 3 fields, 992 bytes
- `Action`: 36 rows, 6 fields, 1600 bytes
- `Player`: 5 rows, 3 fields, 352 bytes
- `Camera`: 2 rows, 7 fields, 224 bytes
- `Wipe`: 3 rows, 4 fields, 192 bytes
- `Sound`: 0 rows, 5 fields, 96 bytes
- `SubPart`: 0 rows, 4 fields, 64 bytes

The decoded BCSV descriptor layout is in
[`table-schema.csv`](table-schema.csv). Raw field hashes, masks, offsets,
shifts, and types were read directly from the file descriptors. BCSV type 0
is signed 32-bit and type 6 is string-table offset.

## Part order and duration

[`tico-guide-time.csv`](tico-guide-time.csv) is the exact 27-row order. Starting
at `スピンゲット[デモ1]` selects row 15 and continues through row 26. All spin
parts have `SuspendFlag=0` and total 2551 game frames (about 42.52 seconds at
60 Hz) before any external talk/runtime effects:

| Part | Spin-relative frames | Duration |
| --- | ---: | ---: |
| スピンゲット[デモ1] | 0-419 | 420 |
| スピンゲット[会話1] | 420-539 | 120 |
| スピンゲット[デモ2] | 540-689 | 150 |
| スピンゲット[会話2] | 690-809 | 120 |
| スピンゲット[デモ3] | 810-1049 | 240 |
| スピンゲット[会話3] | 1050-1169 | 120 |
| スピンゲット[デモ4] | 1170-1669 | 500 |
| スピンゲット[デモ5] | 1670-1729 | 60 |
| スピンゲット[会話4] | 1730-1849 | 120 |
| スピンゲット[デモ6] | 1850-2219 | 370 |
| スピンゲット[デモ7] | 2220-2549 | 330 |
| スピンゲット[デモ8] | 2550 | 1 |

`DemoTimeKeeper` exposes steps `0..TotalStep-1`. On the next update it either
selects the following part at step 0 or ends before dispatching another row.
This gives each table duration exactly the stated number of ticks.

## Dispatch data

The complete action rows are in
[`tico-guide-action.csv`](tico-guide-action.csv); player, camera, and wipe rows
are in [`tico-guide-other-tables.csv`](tico-guide-other-tables.csv). The spin
subset does the following at each matching part's first step:

- Demo 1: action type 2 invokes registered functors for Rosetta and TicoBaby;
  both rows also carry `MarioDemoPos4`. Player data places Mario at
  `MarioDemoPos4`.
- Conversations 1-4: action type 9 invokes Rosetta's no-pause,
  Mario-puppetable talk controller.
- Demo 8: Tico appears (type 0); Rosetta turns switch B off (type 13);
  LightDome, Rosetta, and TicoBaby die (type 1).
- Wipes: demo 1 opens `フェードワイプ` with frame -1, demo 7 closes it over
  120 frames, and demo 8 opens it over 60 frames. Original
  `DemoWipeKeeper` maps types 0/1/2/3 to open/close/force-open/force-close.

The generic Action schema is dispatched at the first part step, then optional
`AnimName` and `PosName` are applied. `CastName` must equal the actor's runtime
name; `CastID` restricts only when it is nonnegative. Action types from the
original keeper are:

| Type | Operation |
| ---: | --- |
| 0 | make actor appeared |
| 1 | make actor dead |
| 2 | invoke registered functor |
| 3 | set registered nerve |
| 4 / 5 | switch A / B on |
| 6 / 7 | show / hide model |
| 8 | start time-keep talk, Mario-puppetable |
| 9 / 10 | start no-pause time-keep talk, Mario-puppetable |
| 11 | no operation |
| 12 / 13 | switch A / B off |

At a type-9 row's last step the original keeper restores talk-animation
interpolation. Types 2 and 3 require a matching registered callback; silently
inventing a fallback would hide a missing cast or source implementation.

## Generalized host design

The compatibility implementation should preserve the original data model,
with no Gateway names or fixed timers:

1. Load `ObjectData/DemoSheet.arc` once through the existing resource/RARC
   service. For each active `demoobjinfo` row, retain `(zone_id, l_id)`,
   localized `DemoName`, `TimeSheetName`, and stage switches.
2. Resolve the seven optional table names as
   `Demo<TimeSheetName><Suffix>.bcsv`, case-insensitively in the RARC. Parse
   `Time`, `SubPart`, `Player`, `Camera`, `Action`, `Wipe`, and `Sound` by BCSV
   field hash, not field order.
3. Treat a missing/zero-row `Time` table as a valid dormant executor
   definition but not an executable demo. Do not alias `SpinGetDemo` to
   `TicoGuideDemo`; the original caller selects TicoGuide by localized demo
   name. Reject an attempted start with a semantic trace instead of indexing
   an empty part array.
4. Register casts by `(placed_zone_id, DemoGroupId)` against the executor's
   `(zone_id, l_id)`. Preserve CastId -1. Also support the original explicit
   localized-name registration overload used by Rosetta.
5. Active state is `{executor, part_index, part_step, paused}`. A start-part
   request performs an exact part-name lookup. Each scene tick advances the
   time keeper, then dispatches in original order: subparts, player, camera,
   action, wipe, sound. Expose part state during callbacks so actor code such
   as `TicoDemoGetPower::exeDemo()` observes step 0.
6. Execute action rows against all matching registered casts and route
   built-in actor/switch/model operations through existing compatibility APIs.
   Registered functors and nerves remain actor-owned and are released with
   actor/scene teardown.
7. Implement `MR::isDemoPartActive`, first/last/step/total queries, pause and
   resume, and clean end semantics over that same active state. This lets the
   source actors own frame-specific effects and spin entitlement exactly as
   on Wii.

## Next implementation slice and blockers

The smallest useful slice is a compatibility-owned `DemoSheetRuntime` plus
unit tests using these exact extracted tables:

1. resource loader and all seven schema parsers;
2. zone-scoped demo-group/cast registration, including CastId -1;
3. start-at-part and exact 2551-frame time keeper;
4. action type 0-13 dispatch, player row dispatch, and wipe dispatch;
5. MR part-query/start/pause/end shims and semantic tracing;
6. tests for a missing `SpinGetDemo` Time table, group-name resolution, step
   boundaries, action ordering, wildcard CastId, and cleanup.

For the visible Gateway sequence, source-close Rosetta/Tico/TicoDemoGetPower
and talk-controller support are still required consumers. In particular,
spin permission is granted by actor code at the first step of
`スピンゲット[デモ5]`; it is not a table action and must not be replaced by a
stage-specific timer. Camera/talk fidelity can follow after the time/action
core, because the extracted spin subset has no Camera/Sound/SubPart rows but
does have four talk actions.

## Revision and extraction confidence

Both local RMGK01 representations (raw ISO and RVZ) identify as revision 0.
The three relevant files extracted from each are byte-identical. The local
RMGK02 revision-1 extract contains system files but not the data-file bodies;
its FST reports the same sizes as RMGK01 for `DemoSheet.arc` (43456),
`HeavensDoorMysteriousZone.arc` (14688), `HeavensDoorGalaxy.arc` (7328),
`HeavensDoorInsideZone.arc` (7104), and the scenario archive (544). Equal FST
sizes strongly support revision-invariant data but are not a byte-identity
proof for RMGK02. The exact hashes and probe/tool hashes are in
[`evidence.sha256`](evidence.sha256).

Reproduction commands are recorded in [`commands.md`](commands.md).
