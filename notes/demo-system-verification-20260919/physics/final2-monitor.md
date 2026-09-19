# Final2 original-process observation

The actual original process completed **30,000 frames**, exited **0**, and reaped PID 79773 after **734.585 seconds**. The executable remained SHA256 `dc02030efc629c57d77b7c848b0e348dce2ed2ef33db352259f7cd9148e44a0b`. `final2-summary.json` records the compact result and hashes of the final immutable trace, operator log, runner report, and runtime log. This observer performed read-only monitoring and offline resource analysis; root owned all controller input and rendering capture.

| Observed transition | Frame |
| --- | ---: |
| Bush rabbit 911 caught | 3190 |
| Bush Tico 896 Talk → Wait | 3560 |
| Automatic ordinary A attempt at pipe rim | 3981–4010 |
| Pipe 701 Ready / PlayerIn / PlayerOut | 3990 / 4020 / 4090 |
| Pipe rabbit 906 appeared / caught | 4170 / 4690 |
| Pipe Tico 890 Talk → Wait | 5060 |
| Automatic ordinary A attempt approaching hole | 5761–5790 |
| Hole rabbit 916 appeared / runaway | 5960 / 5980 |
| Hole rabbit 916 caught | 10480 |
| Last Tico 902 Talk → WhiteOut | 10970 |
| Plane gravity 452 appeared; tower 929 appeared | 11140 |
| Rosalina 881 alive and not hidden; tower Wait | 11440 |
| Mario selected plane gravity 452 | 14440 |
| Mario returned to point gravity 418 after falling off | 17300 |
| Mario selected plane gravity 452 again | 22110 |

The sequential operator ended normally at 11440 with `rosetta_alive_not_hidden`. These flags prove original actor activation, not pixel visibility. Root owns the independent screenshots. The process then continued under ordinary supervised approach input until the requested frame limit.

## Third chase and crater route

Across frames 6400–7400, Mario/rabbit separation ranged 1130–1624 units (median 1376), with tangent separation 1098–1438 and radial cosine -0.467 to +0.238 relative to Mario's movement-up. The rabbit remained about 2089–2091 units from gravity-centre actor 418. This differs from the old target almost directly at the centre. Mario's position bounds spanned 884.9 units but net displacement was only 58.9; 61 of 101 samples were status 19 (original Recovery/Warp enum alias), indicating a repeated movement/recovery cycle rather than stationary input failure.

Rabbit 916 repeatedly had Binder ground contact on original part 722, prism 4893, normal (-0.923221, -0.037312, 0.382453), face feature 1, depth about 1.34–1.81, no wall/roof. Its mostly inward velocity was substantially cancelled by the ground reaction. Original `WalkerStateRunaway` uses 1300 units to enter waiting and 1100 to resume fleeing; `exeWait` only turns toward Mario and applies gravity/attenuation. The measured range never crossed 1100 during that window. Waiting is a source-consistent explanation for the rabbit's small movement, though the internal walker substate is not traced.

The parent's A input at 8050 was accepted, but Mario entered status 19 at 8060 and repeated recovery. The later ordinary supervised sideways route crossed the obstacle and produced the third catch at 10480. No production change or forced game state was needed during this run. The separate collision audit identified an authored PullBackCylinder around the crater; this note does not independently claim exact containment from trace-only geometry.

## Tower collision and optional ascent waypoints

At 14440/15040/16040, Mario selected plane owner 452 (priority 1), his original on-ground `_1` flag was set, and his ground triangle belonged to post-demo stairs actor 725. `MarioAccess::isOnGround` returns that movement flag in the ordinary Mario path. At 17290 the flag was clear while falling, so its stored triangle is explicitly treated as cached rather than fresh ground contact.

A read-only disc extraction of `ObjectData/HeavensDoorAppearStepAAfter.arc` supplied 537 KCL triangles, including 139 upward faces. `read_step_kcl.py` decodes their original prism vertex equations and applies the captured zone placement matrix. World vertices of prisms 534/235/236 match actual trace vertices within **0.000956 units**, bounding the transformation error. The selected horizontal tread centres form an outer ascent from local height ~1900 through 1976, 2052, 2200 and 2400 to 2500. They were sent to root as possible ordinary navigation points; this does not establish straight-line traversability between them or completed ascent. Root reports the late stairs attempt (about 29300–29800) did not reach its first target. No unobstructed rendered Rosalina model capture is claimed. The full decoded geometry is retained in `steps-geometry.json.gz`; the raw extracted archive is an analysis input rather than required review evidence.

The live observer (`monitor_live_route.py`, `final2-live-events.jsonl`) records actual actor transitions and gravity changes. Its progress rows select the latest operator decision at or before the sampled frame; after the sequential operator ends, that field intentionally remains its terminal decision while subsequent root-owned approach input is logged separately. Selected complete snapshots and the 6400–7400 chase window are preserved in `final2-milestone-snapshots.json.gz`.

This proves the recorded original route reached three catches and Rosalina activation and sustained the requested process duration. It is not a blanket equivalence claim for every physics operation, collision primitive, authored placement, or rendered pixel.
