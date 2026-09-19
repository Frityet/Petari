# Authored rabbit route and tower appearance audit

Read-only inspection on 2026-09-19. This establishes authored rules and available source, **not an observed rabbit catch, tower activation, Rosalina appearance, or visual parity**. No production source, actor state, story switch, camera, or controller input was changed by this audit.

## Evidence

- `zone-metadata.json`: full real `HeavensDoorMysteriousZone.arc` tables; scenario1 uses Common and LayerA. Other layers remain in the raw file and are not evidence of active placements. Archive SHA256 `6004cbf1a9669b500ae45a2c54dabd64b019fa0699f50a1cf17e211b7544be97`.
- `tico-guide-sheet.json`: all seven real `TicoGuideDemo` sheet tables extracted from `ObjectData/DemoSheet.arc` on the user's Korean disc. Paths and archive SHA256 are recorded in the JSON.
- `selected-stage-chain.json`: relevant placement subset. General-position rows remain in the full metadata file.
- `source-drift-audit.json`: ten native/canonical source comparisons after removing the CP932 include and decoding CP932 literal wrappers. Nine match exactly. The only additional difference is two `INIT_NERVE` declarations in RunawayRabbit, corresponding to its native nerve-ownership macro boundary; no changed chase/catch behavior was identified.
- `controller-waypoints.json` and `compute_waypoints.py`: raw rows, actual zone placement, matrices, reproducible navigation candidates, and numerical/reachability limits. The script only reads metadata and writes this note JSON.

## Controller-only route

Finish the guide's ordinary dialogue and follow its existing demos until `RunawayTico::startRunaway` turns on SW1111. The collector becomes Active, activates its four hidden rabbits, and broadcasts the original runaway-start message. One A press need not complete a multi-page conversation. Use ordinary A presses with releases as text/pages finish; native A is Return/Space. Do not infer chase readiness from a closed bubble alone: observe the original start part or SW1111/collector state.

There are **three unique capture groups**, with two alternative actors for the pipe group. Select by original zone/type/l_id or collector group, not a runtime actor ID that changes between launches.

| Group | Original child l_id | Reveal gate | Action |
| --- | --- | --- | --- |
| 2, bush | 7 | SW1113 | Enter the Common SwitchCube l_id8 while SW1111 is on. |
| 0, hole/baby | 8 | SW1114 | Enter Common SwitchCube l_id11 while SW1111 is on. |
| 1, pipe alternative A | 6 | SW1112 | Complete transit beginning at EarthenPipe l_id56 and exiting l_id57. |
| 1, pipe alternative B | 10 | SW1118 | Complete the reverse transit, beginning at l_id57. |

Both SwitchCubes have every Obj_arg=-1 and SW_B1111. Original SwitchArea therefore latches its A switch once Mario's position enters the volume; these rows require **no spin, crouch, ground check, or plant destruction**. PlantGroup supplies scenery, not this reveal gate. Hidden rabbits can emit nearby sounds without being revealed.

Cube2 bounds are X/Z centered and Y from zero up to `1000*scale_y`, with the upper bound excluded. The hole's raw origin is at the interior end of this volume. Candidate points at 75% of its height are provided below; they are geometrically inside the trigger, but a collision-free walking path to them has not been observed.

| Target | World position after restored zone transform |
| --- | --- |
| Bush Cube2, Y75% | `(13788.189, -10760.262, 4724.100)` |
| Hole Cube2, Y75% | `(13656.107, -10153.884, 7867.562)` |
| Hole Cube2, Y90%, nearer its opening | `(13565.785, -10103.431, 7954.287)` |
| Bush rabbit l_id7 initial spawn | `(13569.846, -10948.403, 5068.829)` |
| Hole rabbit l_id8 initial spawn | `(13515.648, -10082.710, 7993.759)` |
| Pipe rabbit l_id6 initial spawn | `(16820.500, -10519.086, 6419.470)` |
| Pipe rabbit l_id10 initial spawn | `(12760.199, -11437.315, 5866.059)` |

These use the actual zone5 stageobj translation `(14760,-10676.225586,6770)` and rotation `(65.579147,70.138962,56.559284)`. Rotation is column-vector `Rz*Ry*Rx`; the script models the existing original JMap rotation composition and short-angle table reconstruction used by AreaFormCube. Floating-point results are approximate, not bit-exact captured form matrices. Do not use these world points with the earlier binary whose JMap owner transform was missing. Follow the rabbit's **current** position once it moves.

