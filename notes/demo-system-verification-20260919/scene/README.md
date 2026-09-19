# Original scene setup and omitted-placement audit

2026-09-19. This is a source/data audit plus a validated generic RestartCube registration and diagnostic regression. It does **not** establish whole-scene correctness, collision parity, or a successful current three-catch replay. The parent owns that live run. The parent ran serialized builds and the bounded actual-process probe described below; this auditor performed no additional game/controller run, staging or commit.

## Fresh evidence and retained queues

`fresh-disc-provenance.json` records seven newly extracted Korean-disc archives. Every SHA256 agrees with the prior parsed retail tables. Raw archives stay under ignored `build/demo-system-verification-20260919/`. `classify_placements.py` verifies those hashes before deriving `omitted-placement-classification.json`; every omitted row retains its real zone, full table path, row/l_id, raw arguments/switches and approximate authored world origin. Its double-precision matrix is an audit calculation, not a captured native transform.

The immutable baseline `notes/compat-original-runtime-20260919/gateway-opening-placements.json` reports 243 retained rows: 175 supported, 63 known-unlinked, zero unknown, five StageObjInfo metadata rows. These are the original `StageDataHolder` queues, not an independent host scheduler. The creation order is Mario, `_FC`, `_104`, `_100`, `_108`, `_10C` (`src/Game/Scene/StageDataHolder.cpp:153`). `PlacementInfoOrdered` retains canonical sorting/grouping and skips null creators at actual construction. Thus an unsupported row is not evidence that an actor exists.

The older inventory's three DemoGroup rows were labeled `demo_loader_metadata`; that label is wrong for the actual original process. They are supported `DemoExecutor` actors and are included in 175. Seven RunawayRabbitCollect-owned child rows plus one separate parent-owned child metadata row are not independent top-level factory placements. Only the five attached StageObjInfo rows are excluded as non-actor queue metadata.

## Zone, layer, start, camera and rail checks

Scenario1 has value1 for each zone column. Original ScenarioDataFunction combines Common with the shifted LayerA bit, and StageDataHolder loads only those layers. The root has exactly five active StageObjInfo instances: Mysterious(zone5) in Common, Small(6), Middle(4), Inside(2), BlackHole(1) in LayerA. Large(zone3) is in ZoneList/scenario metadata but has no active StageObjInfo instance, so **that zone is genuinely unattached**. This is distinct from unsupported actors on an attached zone. Raw rows and matrices are in the classification JSON.

Original `findPlacedStageDataHolder` resolves retained archive-entry addresses (`StageDataHolder.cpp:256`); `MR::getPlacedZoneId` and `getZonePlacementMtx` use that owner (`SceneUtil.cpp:341`). Current JMap translation/rotation and all three rail-control-point getters apply its matrix once. Native inventory copies retain local JMap fields and keep world-space descriptors separately. Existing actual-process transform evidence predates this audit; the same focused test is now extended to recheck the new report.

Default selected start is **root zone0, MarioNo0, LayerA StartInfo row0, camera78**: `SceneUtil.cpp:26` defines initial ID `(0,0)` and `SequenceUtil.cpp:50` uses it. Its world position is `(14459.978516,-12791.113281,6059.911621)`. The root camera file contains `s:004e` (row37, `CAM_TYPE_XZ_PARA`). MysteriousZone has a separate MarioNo0/camera999 row and `s:03e7` (`CAM_TYPE_EYEPOS_FIX`); it must not be confused with the selected root start merely because both are StartInfo row0. The report previously could not distinguish them.

MysteriousZone has three Common rails, l_id0/1/2, Bezier/OPEN, with 5/5/6 points. Original lookup uses the rail's row index to select CommonPathPointInfo.N (`StageDataHolder.cpp:168`), and the native getters apply the same attached matrix as actors. Existing process test checks 48 control positions (all three controls of16 points), actual SwitchCube volumes, actor placement and retirement. This does not independently prove every later-planet camera transition or rail movement curve.

## Initial-planet gates and important physics transition

