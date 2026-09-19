# Curved-planet orientation and recovery observations

The user reports intermittent wrong Mario rotations while moving on the curved planet and a starting DemoRabbit that does not rotate. Earlier completion evidence does not settle these visual problems. This pass treats them as open and separates available motion vectors from the missing final draw pose.

`analyze_final2.py` reads the previous 30000-frame final2 trace without changing any game state. `final2-vector-analysis.json` contains selected locomotion statistics, worst-case samples, and all repeated crater recovery transitions. The selection requires status 0, the original on-ground flag `_1`, and processed stick magnitude above 0.1 within the guide and three chase windows.

| Grounded locomotion window | Samples | Head vs inverse air-gravity median/max |
| --- | ---: | ---: |
| Guide, 1450–2620 | 72 | 7.925° / 17.869° |
| Bush chase, 3060–3190 | 13 | 7.584° / 8.327° |
| Pipe chase, 4220–4690 | 47 | 8.713° / 15.970° |
| Hole chase, 5980–10480 | 59 | 3.939° / 9.785° |

The recorded head/front vectors are almost orthogonal (maximum absolute dot about 0.00077 across these chase samples), and these selected grounded samples do not contain a 180-degree head-vector flip. This does not prove the model was drawn upright. Head-to-ground angles reach 43.114° in the hole window; the contact polygon may be cached or sampled after different update phases. At plane selection, head versus air-gravity reaches 49.883° at frame 14470 while head versus ground is only 6.664°, which must be correlated with the exact gravity and posture update order before labeling a defect.

The starting guide, DemoRabbit 874, retains constant authored Euler rotation while its gravity changes 147.251° from first alive frame 1050 to 2690. **That constant Euler is not its draw orientation.** `DemoRabbit::control` blends inherited quaternion `_A0` toward inverse gravity and `mFrontVec`; `NPCActor::calcAndSetBaseMtx` normalizes `_A0`, optionally rebuilds it if cached `_CC` differs from authored rotation, and writes the model matrix. The previous trace contains neither `_A0` nor that matrix. Comparing `mRotation` to gravity cannot confirm or reject the user's symptom.

The repeated crater status transitions are explicit: 5980→6140, 6240→6400, 6500→6660, then roughly every 260 frames until the ordinary sideways route clears the region. The numeric status is 19, aliased Recovery/Warp by `MarioState.hpp`. The earlier authored PullBackCylinder finding explains one trigger candidate; it does not by itself prove correct volume transformation, recovery motion, or presentation. Those remain valid diagnostic targets.

## Debug trace expansion

Only `src/runtime/OriginalProcessTrace.cpp` changed, inside its existing `#ifndef NDEBUG` implementation. It now records borrowed stored data without recalculating pose or animation:

- Generic ModelManager-owned J3D base matrix/scale, internal view, and joint-index-0 animation matrix when its existing buffer is present.
- Mario active model and normal model 0, actor base matrix, actual model index, `_C4`/`_F4` posture matrices, side vector, direction-up `_1FC`, yaw offset, bound/fixed-matrix/update flags, and active MarioState RTTI.
- NPC quaternion `_A0`/`_B0` and cached Euler `_CC`; DemoRabbit desired front and no-ground timer.
- RunawayRabbit quaternion/front/player-bind pose, actual actor matrix and nested Walker state/direction/counter/speed, to distinguish waiting from a stalled flee algorithm.

`ModelManager::getJ3DModel` only selects `mModel` or the already-owned Xanime model. `MarioActor::getJ3DModel` only selects `mModels[mCurrModel]`; the trace checks the index before that getter. `LiveActor::getBaseMtx` is the inherited pure getter for the traced player/rabbit classes. Matrix/index buffer existence is checked before reading joint 0. No `calc`, `make`, model registration, or owner creation operation is called.

This trace is a post-frame observation, so it does not by itself identify which matrix was uploaded in a preceding GX draw. Root owns the paired rendering/J3D upload audit and new runtime probes. The first expansion compiled and produced the completed 4000-frame `orientation-before2` trace. The subsequent camera-up and saved-recovery fields also compiled and produced the completed corrected 12000-frame run below.


## Stored-matrix baseline (before shared vector equality restoration)

