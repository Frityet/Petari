# Gateway scenario 1 content inventory through Rosalina's appearance

Read-only audit on 2026-09-19 against the current source and real Korean disc. **No additional unavailable top-level factory entry was identified as necessary for the direct wakeup, three-rabbit reveal/catch, or tower/Rosalina appearance sequence.** The inventory remains a source-and-metadata conclusion. CrystalCageM and standalone StarPiece are now published in checkpoint `066d765bed38f95270b4ea85434d87d0750fd663`; each passed a 120-frame actual-original-process ownership/retirement probe, and the integrated main completed 18000 frames with exit 0. Those results do not establish natural cage breaking, StarPiece collection, all three rabbit catches, or visual parity.

## Inventory scope and method

`source-placement-inventory.json` records every active authored row, original zone ID, table, row index, l_id, constructor category, raw arguments/switches, source hashes and archive hashes. `compare_inventory.py` compares the ordinary factory table, complete area descriptors and actual PlanetMap catalog; it does not assume a matching object archive makes an actor supported. All input parsing and comparison scripts write notes only. No production code, builds or GPU processes were used.

The real scenario 1 table selects Common+LayerA. Following actual StageObjInfo instances yields root HeavensDoorGalaxy and Mysterious, Small, Middle, Inside, BlackHole zones. HeavensDoorLargeZone is in ZoneList and scenario columns but is not instantiated by these active stageobj rows. This avoids counting every zone-list name as placed content.

Across all instantiated zones the source inventory has 166 explicit ordinary factory/area rows, five PlanetMap catalog rows, seven known collector-owned children, five zone-holder metadata rows, three demo-loader metadata rows, 63 unavailable direct factory rows and one later child whose parent is unavailable. Those overall unavailable counts are mostly later tutorial planets; they are **not 63 blockers to the requested initial demo**.

## Starting-planet and tower coverage

These are zone 5, HeavensDoorMysteriousZone. Table paths begin `/jmp/placement/` unless stated otherwise.

| Content | Exact active placements | Current original owner |
| --- | --- | --- |
| Starting planet | Common ObjInfo row 24, l_id69 `HeavensDoorMysteriousPlanet` | Actual PlanetMapDataTable row 209 selects ordinary `PlanetMap`; every submodel flag 0. The absence of a hard-coded factory row is expected. |
| Planet gravity | Common PlanetObjInfo l_id0 `GlobalPointGravity`; LayerA PlanetObjInfo l_id4 `GlobalPlaneGravity` | Generic original gravity constructors and registered owners. |
| Bush/hole reveal | Common AreaObjInfo rows 1/2, l_id8/11 `SwitchCube` | Original `SwitchArea`, Cube2, SW_B1111 then SW_A1113/1114; no plant-specific reveal helper. |
| Hiding scenery | Common ObjInfo l_id43 `CutBushGroup`,13 `FlowerGroup`,14 `FlowerBlueGroup`,187 `HeavensDoorFlowerA` | Exact PlantGroup variants and SimpleMapObj factory. |
| Pipe reveal | Common ObjInfo rows 18/19, l_id56/57 `EarthenPipe` | Actual EarthenPipeMediator pair 1050, original binder/animation transit and source SW_B1112/1118. |
| Invisible passage | Common ObjInfo l_id52/53 `WarpPod` | Original WarpPodMgr and pair/group ownership; distinct from the rabbit-revealing pipes. |
| Rabbit collector | LayerA ObjInfo row 11, l_id4 `RunawayRabbitCollect` | Collector directly constructs four RunawayRabbit and three RunawayTico children from its authored child table. |
| Guide/hidden children | `/jmp/childobj/layera/childobjinfo` rows 0–6: Tico l_id3/4/5; rabbit l_id6/7/8/10 | These have ParentID4. Missing independent factory rows are **not missing dependencies**. Three unique rabbit groups, with two alternatives in pipe group1. |
| Opening demo rabbits | LayerA ObjInfo l_id66/67/68 `DemoRabbit` | Ordinary original DemoRabbit, three cast identities and actual rail resources. |
| Tower during demo | LayerA ObjInfo l_id82 `HeavensDoorAppearStepA` | Exact HeavensDoorDemoObj, SW_APPEAR1015, original model animation, stage effects and collision. |
| Tower after demo | Common ObjInfo l_id83 `HeavensDoorAppearStepAAfter` | Exact SimpleMapObj; original demo action appears it at tower fade-in and removes the during-demo model. |
| Tower collision boundaries | Common AreaObjInfo l_id18/19 `CollisionArea`; Common ObjInfo l_id91/94/95/188/189/190/191 `CollisionBlocker` | Original area/blocker actors and generated KCL ownership already registered. Natural SW1015 transition remains a live gameplay validation gate. |
| Tower cages | LayerA ObjInfo l_id71/72/73 `CrystalCageM` | Published original CrystalCage row plus DummyDisplayModel dependency; all three cages and their real owners passed the 120-frame initialization/retirement probe. Natural activation/breaking remains unproved. No cage-breaking action is needed to make Rosalina appear. |
| Rosalina | LayerA ObjInfo row 12, l_id28 `Rosetta` | Original NPCActor/Rosetta and RosettaDemoHeavensDoor1. LightDome and DomeHalo are real owned PartsModels, not independent missing placement factories. |
| Ambient/contact content | Common ObjInfo fourteen PunchingKinoko and three Butterfly; ten Coin; seven Common/LayerA StarPiece | Published ordinary actor owners; all seven standalone StarPiece placements passed the 120-frame initialization/retirement probe, separate from the 70-entry pool. Collection remains unproved. |