The real MysteriousPlanet row69, two SwitchCubes (Common area l_id8/11), EarthenPipes56/57, original collector and its children, two step models, Rosetta28, two CollisionAreas and seven CollisionBlockers all have current creators. The only missing creator **within MysteriousZone** is SpinGuidanceCube LayerA area row1/l_id3, which requires SW_APPEAR1016.

The two SwitchCubes latch1113/1114 only while B1111 is on. Their all-minus-one arguments do not require spinning, breaking plants, crouching, or grounding. EarthenPipe56/57 use pair1050 and write B1112/1118 after actual transit's PlayerOut; WarpPod52/53 is a different pair and does not reveal pipe rabbits. Collector children form three unique capture groups with two alternatives for the pipe group. Tower part actions on Rosetta write A1015 and local B90; its next part appears Rosetta. Exact sheet/source evidence is retained in `notes/original-rabbit-tower-chain-20260919/`.

Two original gravity fields need distinct runtime checks: Common point gravity l_id0 (range8200, priority0) and LayerA plane l_id4 (range2300, priority1, SW_SLEEP1015). SleepControllerHolder maps OFF to virtual `makeActorDead` and ON to `makeActorAppeared`; GlobalGravityObj sets the field's `mAppeared`, and PlanetGravityManager excludes fields lacking activated/valid-follower/appeared. Therefore tower1015 must activate the authored plane field. A visible tower alone cannot prove that physics transition. Parent has added generic GlobalGravityObj/selected-GravityInfo trace fields for the next run.

## Every baseline omission is accounted for

The JSON maps every one of the63 baseline rows. Four have a new source registration that passed its actual-process runtime gate; the observed probe report is **179 supported,59 known-unlinked,0 unknown,5 metadata**. Classification is deliberately narrower than calling all remaining content unnecessary:

| Rows | Content | What is established / remaining gap |
| ---: | --- | --- |
| 4 | RestartCube | Now registered through exact Cube2 descriptor; original player dispatch and sequence-owned restart data exist. Actual-process120 owner/dispatch probe passed. |
| 4 | ChangeBgmCube×3, AudioEffectSphere×1 | Original audio areas are absent. Audio output is user-deferred, but this is still a setup difference. |
| 2 | Mysterious SpinGuidanceCube, BlackHole | Both require1016. Rosetta's later spin-get demo8 kill writes this SW_DEAD, after her first appearance. This proves their placement gate is later; their construction/owner closure is still absent. |
| 3 | SpinDriver×1, SuperSpinDriver×2 | Gates1127/1017/1009 belong to later enemy/cage/chip launch progression. None is a direct collector/tower-sheet input. Actual launch physics remains unsupported. |
| 8 | Five HeavensDoor RotateParts; KoopaJrNormalShipA×3 | **Real omitted model and KCL geometry.** Fresh ObjectData extraction confirms BDL+KCL in all six distinct archives; ship also has MoveLimit.kcl. Middle parts and ships are ungated; Inside parts require local1. Other-planet placement does not prove they are invisible or physically irrelevant to all possible play. |
| 42 | KuriboMini×8, Kuribo×8, KuriboChief×1, CrystalCageS×7, CapsuleCage×2, RailCoin×2, YellowChip×5, YellowChipGroup×1, BenefitItemOneUp×1, MeteorCannon×2, remaining SpinGuidanceCube×4, ExterminationKuriboKeySwitch×1 | Original content on other attached planets. Per-row switches and dependency explanations are in JSON. No direct initial-collector/tower gate was found; **visual, collision, sensor and later gameplay relevance has not been ruled out**. |

`omitted-geometry-resources.json` contains exact disc paths, SHA256 and file listings for the six missing geometry archives. The rotate-part creator is original `RotateMoveObj`; ship is original `SimpleMapObj`. Original MapObjActor actually checks and creates the archived KCL, so this is a concrete geometry omission, not merely an unimplemented name in a catalog.

## General fixes in the verified source checkpoint

`OriginalPlacementCoverage` previously reported zone=-1 for every original row because it used optional metadata on native JMap copies. It now resolves the original holder and matches the retained source pointer/name to that holder's actual archive entry. The report includes full table path, actual layer directory, zone name/ID, attached-holder index path and original3x4 placement matrix. The archive scan is cached once per archive/report; no placements are parsed again, sorted, filtered, activated or created. Unrelated empty archive entries may share addresses; only ambiguous provenance of an actual retained table fails loudly. The old report is preserved as evidence, not rewritten.