`orientation-before2.json` reports 4000 completed frames, exit 0, no surviving process, and unchanged bundle SHA `c4f8cdbdbc83f65bc0a8f6a0902ae904bfd8f1e0897b6a8a68ecd970c8816bf3`. The trace samples every ten frames through 3990. `before2-matrix-analysis.json` records its content hash and selection bounds; `before2-matrix-samples.json.gz` retains all per-sample matrix diagnostics and status/ground flags.

The starting DemoRabbit 874 has exactly one quaternion and one base rotation across all 165 alive, unclipped observations from 1050 through 2690. Its stored base-up versus inverse-gravity angle reaches 157.350 degrees at 2230; its front versus desired front angle reaches 179.285 degrees at 1460. Joint 0 agrees with the same fixed base. The independently reconstructed quaternion matrix agrees within 5.27e-8 per rotation element. Thus the original control pose is being reset before drawing; this is stronger evidence than the earlier constant authored Euler observation. Both other DemoRabbits also retain one quaternion/base rotation while their intended fronts change.

For Mario's 150 status-0, on-ground observations with accepted stick magnitude above 0.1, stored base-up agrees with direction-up `_1FC` within 1.5e-6 degrees. Its maximum deviation from head is 9.510 degrees and from inverse air gravity 10.908 degrees. Base determinant ranges 0.99999960–1.00000001; all matrices are finite, right-handed and orthogonal to float precision. Joint-0-up differs from base-up by at most 1.900 degrees. Joint-0 front yaw reaches 53.224 degrees at 3560, while up still agrees; animation/turning needs separate correlation before classifying that yaw as wrong. The analyzer also reports the narrower unbound subset.

Across all alive samples, the largest Mario base/head and base/joint0-up mismatch is 135.540 degrees at 3890. That frame has original binding enabled, on-ground false, zero velocity and a pipe transition; the joint up matches head while the stored base follows the binding direction. It is distinct from unbound planet locomotion. At screenshot frame 2300, stored base, head, inverse gravity and joint-0 up agree within 0.00003 degrees. Neither fact rules out an intermittent upload/animation/camera defect elsewhere or between ten-frame samples.

## Reusable analyzer and trace getter audit

Run `python3 notes/demo-rotation-recovery-20260919/observations/analyze_matrices.py TRACE.jsonl OUTPUT.json --samples SAMPLES.json.gz`, optionally adding `--start-frame N --end-frame M`. It accepts live files by excluding only an unfinished last line; malformed complete records fail. The report separates status/active-state groups, status 19 recovery/warp, normal grounded movement, and grounded unbound movement. It reports camera-up lag and stable-head *snapshot* intervals once those fields exist; equality of two sampled heads does not establish that every intervening frame was stable. Saved safety candidates and active warp/recovery fields are retained without choosing a safety point on the game's behalf.

Seven offline analyzer tests pass (`analyzer-tests.log`): known column-axis quarter turn, independent quaternion sign/rotation, scale/reflection/shear distinction, missing/nonfinite/degenerate input, live partial-tail behavior, frame-window selection, and camera countdown versus repeatedly reset timer. These validate the analysis tool, not gameplay correctness.

The trace call graph was rechecked: `OriginalModelAccess.cpp:64` only selects the existing ModelManager; `ModelManager.cpp:319` chooses its static or Xanime-owned model; `MarioActorDraw.cpp:1021` indexes the existing model array after the trace's index check; `LiveActor.cpp:218` returns that stored base matrix; `J3DModelData.hpp:36` → `J3DJointTree.hpp:39` reads joint count. No method in those chains calculates animation or matrices. `getLastSafetyTrans` is deliberately not called because it can call `Triangle::calcAndGetNormal`; the new fields read both stored safety candidates and cached normals directly.


## Corrected 12000-frame observation

The corrected process completed all 12000 frames, exited 0, and was reaped; bundle SHA stayed `4658f1cc3d770857e5e5c1a5d36d64103dab2e6a61bffb8f62c8bfd3a9f475f6` (`../after.json`). The analyzer retains 1200 complete ten-frame observations through 11990. Final outputs are `after-matrix-analysis.json` and `after-matrix-samples.json.gz`; `after-summary.json` retains state transitions, recovery candidates, and route milestones.

