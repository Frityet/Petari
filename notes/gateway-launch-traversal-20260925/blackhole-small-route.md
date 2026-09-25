# BlackHole and Small planet route references

Read-only planning against the current source after `c3f55804a`; no game/controller writes, source edits, builds or tests. Coordinates below are **authored world positions**, computed as zone translation + `Rz * Ry * Rx * local position` (degrees), rounded to 0.001. They are navigation references, not a verified collision-free route or exact live actor positions. Native float/trig evaluation can differ slightly; moving actors and the spawned key must be tracked live.

## BlackHole: five yellow chips, then the second launch

The first launch's root path 3 ends at `(13436.447, -2651.243, -3706.727)`. This is the authored rail endpoint, not a proven landing position.

| Authored object | World position | Required interaction / condition |
| --- | --- | --- |
| YellowChip 16 | `(13587.540, -1989.895, -5968.153)` | Touch/get item after its initial 40-step immunity; group 0 |
| YellowChip 17 | `(13918.280, -3029.414, -4070.882)` | Appears after local switch 80; group 0 |
| YellowChip 18 | `(13044.146, -877.746, -4618.176)` | Touch/get item; group 0 |
| YellowChip 19 | `(13085.628, -1389.303, -3783.888)` | Touch/get item; group 0 |
| YellowChip 20 | `(14871.662, -1200.316, -4757.168)` | Touch/get item; group 0 |
| CrystalCageS 30 | `(13915.536, -2896.946, -4075.218)` | A valid player hit breaks the small cage; DEAD local 80 releases chip 17 |
| CrystalCageS 29 | `(12908.845, -2329.347, -4305.629)` | DEAD local 81; not part of the inspected five-chip switch dependency |
| CrystalCageS 32 | `(13753.802, -2119.573, -3469.190)` | DEAD local 82; not part of the inspected five-chip switch dependency |
| Tico 25 | `(13587.057, -2321.234, -3515.172)` | Reads global A 1009; nearby route guidance |
| SuperSpinDriver, root 8 | `(12958.847, -830.847, -4648.078)` | Appears on global 1009; spin in binding range to launch |
| BlackHole 14 | `(13955.342, -1910.785, -4524.826)` | Hazard: eye sensor radius 500 at authored scale 1; APPEAR 1016 |

ChipGroup 15 has group ID 0 and A 1009. `ChipBase::requestGet` informs the actual holder/group, then enters Got. All five registered chips must be marked gotten. Completion requests the original ChipCounter demo; **1009 is written only after the completion/outro animation ends** through `noticeEndCompleteDemo`. Wait for the HUD/demo to finish and the launch actor to appear; five hidden chip models alone do not establish that the gate completed.

Small cages initialize with one hit point, and accept `isMsgPlayerHitAll` while waiting; the large-cage two-hit branch does not apply to these `CrystalCageS` placements. Cage 30 is the explicit required chip-release gate. The other two cages' switches do not directly gate any of the five listed chips.

Use surface/tangent movement around the planet and the current gravity direction. A straight line between opposite-side chip coordinates can cross the planet or the black-hole region. MeteorCannons 21/22 at `(14263.455, 618.587, -8129.407)` and `(12610.948, -2607.485, -8247.175)` are hazards gated by local 21, not collection targets.

The next launch is root path 4: first control point `(12815.184, -679.738, -4658.647)`, last `(7380.000, 1658.289, -5250.000)`. The launch actor and the first rail point are different coordinates. SuperSpinDriver requires the ordinary swing trigger (or original second-player trigger); being pulled near its center without swinging can time out/cancel after 60 frames. Let the original launch/camera/rail run rather than steering toward the endpoint during flight.

## Small: caged Tico, Goomba, key, then transformation

The Small zone origin/planet reference is `(6680.000, 2078.292, -5310.000)`. The cage/Tico are near its positive-Y pole; the Goomba starts on another side.

| Authored object | World position | Required interaction / condition |
| --- | --- | --- |
| Tico 9 | `(6680.000, 2925.405, -5310.000)` | A 1125, B 1124, DEAD 1127; MessageId 0 |
| CapsuleCage 16 | `(6680.000, 2643.584, -5310.000)` | Opens on B 1125 |
| ChildKuribo 3 | `(7112.569, 1852.793, -6206.094)` | Parent 21; SW_SLEEP 1124; position changes when active |
| ExterminationKuriboKeySwitch 21 | `(6671.467, 3643.575, -8239.595)` | APPEAR 1124, A 1125; **logic-owner position, not the key spawn** |
| SpinDriver, root 2 | `(6704.375, 2943.039, -5296.367)` | Appears on 1127; ordinary spin/bind launch |

