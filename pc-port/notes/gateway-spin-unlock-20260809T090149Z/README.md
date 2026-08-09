# Gateway spin-unlock checkpoint

Date: 2026-08-09 UTC

This milestone extends the real Gateway walking slice with a bounded,
playable post-high-tower checkpoint that reaches the original spin-unlock
prompt and grants the real player entitlement. It deliberately does not claim
the rabbit chase or the complete continuous prologue.

## Demo

From `pc-port`:

```sh
xmake run smg-pc-showcase gateway-spin --disc "$PWD/../RMGK01.iso"
```

The route loads the real RMGK01 Gateway planet BDL, KCL, PA, point gravity,
Mario model, and Mario animations. It seeds the authored progress-10
post-high-tower checkpoint, starts Mario inside the exact 500-unit Rosetta
trigger, performs the exact 90-frame handoff, and executes the real
`TicoGuideDemo` Player and Wipe rows. After the authored 1,670-frame prelude,
the exact `InformationObserver` pauses the timekeeper and displays the retail
Korean `InformationObserverSpin` message. Press A, Enter, Space, or left-click
after its 30-frame guard. The fresh edge persists story progress 15 and grants
spin permission to the attached Mario actor. X then proves the retained
unlocked swing request. The visible spin-action state machine remains outside
this checkpoint.

The ordinary real stand/walk/release route remains:

```sh
xmake run smg-pc-showcase gateway --disc "$PWD/../RMGK01.iso"
```

## Original-source boundary

- `InformationObserver.cpp` and `.hpp` are byte-identical PC mirrors of the
  reconstructed Game source.
- The production prompt path is authored demo part 22 ->
  `MR::explainEnableToSpin` -> exact `InformationObserver` -> exact event flag
  provider -> the general PlayerSystem entitlement bridge. There is no fake
  prompt, prompt callback, or direct grant in Showcase.
- Modern-host branches remain marked `TARGET_PC`; retail branches reconstruct
  exactly under the mirror tests.
- The only root reconstruction correction in this tranche is the retail-proven
  `atan2_(stick.y, stick.x)` operand order in `Mario.cpp`.
- A host `wchar_t` truncation bug was fixed generally by retaining layout
  message-ID provenance. A raw BMG control tag contains an embedded zero;
  recognized layout-message pointers now use the existing tagged-ID renderer
  path, while literal wide strings retain their old behavior.

## Exact checkpoint evidence

- Scenario: `HeavensDoorGalaxy`, scenario 1, zone 5, DemoGroup link 0.
- Rosetta/TicoBaby source rows: 12/13.
- Player source positions: `MarioDemoPos2` and `MarioDemoPos4`.
- Rosetta trigger radius: 500 units.
- Fade/handoff: 90 frames.
- Demo start: part 15 (`スピンゲット[デモ1]`).
- Part 15 through part 21 duration: 1,670 frames.
- Prompt boundary: part 22 (`スピンゲット[デモ5]`).
- Information guard: 30 display executions; only a fresh later A edge grants
  entitlement.
- Result: story progress 15, PlayerSystem swing permission true, Mario `_EEB`
  true.
- Rabbit route: explicitly absent from this checkpoint.

## Verification

See `verification.log`. Highlights:

- Exact Gateway spin checkpoint: PASS against real RMGK01.
- Exact InformationObserver prompt/guard/lifecycle plus nonempty real text
  raster: PASS.
- Real Mario stand/walk/release/60-frame idle/recreate: PASS.
- Showcase Gateway Vulkan smoke: PASS.
- Interactive spin route through unlock: PASS; final capture shows upright
  Mario, the retail Korean message, and the A icon.
- Player source mirror: 96/96 retail source branches and 63/63 headers exact.
- Root RMGK02 DOL SHA-1: `54b71431af0d509097bfdef4ec28617afc487e89`,
  exact manifest match.

Local visual evidence (generated, intentionally not committed):
`pc-port/demo-report/evidence/gateway-spin-final.png`.

## Decompilation report

Fresh RMGK02 report after the Mario direction correction:

- Overall code: 65.70% matched (3,556,308 / 5,413,352 bytes), 14.25%
  linked; 34,021 / 42,036 functions and 734 / 2,219 files represented.
- Game code: 63.09% matched (2,616,792 / 4,147,644 bytes), 13.30%
  linked; 28,627 / 35,720 functions and 532 / 1,605 files represented.
- Overall data: 30.83% matched (511,184 / 1,657,852 bytes).
- Game data: 44.10% matched (294,928 / 668,696 bytes).

These percentages are reconstruction metrics, not the PC demo completion
percentage. The demo intentionally uses the smallest honest runtime slice
rather than linking every reconstructed Player state.

## Deferred

- The rabbit chase and full continuous beginning before this authored
  checkpoint.
- Visible spin attack animation/action-state closure after entitlement.
- Full NPC/talk choreography outside parts 15-22.
- Audio playback; the exact sound request is retained on the logical event
  path, but audio remains outside the requested demo scope.