**This is mixed ordinary live input plus resumed scripted control, not a matched before/after route.** The first notes-only operator failed before publishing its first decision because the optional manual-input file did not exist. `after.log:49,52` confirms empty script revisions at frames 0 and 1405. Nevertheless, the controller trace records ordinary host input: held A at 2440, stick input from 2550–3550 and 5300–5550, plus further button events. `after-preoperator-controller.json` records all sampled nonzero runs. The native frontend samples window input before applying optional scripts (`OriginalGameApplication.cpp:361–390`); empty scripts do not clear ordinary controls (`DebugWpadInputScript.cpp:266–293`). The trace does not identify the human input source. The resumed notes operator's first decision is frame 6820; its first nonempty revision is accepted at 6829, so 6830 is the first post-receipt trace sample. `after-preoperator-matrix-analysis.json` ends at 6820; `after-resumed-matrix-analysis.json` starts at 6830.

The starting DemoRabbit 874 now records 238 distinct quaternion/base orientations across 377 alive, unclipped observations from 1050 through 4810. Its base-up versus inverse gravity median is 0.02305 degrees, maximum 5.39656 degrees during initial appearance at 1050. Base front versus intended front median is 0.03085 degrees; a transient 65.36-degree turn at 2760 remains visible numerically, so this is not a claim of instantaneous direction matching. Joint 0 and the reconstructed quaternion agree with the stored base to float precision. The earlier baseline's constant orientation is absent. Different paths and dwell times prevent treating the aggregate medians as a controlled numerical A/B comparison; the direct frozen-versus-updating observation and separate original-owner equality regressions provide the stronger evidence.

Mario has 331 grounded, unbound, status-0 observations with accepted stick magnitude above 0.1, of which 76 precede the resumed script and 255 follow it. Across those samples, base-up agrees with `_1FC` within 1.3e-6 degrees; joint-0-up agrees with base-up within 1.5e-6 degrees. No nonfinite matrix, reflected orientation, or degenerate basis occurs. Base determinant spans 0.99999961–1.00000001, with maximum normalized cross-axis dot 6.35e-8. In the resumed-script subset, head/base-up mismatch peaks 14.000 degrees and inverse-gravity/base-up mismatch peaks 13.382 degrees at 9550. Camera-up/head lag reaches 26.329 degrees at the same fast-moving frame; this is an observation of original smoothing during changing orientation, not automatically an error.

The camera's equality-dependent countdown now visibly completes on stable-head snapshots: timer 13 at 2480 → 3 at 2490 → 0 at 2500, with camera/head lag 0.1807 degrees → 0.0000394 → 0. Timer 11 at 3530 → 1 at 3540 → 0 at 3550 likewise settles. There are 798 adjacent stable-head snapshot intervals; 714 end with timer 0. Identical sampled heads do not prove every intervening frame was unchanged; the separate actual-owner same-head regression provides that focused check.

All 1196 alive Mario samples have finite, right-handed base and joint-0 matrices. The largest full-run camera/head angle (147.520 degrees) and model/head angle (80.106 degrees) occur at 8210 during bound pipe transit, with on-ground false. These are kept distinct from unbound walking. Animated joint scale gives determinants from 0.77807 to 1.05604 while normalized handedness and orthogonality remain intact; non-unit animated scale is not classified as a defect.

The only observed status-19 episode is 9770–9920, with normal status returning at 9930. Active state RTTI identifies `MarioWarp`, mode 3, rather than guessing from the shared Recovery/Warp numeric status. Its destination `(13098.4424,-9620.4639,7968.3994)` matches the latest stored safety point plus cached normal ×160 within 0.000388 units; the prior candidate differs by 9.363 units. During this episode, model/head up mismatch stays below 0.545 degrees, camera/head lag below 0.951 degrees, and joint/base up agrees. No repeated recovery cycle occurs in the remaining trace. This supports the recovered destination calculation in the observed episode; it does not establish every crater boundary, surface intersection, or visual bubble behavior correct.

The resumed controller records catches at 7350, 8970 and 10590, with ordinary post-catch dialogue completion at 7720, 9340 and 11080; Rosalina becomes alive/not-hidden at 11550. That is flag and route progression evidence. Pixel visibility, all-animation correctness and all-area collision correctness remain separate claims.