RestartCube's stale reason claimed the real Mario update/restart dispatcher was unavailable. That is no longer true: `MarioCollision.cpp:2046` calls canonical `MR::tryToUpdatePlayerRestartIdInfo`; `AreaObjUtil.cpp:262` dispatches the exact class; `StageSessionGameCompat.cpp:23` writes the actual GameSequenceDirector temporary owner. `src/Game/AreaObj/RestartCube.cpp` compares byte-for-byte with the donor. The generic descriptor now binds Cube2 to the original manager18/capacity64; no Game behavior or stage-specific branch changed. Its current disabled-audio calls have owners; audible BGM output remains outside the user's requested scope.

`OriginalProcessPlacementTransformTests.cpp` now checks all243 provenance identities/six attached zones, the actual selected root start, Rosetta's zone5 LayerA row, all four real RestartCube instances/forms, original area dispatch into real restart state, a miss, restoration, then ordinary teardown. Existing state/actor/rail tests are preserved. Old strict/missing-manager tests now use still-unlinked ChangeBgmCube instead of claiming RestartCube is unavailable. The successful serial run is recorded below; the older standalone-owner fixture failures remain separate.

Remaining bounded candidates: KoopaJrNormalShipA uses already-present SimpleMapObj and real resources but needs ordinary factory/low-model validation; CrystalCageS uses already-present CrystalCage but has different break/dummy archives and must prove that branch. ChangeBgmCube has complete donor source but needs native import/link and current audio-control validation. SpinGuidanceArea is not a descriptor-only fix: native PlayerActionGuidance/createSpinLayout owner is absent. RailCoin's old combined shadow/area reason is partly stale, but `AreaObjRuntimeCompat.cpp:141` still explicitly rejects undecompiled Mercator rail division; do not label the complete general class closed.

First serial build passed. The first process probe then correctly failed during AreaObjContainer validation: I inserted RestartCube(order18) before SwitchCube(order0), violating the pre-existing sorted descriptor contract. The row has been moved between orders17 and23; a compile-time order assertion now catches this catalog error before any process starts. No runtime guard was weakened. The factory fixture also retained obsolete unavailable expectations for already-supported Steam/Coin/PurpleCoin/StarPieceFlow/StarPieceGroup; those assertions now positively check their actual creators, while genuinely unavailable rows retain strict assertions with the concrete actor name in failures. Existing standalone fixture ownership failures are separate from this registry correction.

The corrected CPU AreaObj runner records11 failing legacy cases before this correction: nine require the now-explicit original GameSystem/SceneController (container/manager readiness, CubeCamera, LightArea, generic areas, real-disc camera/message/switch areas and area movement); one was the introduced descriptor order; one still expected an obsolete WaterArea error string. The order is fixed and the water absence check now requires the actual missing scene-owner diagnostic while preserving both Mercator rejection/no-write assertions. The registry fixture positively checks RestartCube's exact form/manager/order/capacity. The nine older owner failures are not passed or suppressed: their standalone constructions need original controller/executor and, for retained-placement cases, original stage/file ownership. The old FileSelect collision fixture additionally hits missing resource-language/FileLoader ownership. The actual-process120 probe is the acceptance gate for the new registration; this audit does not replace these older test assertions with fabricated owners.


## Observed restored validation

Parent's serialized `smg-pc-original-process-placement-transform-tests` build and120-frame run both exited0. Binary SHA256 `6d39a91f3acbf2405424c709334529b7685defba33f119826eff25ad7002a2ce`; runtime3.644592s. Exact command/build details and the observed log lines are copied from `../restored.json` into `validation.json`, without another run. The source log is `../restored-smg-pc-original-process-placement-transform-tests-test.log`.

At frame38 the probe verified243 provenance rows/six attached zones, all four original RestartCube controllers and real GameSequenceDirector dispatch with prior state restored,84 actor translations,84 rotations,48 rail control positions and two SwitchArea volumes. It completed120 frames and verified checked actor/area retirement. The startup summary was179 supported,59 known-unlinked,0 unknown,5 metadata. The separate original-placement-coverage/strict-boundary CPU test also exited0 (binary `b8fde296858c667f5e26e665dd1732379a44c7095db307c7c75754da7edc4b28`).

