# Original Gateway: Middle → Inside → Grand Star

2026-09-25. Read-only source/resource planning after `c3f55804a`. No production edits, builds, tests, controller writes, or gameplay completion claim. Coordinates are authored placement anchors, not observed live actor positions or guaranteed navigable waypoints. Moving enemies, dropped keys, pipe animation and gravity change their live targets.

## Positions for the controller operator

Scenario1 uses Common+LayerA. The Small-to-Middle root SpinDriver `l_id2` is at `(6704.4, 2943.0, -5296.4)`, appears through switch1127, and uses CommonPath_ID1 (`commonpathpointinfo.0`, not `.1`). Its authored path goes from `(6692.2, 2962.3, -5310.3)` to `(3786.5, 4753.2, -6863.3)`. The latter is a useful Middle arrival anchor, not a measured landing point.

Middle center is `(2856.9, 6490.8, -7048.8)`; Inside center is `(42840.0, -14537.6, -2530.0)`. Both zones have X rotation `-12.30056381225586°`. Below, child positions are transformed approximately using the current JMath degree-table index convention (`floor(abs(degrees)*45.511112)`) and rounded to0.1. Root positions are directly authored. These numbers are for navigation planning, not a bit-exact SDK transform oracle.

| Target | Placement | Approximate world XYZ | Meaning |
| --- | --- | --- | --- |
| Chief Goomba | Middle Common `KuriboChief`31 | `(3586.9, 8010.1, -6847.4)` | Spawn anchor; use live enemy position after movement. |
| Cage | Middle Common `CapsuleCage`24 | `(2856.9, 4906.9, -6704.0)` | Opens when switch1100 is written by collected key. |
| Freed Luma | Middle Common `Tico`26 | `(2856.9, 4744.6, -6668.7)` | MessageId0, SW_A1100, SW_DEAD1008. This is the progression Luma. |
| Other Luma | Middle Common `Tico`43 | `(3197.8, 5491.9, -5667.5)` | MessageId3; no progression switches. |
| Entrance pipe base | Root LayerA `EarthenPipe`6 | `(2857.6, 4809.8, -6675.5)` | Pair70, SW_A1008; normal mode0. |
| Entrance blocker | Root LayerA `CollisionBlocker`12 | `(2859.0, 4830.5, -6677.6)` | SW_B1008 kills its push sensor. |
| Inside exit pipe base | Root LayerA `EarthenPipe`7 | `(42836.1, -13079.2, -2848.1)` | Pair70, mode1; initially hidden, appears for transit. |
| Inside entry Luma | Inside Common `Tico`25 | `(42816.2, -13376.4, -3314.2)` | MessageId1, behavior8, SW_A0. |
| Inside lower Luma | Inside Common `Tico`24 | `(43090.0, -15758.2, -2653.1)` | MessageId0, SW_A0. |
| Grand Star | Inside LayerA `GrandStar`26 | `(42840.0, -15595.7, -2299.6)` | Local `(0,-1082.8,0)`, star ID1, DemoGroup0. |

Root RestartCube2 is centered at `(3566.9,4469.0,-6168.8)` and selects restart ID2; RestartCube3 is `(42350.0,-14195.1,-2610.0)` and selects ID3. These are volume centers, not the restart spawn coordinates. Original `RestartCube::updatePlayerRestartIdInfo` supplies the checkpoint when the player is in its volume.

## Middle gate: fight, key, conversation, pipe

