# Retail Demo Action/Wipe dispatch

Date: 2026-08-07 UTC

## Source oracle

The implementation was derived from the regular decomp's
`DemoExecutor::movement` and `DemoActionKeeper`, plus the RMGK02
`DemoWipeKeeper` behavior. It remains in the generalized demo compatibility
runtime; this slice does not edit `pc-port/src/Game` and contains no actor,
stage, demo-name, or route-name branches.

Retail keeper order after Time/SubPart is Player, Camera, Action, Wipe, Sound,
then TalkAnim. `DemoSheetRuntime::advance()` now decides whether those keeper
slots dispatch on a tick. A paused/corrected tick does not execute or invent a
missed first-step operation.

## Action behavior installed

Action rows remain in BCSV order and target their registered cast rows. On a
part's first step, the compatibility runtime performs the original operation,
then `AnimName`, then `PosName`:

| Type | Operation |
| ---: | --- |
| 0 | appear |
| 1 | dead |
| 2 | registered functor |
| 3 | registered nerve |
| 4 / 5 | switch A / B on |
| 6 / 7 | show / hide model |
| 8 | time-keep talk with puppetable Mario |
| 9 / 10 | time-keep talk without pausing puppetable Mario |
| 11 | no operation |
| 12 / 13 | switch A / B off |

Unknown types still retain the retail `AnimName`/`PosName` ordering. `PosName`
now resolves against the active scenario's real `jmp/GeneralPos` tables in
retail root-zone, layer, archive-entry, then child-zone order. CP932 names are
decoded consistently with DemoSheet names, and child-zone positions and Euler
rotations use the composed stage placement transform. A missing
source-required type-2 functor, type-3 nerve, or talk controller throws a
diagnostic instead of silently substituting behavior. An absent named
`GeneralPos` also fails explicitly; no transform is guessed. The remaining retail
TalkAnim interpolation calls are documented but not fabricated until the real
scene TalkAnim controller exists.

## Wipe behavior installed

First-step Wipe rows preserve their arbitrary `WipeName` and raw `Frame` value:

- type 0: open;
- type 1: close;
- type 2: force open;
- type 3: force close;
- unknown: no operation, matching the retail switch.

The `-1` frame sentinel passes through unchanged. The dispatcher uses the
general scene `WipeService`, not named demo special cases.

## Honest remaining keeper gaps

Player, Camera, Sound, and TalkAnim keep their retail positions but are not
claimed as implemented by this slice. Those gaps are explicit follow-up
compatibility work; this change does not use fallback motions, cameras,
positions, or callbacks.

See `verification.log` for the focused regression result.
