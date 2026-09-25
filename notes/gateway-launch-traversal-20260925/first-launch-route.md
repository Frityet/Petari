# First tower launch: ordinary controller route

2026-09-25. Read-only current-source/log/resource-metadata review. No production code, operator code, UI/input, builds or tests changed. Coordinates below are navigation targets, not game-state writes. Runtime IDs refer to the completed36000-frame spin-ascent run and must be rebound if a later run differs.

## Driver approach and firing

1. Break the actual gate crystal(s); do not approach a dead driver. Last run crystals1016/1020 entered Break at35950, driver864 appeared35960 and reached Wait35980. Its original `isNerveEnableBind` requires **Wait step >90** (`SuperSpinDriver.cpp:1022`), so that run ended before it could accept capture. A visible yellow star is not yet bind-ready.
2. Walk toward the **tower floor under the star**, approximately **[15045.333,-8300.073,7495.381]**, with a small horizontal/reach margin (about60–80). Driver actor centre is **[15080,-8042.270,7580]**; treating that floating centre as an ordinary foot-position goal gives an irreducible vertical distance.
3. Once Capture starts, release movement and press ordinary **X** promptly, preferably during the first30–40 Capture steps. The original `tryShootStart` reads actual pad swing (`:445`). Alternatively, X during valid rush overlap in Wait starts ShootStart directly. A jump is only needed if normal walking cannot bring the body sensor near enough; the geometry does not inherently require jumping when directly below this star.
4. ShootStart advances after45 steps; Shoot lasts the authored **300 frames**. Leave stick neutral to follow the authored centre route. EndBind hands Mario back to original jump/landing behavior and ends the event camera (`:466/:816`). Confirm driver Capture/ShootStart/Shoot and Mario `bound_actor_934`, then the actual far-end landing; appearance alone is not traversal.

**Why the grounded/body distinction matters:** driver bind sensor radius300 is centred on its actor (`SuperSpinDriver.cpp:141`), but automatic pull separately requires strict sensor-centre distance **<240** (`:368/384`, `ActorMovementUtil.cpp:179`), not radius overlap or foot distance. The ordinary Mario body sensor is centred at `_2A0` with radius100 (`MarioActorSensor.cpp:23/52`); `calcCenterPos` ordinarily places that near feet+60×up (`MarioActor.cpp:2507`). Special states use different centres, so60 is an ordinary-standing estimate, not a replacement for Game logic.

Using the last frame's actual tower basis/up, the star is273.5 units above its floor. Standing directly underneath gives estimated body-centre distance213.5, inside240; allowable horizontal offset is only about110. At frame35990 Mario was at **[15147.149,-8370.063,7666.977]**,211.4 horizontally from the star,345.7 foot-distance and approximately300.5 body-distance: outside automatic pull. Thus another ~130–200 units of ordinary lateral approach is useful; more X at the same distant point is not the first thing to try.

Authored Obj_arg2=-1 leaves `mIsPullPlayer` at its original true default (the NoInit reader rejects-1). Capture can cancel after step60 when centred within15 (`:425`). If it cancels, a fresh X in a valid Wait overlap can still start shooting; passive pull is disabled until the player moves farther than350 actor-position units (`:283/:1030`). Do not wait indefinitely for an automatic launch: Capture expects a swing.

## Authored destination

Root LayerA SuperSpinDriver l_id5 has SW_APPEAR1017, CommonPath_ID3, flight300, camera count3 and landing rotation-180. Path3 is **three-point Bezier**, so SpinDriverShootPath selects the real RailRider, not its <=2-point parabolic branch:

- pnt0/start: `[15080,-8007.236,7490]`
- middle: `[14358.454,-5255.388,613.768]`
- pnt0/end: **`[13436.447,-2651.243,-3706.727]`**

The original start-position correction brings the first half onto the actor centre, then fades out; the final target remains the authored rail endpoint (`SpinDriverShootPath.cpp:32/46`). This is the BlackHole planet leg, near HeavensBlackHoleZone's root placement `[13917.063,-1896.362,-4606.810]`, not the Small planet. The subsequent five YellowChip gate and driver at `[12958.847,-830.847,-4648.078]` are later milestones.

These rows come from `notes/gateway-content-inventory-20260919/root-zone.json`; its source archive `build/gateway-content-inventory-20260919/HeavensDoorGalaxy.arc` still matches recorded SHA256 `9c80464cbdc9e73f3d440476eb7a6755ad19dc87c3a9d764501a7dda9b164f56`. `CommonPath_ID3` resolves metadata row1/no1 and `commonpathpointinfo.1`, not the filename suffix3. Current factory/driver source is present; no missing-driver workaround is needed.

## Stair-operator handoff

The **previous run's** `notes/gateway-spin-ascent-20260925/climb_tower.py` enters tutorial by proximity/end-of-points and then pulses A every100 frames **without a completion transition**. That version keeps the input lock and continues publishing neutral stick/A until35000 or an external stop. The completed spin-ascent run needed explicit stop/transfer; its logged tutorial phase continued through22190 even though tutorial actors retired18710.

The **current run's copied operator**, `notes/gateway-launch-traversal-20260925/climb_tower.py`, already adds a completion transition: while in tutorial phase, `rosetta.dead && frame >16000` emits `tutorial-finished-handoff` and exits through `finally` cleanup. The missing-transition finding above applies only to the previous script. The stronger history/state checks below are a possible later tightening of the current route, not a claim that its present operator lacks a handoff. `follow_actor.py` does not take the same operator lock, so the next operator should still start only after the climber has exited.

A more conservative notes-operator completion predicate can use existing trace observations:

1. During this run, first observe Rosetta1060 alive/unhidden and then the spin-tutorial entry evidence: her repositioned demo pose and Tico903 alive (last run15700). Do not use `rosetta.dead` without this history: she starts dead before the rabbits.
2. After that observed tutorial entry, require **both Rosetta and the same Tico to be dead**, Mario status0 (not MarioTalk34), `bound_actor_934` false, and the existing fixed-matrix flags false in at least two consecutive fresh samples. The first matching actor transition was18710. This is a conservative observed handoff boundary, not direct proof of the untraced spin-permission bit or a universal demo-completion API.
3. Clear current buttons/stick, emit a completion record and exit the climb operator so its `finally` cleanup and lock release finish. Only then start the crystal operator, ideally taking the same exclusive lock. This avoids two writers or the old operator clearing a newer command during teardown.
4. A later ordinary Luma conversation is a separate gate. In the prior run MarioTalk persisted until scripted A triggers29460/29590, then status0 at29600. Pause approach and advance actual talk rather than diagnosing blocked movement as collision. Resume only after status leaves34. Physical X at33190 then produced MarioMagic and cage1013 Break33200, demonstrating the input path and acquired ability.

The trace does not directly expose time-demo activity or swing permission. Keep the historical actor/state predicate limited to this notes-only route, and retain actual spin/cage-break evidence as the permission-behavior check. No such predicate belongs in production Game behavior.