This proves initialized scene ownership, diagnostic provenance, original restart-ID dispatch and bounded teardown. It does not establish death/respawn behavior, complete scene geometry, a successful three-catch route, or parity with original collision/physics. The fresh main route report was subsequently audited as recorded below.


## Fresh main route placement report

`verify_live_report.py` independently compared `../final-route-placements.json` (SHA256 `031fbd727a70f1c55e65c6d2b97d9b635b9e56b0dea6c07b2f87829e6d35d1e7`) against the hash-revalidated retail inventory, including the selected root Mario row. **All243 row identities matched exactly** by `(zone, full table path, row)` with no missing, additional or duplicate row. Every reported object, zone name, basename, layer, l_id and creator availability matched the source/data classification.

Observed counts are179 supported,59 known-unlinked,0 unknown,5 metadata. Zone totals are0:38,1:24,2:44,4:30,5:84,6:23; layers are205 Common and38 LayerA. Every attached-holder path matched the authored instance order. All matrix translations equal authored floats exactly; the maximum rotation-entry difference from an independent double-precision `Rz*Ry*Rx` evaluation is `1.69573312074e-07`. Full numeric results and exact four supported RestartCube rows are in `final-route-report-verification.json`.

This is a passing **scene placement/provenance inventory** comparison. The59 actual missing creators remain real omissions; this check does not prove their irrelevance, complete actor/collision initialization, correct rendered geometry, or that the main gameplay route has completed. The parent records that route independently.


## Final scene evidence and limits

The native scene changes are included in checkpoint `eda37d29cf809c75dea8d32ab9ab1bb858b2a23c`; the checked decomp checkout is `4ae0d93c3e9b451da9fc05ec9a31533ce44ae843`. No source file was changed during this final evidence audit. `hash-verification.json` confirms all **25 previously recorded source/archive/report/binary digests** still match, the copied probe records equal `../restored.json`, and every quoted probe observation remains in its original log.

The scene evidence to retain is:

- `validation.json` plus `../restored-smg-pc-original-process-placement-transform-tests-test.log`: the passing original-process 120-frame probe, including four real RestartCube dispatches/restoration and normal teardown.
- `final-route-report-verification.json` plus `../final-route-placements.json`: the exact 243-row live inventory match, six attached zones, selected root start, layer counts and matrices. `verify_live_report.py` reproduces this read-only comparison.
- `fresh-disc-provenance.json`: seven stage/scenario archive hashes, freshly rechecked against the extracted disc data.
- `omitted-placement-classification.json` and `classify_placements.py`: the complete per-row account of the original 63 omissions, with four validated RestartCube rows now registered and 59 creators still absent.
- `omitted-geometry-resources.json`: six distinct real object archives proving eight missing model/KCL placements. Raw archives and executables are excluded from the evidence commit.
- `manifest.json` and `hash-verification.json`: curated paths, source checkpoint, checksums and the final digest audit.

The decisive limitation is **59 actual missing creators, including eight confirmed geometry/KCL placements**. Other-planet placement does not prove these harmless. This scene audit does not establish death/respawn behavior, every actor's initialization and switch transition, complete visible geometry, or collision/physics parity. The focused probe and inventory match also do not establish three catches or Rosalina's appearance in the parent's new route. Legacy standalone AreaObj/FileSelect fixture ownership failures remain recorded rather than bypassed.


## Authored crater recovery in the final2 route

The read-only [PullBackCylinder audit](pullback-final2.md) maps the real zone5 Common row0/l_id6 into world space using the original degree-rotation convention. All17 sampled entries into status19 during frames5000–11000 occur inside that authored volume, with their immediately preceding samples outside. This supports ordinary crater recovery as the cause of those repeats; no area relocation or gameplay workaround was applied. The detailed JSON preserves the original row, attached matrix, numerical limitations and trace-prefix hash. Status19 alone cannot distinguish Recovery from Warp, and this bounded result is not complete physics parity.