1. Approach the caged Tico and advance its ordinary dialogue with discrete button presses/releases. Its authored B 1124 is the encounter-start switch; `TalkMessageCtrl` event group 6 writes B, and its branching can read A/B. The exact MessageId 0 BMG flow was not decoded in this planning pass, so confirm 1124/awake Goomba live rather than assuming a single press completes this step.
2. Defeat the owned ChildKuribo. Original Kuribo accepts a stomp/hip drop; a general player hit can only stagger it until kicking becomes enabled. One spin is not proof of defeat.
3. The actual ExterminationChecker watches its owned children and retains the last living child's position. After all are dead, its key variant waits until step 30 and calls `appearKeySwitch` there. Follow the actual key: it launches away from gravity and can move/bounce. Item-get is rejected during the first 60 steps of the key's Appear state. The checker coordinate above should never be used as the pickup target.
4. Collecting the key kills it and writes A 1125. CapsuleCage listens on B 1125, runs its original opening/camera animation, then disappears. Wait for the actual cage state to open.
5. Talk to the freed Tico and let the original Metamorphosis animation finish. Its registered kill callback starts Meta; when the animation stops, `NPCActor::kill` writes DEAD 1127. This releases the root SpinDriver. Use the actual actor position after its rail movement, rather than its initial coordinate.
6. Spin near the visible SpinDriver. Root path 1 starts at `(6692.188, 2962.334, -5310.306)` and ends at `(3786.453, 4753.193, -6863.286)`, toward Middle. Again these are rail controls, not proof of a landing state.

Four Small CrystalCageS placements have no story switch dependency in these rows: 17 `(6427.686, 2054.943, -4488.936)`, 18 `(7371.104, 1756.427, -5682.817)`, 19 `(6397.868, 2188.912, -6109.532)`, 20 `(5900.266, 2462.214, -5359.120)`. They are not required by the inspected Kuribo/key/capsule/Tico chain.

## Evidence and limits

Freshly decoded the actual local archive bytes with the existing notes-only metadata parser (skipping its removed compat file as a field-name discovery source, without editing it). Current hashes match the earlier inventory provenance:

- `build/gateway-content-inventory-20260919/HeavensDoorGalaxy.arc`: `9c80464cbdc9e73f3d440476eb7a6755ad19dc87c3a9d764501a7dda9b164f56`
- `build/gateway-content-inventory-20260919/HeavensBlackHoleZone.arc`: `d7dd8f6245ab8c9fb36b34ceaf1d1983a1efa733a837fddc3a32c48bd44e01bd`
- `build/gateway-content-inventory-20260919/HeavensDoorSmallZone.arc`: `3fea500809a7bf71cb6fb8a2736b82476dc2891a918d940fd37fb4d3d34a1418`

Raw placement rows and zone transforms are also retained in `notes/gateway-content-inventory-20260919/{source-placement-inventory,root-zone,HeavensBlackHoleZone,HeavensDoorSmallZone}.json`. Older availability/factory claims in those notes are stale and were not used. Root LayerA stage rows place BlackHole at `(13917.0625,-1896.3616943359375,-4606.8095703125)` with rotation `(132.539794921875,20.9361629486084,19.288833618164062)`, and Small at `(6680,2078.29248046875,-5310)` with rotation `(180,-69.99998474121094,180)`.

Behavior inspected in current actual owners: `Game/MapObj/{ChipBase,ChipGroup,ChipHolder,ChipCounter,CrystalCage,ExterminationChecker,KeySwitch,CapsuleCage,SuperSpinDriver,SpinDriver}.cpp`, `Game/Enemy/Kuribo.cpp`, and `Game/NPC/{Tico,NPCActor,TalkMessageCtrl}.cpp`. This is a route plan from authored data and source, not a claim that either gate has been completed in the current live run.

## Bounded chip-chain donor check

Compared the full current `ChipBase.cpp`, `ChipHolder.cpp`, `ChipGroup.cpp`, `ChipCounter.cpp` and `CollectCounter.cpp` against their current `decomp/` donors, then checked the actual holder factory and switch write. No concrete missing/empty method or donor-semantic drift was found that blocks pickup/group completion. ChipBase/Holder/Group differ only in explicit includes and CP932 literal conversion. CollectCounter only changes nerve references. ChipCounter uses older `_2C` naming, expanded hidden/completion predicates and explicit nerve execute methods; those methods still execute the same Appear/Wait/End/TryDemo behavior, and its CompleteOut still ends the demo then writes the group switch. The empty Hide nerves and destructors also exist in the donor and are not missing completion implementations.

`ChipBase::isGettable` deliberately rejects the first **40 steps** and only accepts Wait/Flashing while alive. This matters immediately after cage 30 releases chip 17. The actual YellowChip/YellowChipGroup factory entries and SceneObj_YellowChipHolder case are present. `StageSwitchCtrl::onSwitchA` reaches actual StageSwitchFunction storage; global 1009 maps to bit 9. There is no native success stub on this immediate path. This bounded source check does not validate the shared layout/demo engine at runtime; observe count, counter animation, switch 1009 and launch appearance during the supervised run.
