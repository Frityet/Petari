# Original CrystalCage integration — 2026-09-19

Canonical imports, exact creator/utility registration and actual-process initialization/retirement validation are complete. Both focused original-process runs completed 120 frames with exit 0; this proves the bounded ownership path, not cage activation, breaking or full Gateway progression.

The shared DummyDisplayModel data and selection recovery is published in decomp commit `d1ddacad6a59a61ae2bc26e433bc2ce9a9fbab29`; its complete retail proof is in `notes/original-dummy-display-model-20260919/`. CrystalCage has an existing complete canonical donor.

Implemented bounded native scope: import both canonical actors; retain explicit CP932 source encoding for two original Japanese names; normalize the original null constant passed to a bool as `false` and include the previously umbrella-provided RumbleCalculator definition; add the exact original Crystal scene connection wrapper outside Game. Register only the exercised CrystalCageM factory row plus its original placement-dependent dummy-model archive callback.

The actual-process probe observes the three authored Gateway medium cages during natural scene construction and normal opening frames. It checks the three actual break models, original KCL/body sensors, the center SuperSpinDriver/Freeze display model, original Crystal draw category, off global appearance switch 1015 and the center dead-switch / outer listener relationship at 1017, plus normal scene retirement of seven original actors. It does not turn on switches, force a break, inject player messages or claim natural cage gameplay.

Initial complete focused links passed in 4.877 seconds and 5.175 seconds (coordinated build2 logs/JSON). The final corrected-probe links passed in 5.243 seconds and 5.304 seconds (`getter-corrected-builds.json`). The two focused targets are `smg-pc-original-process-crystal-cage-tests` and `smg-pc-original-process-star-piece-placement-tests`; the latter has separate non-pool placement assertions described in `notes/original-star-piece-placement-audit-20260919/`.

`source-equivalence.json` verifies all four imports against the published donor, permitting only the explicitly listed compile changes. Existing CrystalCage donor compilation compares all 24 symbols at or above 99.62963%; constructor, init, after-placement and force-break helpers are 100%.

## First native link frontier

The first focused target compiled both imported actors and the cage probe but failed at link on `MR::getFirstPolyOnLineToMapExceptActor`. The original donor still had two placeholders: this category-0 wrapper and its anonymous category helper. Retail uses an ordinary stack `CollisionPartsFilterActor`, then the existing nearest-hit category query. Both methods were recovered in `decomp/src/Game/Util/MapUtil.cpp` first and copied exactly into `src/compat/OriginalMapQueries.cpp`. The existing filter excludes all collision parts whose actual sensor host equals the supplied actor.

MWCC comparison is 100% for both the 60-byte generic helper and 8-byte public wrapper, with all instruction bytes verified against the DOL and matching relocations. `MapUtil-command.json`, `MapUtil-compile.log.gz` and `MapUtil-verification.json` preserve this evidence. The helper retains an explicit `NO_INLINE` to preserve the original call boundary. No triangle selection, lifetime, collision ownership or actor special case was added. The initial failed link remains in the coordinated build log.

## Initial runtime probe — incomplete

The first CrystalCage run reached the initialized original scene and failed a test assertion about its real break model and dead flag. It exited 1 after 2.739 seconds, PID 78181 was reaped, no debugger was attached, and the 120-frame completion marker was absent. This is not a completed run or a GameSystem crash. Binary SHA256: `8c5bd195c80af77803f234e89a29c6e4a55a371e257d8e0360e16715d765346b`. Evidence is preserved as `initial-break-model-assertion.log.gz` and `.json`. The follow-up diagnostic (`raw-model-storage-diagnostic.log.gz` and `.json`) exited 1 after 2.076 seconds, with PID 79328 reaped, and established that the break actor was correctly dead and its model manager existed; only the directly inspected `mModel` member was null. This was an incorrect probe assumption, not a resource/model failure: original `ModelManager::initModelAndAnimation` stores animated models in `mXanimePlayer->mModel`, and `getJ3DModel()` selects that owner. Both probes now use this original accessor, and the cage probe additionally verifies the break model is the original XanimePlayer model. No production model behavior was changed. The failed runs are pre-zone-transform diagnostics and do not establish completed runtime or correct authored world positions.

The independent JMap audit found the existing host path ignored child-zone transforms. These first diagnostics precede that fix and cannot establish correct authored world placement. The standalone StarPiece probe was not launched after the cage assertion. Both final runs below use the corrected general placement path.

The 100%-matched MapUtil recovery is published as decomp `d97d7e19a5d80e85c8d73327718c050a2b9e1c70`, with the remote pcp-decomp SHA verified.

## Final bounded runtime evidence

`run_probe.py cage` and `run_probe.py star-piece` launch the actual original GameSystem with a fresh console save directory, the real disc, neutral input and 120 requested frames. They check the original frame-loop completion marker, each probe's post-teardown PASS marker, unchanged executable hashes and that the child process has exited. No debugger was attached.

- CrystalCage: 120 completed frames, exit 0, 3.099 seconds, PID 80612 reaped. All assertions ran at frame 57: three actual factory cages, three real animation-owned break models, one fixed SuperSpinDriver/Freeze display model, actual collision/sensor ownership, exact original Crystal category and authored off appearance/dead/listener switches. Normal original scene teardown retired all seven retained identities.
- Standalone StarPiece: 120 completed frames, exit 0, 3.694 seconds, PID 80725 reaped. All assertions ran at frame 56: seven ordinary placements at the corrected original world transforms, separate from the unchanged 70-entry pool and group-created actors, real resource/Binder/sensor/shadow/pointer/material-color owners, and normal actor/director retirement. The actor's original `incNumStarPieceGettable(0)` remains unchanged.

The cage diagnostic now reports world positions near `(15200,-8350,7550)`, rather than the incorrect child-zone-local positions from the failed pre-transform probes. This change comes from the independent general JMap restoration, not cage-specific coordinates. Both final runs also include the separately restored original ModelObj initialization. Their exact binary hashes and process proof are in `crystal-cage-run.json` and `star-piece-placement-run.json`.

These read-only probes leave original switches, nerves, actor positions and gameplay inputs untouched. The cages remain dead behind appearance switch 1015 during the opening. Neither cage breaking, natural StarPiece collection nor the three-rabbit/Rosalina progression is established by these runs.

All published logs are deterministic `.log.gz` files (`mtime=0`, no source filename); the original uncompressed working logs and generated disc/object/runtime-save artifacts are excluded. `verification.json` collects the final claims and evidence hashes.