1. **Hit the actual chief with a spin/punch.** `KuriboChief::receiveMsgPlayerAttack` (`src/Game/Enemy/KuriboChief.cpp:252`) accepts PLAYER_PUNCH/UPPER_PUNCH on its `Punch` sensor and requests Stagger. An ordinary stomp is not in `isMsgPlayerHitAll` (`ActorSensorUtil.cpp:781`), so do not assume one stomp kills this enemy. Current keyboard mapping is WASD stick, X swing, Space/Return A (`src/render/RendererService.cpp:346`). The existing controller script can supply the same ordinary inputs.
2. **Finish while it is vulnerable.** Chief parameters set stagger duration240 and kick enable step70 (`KuriboChief.cpp:93`; `WalkerStateStagger.cpp:116`). During the early Stagger nerve, finishing contact before70 is rejected as a kick; a later PLAYER_KICK contact or second accepted punch requests BlowDown. Follow the live Stagger/Swoon state rather than counting global frames through pauses. BlowDown waits for ground contact after its minimum5 steps, then BlowDownLand waits for the actual BCK to stop before `kill` (`KuriboChief.cpp:444`).
3. **Collect its real dropped key.** `kill` appears the owned KeySwitch at the chief's *death position*, not the original spawn (`KuriboChief.cpp:221`). Key launches upward against gravity, bounces and may move. `KeySwitch::receiveOtherMsg` rejects item-get for Appear steps≤60; afterward normal ITEM_GET kills it and writes switch1100 (`KeySwitch.cpp:148,180`). Its body sensor radius105 is offset105 above its actor origin. The cage's B1100 listener starts its original camera/Move animation; with a camera it waits50 steps before opening and60 after animation completion (`CapsuleCage.cpp:40–90`).
4. **Talk to progression Tico26 and finish the conversation.** It is near the cage/entrance, not Tico43. Tico sets talk distance350 and registers the message kill callback (`Tico.cpp:123`). That callback enters Metamorphosis; only when its BCK stops does Tico kill, and original NPCActor kill writes SW_DEAD1008 (`Tico.cpp:283,365`; `NPCActor.cpp:513`). Use normal A press/release dialogue input and wait for the actual transformation/kill. Opening the cage alone does not enable the pipe. This source inspection does not decode the BMG dialogue branch; the authored SW_A1100 and real message flow remain authoritative.
5. **Center on the enabled pipe opening.** `EarthenPipe::exeWait` validates the binder only after A1008; CollisionBlocker also dies on1008. The pipe handles AUTORUSH_BEGIN, so its contact path does not require a special scripted teleport or direct switch write. Binder radius70 is offset`(0,-50,0)` from its actual pipe transform (`EarthenPipe.cpp:126`). It rejects non-swimming entry when lateral distance from its Top joint exceeds50 **and** the player is below its outward axis by more than5 (`:409`). Approach the mouth center, using ordinary movement/jump if needed. Do not target only the authored base: init extends this entrance by100 along gravity-up; the exit's scale gives120 and it begins hidden. Ready→PlayerIn→target ShowUp→PlayerOut uses original Mario BCK completion and paired cameras (`:253–327`).

## Inside: all sixteen original reverse panels

Every row below is Inside Common `FlipPanelReverse`, GroupId0, scale0.5. The first four are isolated around the equator; the twelve61–72 form a lower perimeter in local X/Z. Numeric order61→…→72 follows that perimeter and avoids revisiting a successful panel, but connecting terrain/camera motion still requires live steering. The table is not a prevalidated collision-safe route.

| l_id | Local XYZ | Approximate world XYZ |
| --- | --- | --- |
|49|`(12.1,58.0,-1492.9)`|`(42852.1,-14798.5,-4001.1)`|
|58|`(-10.8,58.6,1484.6)`|`(42829.2,-14164.6,-1091.9)`|
|59|`(-1492.7,55.2,-10.4)`|`(41347.3,-14485.9,-2552.0)`|
|60|`(1488.5,55.1,11.4)`|`(44328.5,-14481.4,-2530.6)`|
|61|`(-607.7,-1247.9,-607.3)`|`(42232.3,-15886.1,-2858.0)`|
|62|`(-610.3,-1368.7,-205.9)`|`(42229.7,-15918.8,-2440.1)`|
|63|`(-609.4,-1372.9,214.4)`|`(42230.6,-15833.5,-2028.4)`|
|64|`(-607.3,-1247.7,608.1)`|`(42232.7,-15627.3,-1670.4)`|
|65|`(-206.4,-1356.9,632.1)`|`(42633.6,-15728.9,-1623.8)`|
|66|`(202.9,-1357.1,632.7)`|`(43042.9,-15729.0,-1623.1)`|
|67|`(606.8,-1263.8,607.9)`|`(43446.8,-15643.1,-1667.1)`|
|68|`(620.9,-1379.4,207.2)`|`(43460.9,-15841.3,-2034.1)`|
|69|`(621.7,-1381.4,-209.8)`|`(43461.7,-15932.0,-2441.1)`|
|70|`(607.8,-1266.5,-608.5)`|`(43447.8,-15904.6,-2855.1)`|
|71|`(202.8,-1359.9,-628.3)`|`(43042.8,-16000.0,-2854.6)`|
|72|`(-206.1,-1359.7,-627.7)`|`(42633.9,-15999.8,-2854.1)`|