The EarthenPipe pair has original pair ID1050 (Obj_arg3), mode0, and valid binder sensors. Approach the real pipe opening/top matrix and overlap its binder. The inspected standard rush path uses automatic binder contact; EarthenPipe accepts `ACTMES_AUTORUSH_BEGIN` without an extra Z/down/button check for this normal, non-swimming case. Stop steering once Ready/In starts and let the original animation sequence complete. The source pipe turns on its own SW_B **after PlayerOut's animation ends**, and only when the collector's `_194` waiting suppression has been cleared by the chase-start broadcast. Transit before chase does not reveal this rabbit. Entering both alternatives does not add a fourth capture: revealing either disables the other member of group1.

WarpPod l_id52/53 is a separate invisible eye-sensor warp pair, not these EarthenPipes. Its actual arg1=0, scale2.25 gives radius270. Original eye contact sends ACTMES_WARP automatically; no switch/button is required. Its world sensor centers are in the JSON if this passage is useful for navigation. It does not write the rabbit pipe switches.

Once a rabbit appears, pursue ordinary contact. Its Spine Catch sensor has radius30; its Body sensor has radius70. `isCaughtable` requires a runnable nerve and zero not-catchable timer. Appearance, pending/caught demos, caught talk/end and Stop exclude catching. A successful Catch-sensor/player overlap requests the original `捕まり` demo; spin or stomp is not required. Temporarily neutral input during this sequence avoids steering against the demo. Finish the rabbit's ordinary forced conversation and animation; only its Stop nerve is reported as fully caught to the collector. Continue through the following Tico comment before navigating to the next group.

## Exact last-catch to Rosalina chain

1. The collector receives a Catch-sensor event, increments `_A4`, raises remaining rabbit speed up to original level2, and selects the last-message branch when `_A4==mCompleteRabbitCount` (three unique groups here).
2. Rabbit Caught waits at least seven steps and for ground binding, then CaughtTalk. Talk completion leads through Toss/CaughtEnd; BCK completion ends `捕まり` and enters Stop. The collector processes Stop once, marks the matching `_B0[castID]`, and appears the corresponding Tico. Earlier comments select the first uncaught group; all three bits select `appearMamaComment`.
3. That Tico sets `mIsAllCaught`, runs original appearance/talk, then whiteout. Whiteout lasts90 frames; after swipe completion, WhiteIn at step75 opens whitefade90, ends `ぼやき`, and starts `チコガイドデモ` at `高楼出現[デモ]`. The last comment Tico dies normally.
4. Real time-sheet rows11–14 are `高楼出現[デモ]`300, `[デモ後]`120, `[フェードアウト]`60, `[フェードイン]`1 with Suspend=1 at the last row.
5. In the first part, Rosetta ActionType2 runs preDemo/hidePlayer; ActionType4 writes its SW_A1015 and ActionType5 writes SW_B90. **This Rosetta sheet action is the actual tower switch writer.** The collector's final `MR::isValidSwitchA(this)` call is also present in canonical source; it must not be replaced with a guessed switch write.
6. At the next part boundary (300 part frames), `[デモ後]` ActionType0 appears Rosetta. Additional callbacks handle her light/halo. The later fade-in appears `HeavensDoorAppearStepAAfter`, kills the during-demo step, restores player visibility, and places surviving Tico casts1/2 at the authored general positions.

SW1015 is global bit15 under original IDs>=1000 semantics; SW90 is local to zone5. The actual Rosetta placement is LayerA l_id28, DemoGroupId0, SW_A1015, SW_B90, SW_DEAD1016. Its DomeLight/DomeHalo are owned PartsModels, not missing independent factory names. SW1015 also gates two CollisionAreas, seven CollisionBlockers, three CrystalCageM and camera areas. CrystalCageM was under a peer's bounded import/validation at audit time; do not infer it passed from this note.

No additional missing direct factory or utility was proved in the inspected catch-to-appearance chain. Existing exact actors cover Rosetta, RunawayTico/Rabbit/collector and the two step models. The collector's separate DemoGroupId1 refers to dormant SpinGetDemo metadata; its children participate in real TicoGuideDemo group0. This does not justify a demo-name alias.

The live acceptance gate remains: ordinary controller input catches all three groups; forced talks/demos end naturally; the Tico whiteout starts the named tower part; actual Rosetta sheet actions write SW1015; the during/after step, CollisionAreas and Rosetta appear under their original gates. None of those late events was established by this read-only audit.