The actual TicoGuideDemo action sheet up through `高楼出現[デモ後]` references only available DemoRabbit, collector-owned RunawayTico, Rosetta, and Rosetta's LightHalo. The fade-in also references both existing step models and surviving Tico casts. Timekeep executor, actor cameras, talk controller, original action dispatch and zone-transformed general positions remain real owner dependencies; this inventory does not prove their full natural progression. Detailed switches/parts are in `../original-rabbit-tower-chain-20260919/README.md`.

## Actual missing rows and why they do not justify expanding this gate

The **only unavailable direct row in MysteriousZone** is LayerA AreaObjInfo row 1/l_id3 `SpinGuidanceCube`, SW_APPEAR1016. It uses original `SpinGuidanceArea` and `SceneObj_PlayerActionGuidance::createSpinLayout`, with `SpinGuidance` layout resources. Canonical source exists in decomp but is not imported/registered natively. The manager descriptor alone is not support. The authored tower appearance actions write 1015 and local 90, not 1016; this is later spin guidance, so it does not block Rosalina's initial appearance. No change was made or proposed to bypass its gate.

Root-zone missing rows are Common RestartCube l_id0/2/3/4; LayerA ChangeBgmCube l_id17/22/23, SpinDriver l_id2 (SW1127), and SuperSpinDriver l_id5/8 (SW1017/1009). The launch objects are later traversal; the RestartCube volumes are on those later-route locations, outside the starting planet. Their original owner dependencies are restart-ID/checkpoint state, automatic bind/rail traversal, and audio control respectively. They are real missing coverage for the full tutorial, but not a prerequisite to the inspected rabbit/tower path. Audio remains outside the current priority.

The other zones contain missing enemies/items/tutorial actors and five explicit rotating geometry rows: MiddleRotatePartsA/B and InsideRotatePartsA/B/C, whose canonical constructor is `RotateMoveObj`. These are later Small/Middle/Inside/BlackHole route content. They are recorded in the complete JSON so they are not mistaken for supported geometry, but this audit does not recommend importing that broader route before the requested appearance gate works. Distant visual parity has not been established.

## Live pipe mapping supplied to the operator

`live-pipe-route.json` records a read-only snapshot from `content-world-first-catch-18000-actors.jsonl`, frame 12290. Runtime 697 maps to zone 5 EarthenPipe l_id56, and 699 to l_id57. Transit 697→699 reveals rabbit 902 through SW1112; transit 699→697 reveals rabbit 917 through SW1118. The other live pipe IDs 781/782 belong to the later root route. The JSON contains actual traced top positions and binder/approach offsets derived from original gravity-up. No controller or gameplay state was written by this audit.

## Published validation update

The raw probe result files were re-read for this update. CrystalCage completed 120 frames, exit 0, 3.099 seconds, PID 80612 reaped, with probe and retirement passing. Standalone StarPiece completed 120 frames, exit 0, 3.694 seconds, PID 80725 reaped, also with probe and retirement passing. Both used the restored original zone-transform path. Detailed assertion scope and earlier diagnostic failures remain in `../original-crystal-cage-20260919/README.md`; no failure evidence was removed.

The integrated original main SHA256 `b38e90868eb8ab1f1cdc72487c293cf0f15a3e50c6fd8b14adc846bec7a7d123` completed 18000 frames in 372.303 seconds, exit 0, with its process reaped. A fresh read of the 10-frame-interval trace finds the first Caught samples for bush rabbit 907 at 3490 and pipe rabbit 902 at 17670. Both are Stop/dead at the final 17990 trace sample. Hole rabbit 912 remains Hide/hidden, and Rosetta 877 remains dead; the third catch and Rosalina appearance are not complete. These are sampled observations, not claims of exact event timing between trace samples.

`validation-update.json` links the original probe/run records and records their hashes and these bounded trace observations. The sequential input route is owned by the parent/peer driver task; this audit wrote no controller state.