`FlipPanel::checkPlayerOnTop` (`src/Game/MapObj/FlipPanel.cpp:227`) requires actual player-on-actor contact, a grounding polygon, and panel-up matching ground normal. Its counter confirms contact across evaluations; a rising jump after prior contact can also flip it. Merely being near a placement or airborne above it is insufficient. Each starts Front with bloom visible because this is the Reverse variant. Front→BackLand runs PanelA, removes bloom and sends SUCCESS once; revisiting turns FrontLand, restores bloom and sends FAILURE. Leave onto other ground to finish the landing nerve; avoid repeated jumps/re-entry over completed panels. Use Back/BackLand plus observer count as semantic evidence, not color alone.

Observer2 counts group members minus itself:16 successes are required (`FlipPanel.cpp:307`). It starts `FlipPanelComplete`, stops group movement, waits its default40-step delay, ends that demo and only writes local switchA0 once no demo is active. This triggers DemoGroup0/MiniSunDisappear, not direct star creation.

## MiniSun disappearance, pickup and normal handoff

Freshly decoded `DemoSheet.arc` has four unsuspended main parts: ミニ太陽消失80, ミニ太陽演技120, ケージオープン50, ケージ演技210 (460 total steps). Its one-step グランドスター復活 subpart occurs at ケージ演技 step60, about main step310. That invokes PowerStar WeakToWait; it reaches Wait after30 nerve steps, and Wait validates its sensors (`PowerStar.cpp:132,741,779`). Wait for demo/camera control to return before attempting the pickup, even if the star becomes visually bright earlier.

Move into the actual GrandStar binding sensor near the anchor above. Only Wait accepts AUTORUSH_BEGIN; weak appearance alone is not collectible (`PowerStar.cpp:332`). StageClearDemo requests GameSceneGrandStarGet, sends star ID1, starts original player/star animation and camera. Do not stop proof at the animation: `GameStageClearSequence.cpp:101` shows the result layout at510, starts the non-final-boss circle wipe at670, then waits for it to complete before requesting GalaxyMoveArgument type4. `StorySequenceExecutor::decideNextEventForClearGalaxy` routes first HeavensDoor1 clear to AstroGalaxy and queues the original Grand Star return/Rosetta explanation. Evidence milestones are:16 counted panels→MiniSunDisappear ends→star Wait→StageClearDemo/GameSceneGrandStarGet→normal scene-change request→AstroGalaxy scene initialization. The previous audit documents ownership; this note does not establish those runtime outcomes.

## Resource provenance

Freshly re-read existing extracted RARC bytes with the read-only parser from `notes/gateway-content-inventory-20260919/read_metadata.py`, omitting only its now-deleted compat source from field-name discovery in memory. No archive writes or new extraction. Placement tables: root `/jmp/placement/layera/{objinfo,stageobjinfo}`, root Common areas, and child Common/LayerA objinfo. MiniSun tables include time, action and subpart.

- `build/gateway-content-inventory-20260919/HeavensDoorGalaxy.arc`: SHA256 `9c80464cbdc9e73f3d440476eb7a6755ad19dc87c3a9d764501a7dda9b164f56`.
- `…/HeavensDoorMiddleZone.arc`: `79d2d631996a73f40c63751ec7b25e73a81a28e9b0b542a6e8d7e78811062a0d`.
- `…/HeavensDoorInsideZone.arc`: `ecb3b7f72e22187ad2b4ca7c7f4f34e90cbc63f55304ad2695ee2f804a2a0fa0`.
- `build/original-warp-pod-audit-20260919/DemoSheet.arc`: `975b6c99ec882c5e56f24e4684859692fc98613dc0e021834e9895b46eae8b6d`.

These hashes match the existing extraction provenance; current controller-run actor traces remain authoritative for live positions/state. Older missing-source/factory claims are not reused.
