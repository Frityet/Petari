# Standalone StarPiece placement audit — 2026-09-19

Initial read-only assessment followed by bounded native activation. The exact ordinary factory row is now enabled provisionally and a separate actual-process placement probe is linked. The actual-process placement probe passed 120 frames and normal scene retirement; no gameplay collection result is claimed.

## Readiness and exact missing row

The complete native `src/Game/MapObj/StarPiece.cpp` matches `decomp/src/Game/MapObj/StarPiece.cpp` byte-for-byte after removing its explicit CP932 include and one CP932 literal wrapper. All methods are already linked for the original pool and group-created pieces.

Retail `NameObjFactory.cpp:5167` registers `"StarPiece", createNameObj<StarPiece>, "StarPiece"`. The native creator subset previously had StarPieceGroup/StarPieceFlow but omitted this ordinary row; this change adds the exact original StarPiece row. The existing archive catalog already contains `StarPiece → StarPiece`, and there is no placement-dependent StarPiece callback in the original callback table. Restoring the exact creator row therefore appears sufficient for compile/link closure; normal placement was validated independently by the final actual-process probe below.

## Actual authored inputs

Fresh decoding of the already extracted, hashed real `HeavensDoorMysteriousZone.arc` confirms seven StarPiece entries: IDs 84–86 in common and 87–90 in layer A. All eight arguments, all stage switches, rail IDs and demo references in the active placement inventory are `-1`. The full original-process placement inventory separately records zone 5 and transformed coordinates; it is preserved as an explicitly prior placement reference, not rerun evidence.

`fresh-stage-metadata.json` is the new BCSV decoding of the actual archived data. `verification.json` records the archive/source hashes and verifies all 261 instructions of retail `StarPiece::init` against the local verified DOL.

## Pool versus standalone ownership

`GameScene::init` constructs StarPieceDirector before `SceneFunction::startActorPlacement`; it creates the pool only afterward with `MR::createStarPiece`. Therefore an ordinary placed StarPiece can safely increment the real director's construction counter during its original constructor.

The 70 pool entries are constructed with the original pool name, initialized with an invalid iterator, explicitly made dead, registered into the director's LiveActorGroup and registered for demos. Their existence proves the shared constructor, model/material, Binder, sensors, shadow, pointer, animation and effect setup are linkable and have been exercised.

A standalone actor follows a distinct valid-iterator branch: it loads the authored position/rotation, selects color from argument 3 (random for these seven), reads shadow behavior from argument 4, chooses the Floating nerve, and finishes with `appear()`. It is owned by the normal scene placement graph and must **not** be added to the director's 70-entry reusable pool. `mGroupType == groupType_noGroup`, `isGroup == false`, and host/receiver pointers begin null. There are no StageSwitch registration calls in this actor's initialization; these seven placements also author no switches.

The unusual `incNumStarPieceGettable(0)` in valid standalone initialization is faithful: retail loads `r3 = 0` at `0x8023D2AC` before the call. Do not replace this with an inferred increment. Every constructor increments the distinct `mNumStarPieceNewed` counter; a final counter assertion must account for group-created pieces as well as the 70 pool and seven placements.

## Required bounded activation proof

The new `smg-pc-original-process-star-piece-placement-tests` target observes natural scene initialization through the original-process debug observer. Distinguish the seven real placement actors from both director pool membership and StarPieceGroup children; verify original initialized placement transforms, live Floating state, default color membership, bound material-color storage, actual model/Binder/two sensors/shadow/pointer/BTK resources and normal teardown. Verify the pool still contains exactly its original 70 members and none of the seven placed identities.

Do not infer collection success from initialization. The first real Floating update additionally computes gravity, enables original clipping, rotates and runs the pointer/collection checks at the authored locations, which differ from pooled actors at the origin. Natural pointer collection and visual rendering remain later gameplay evidence.

## Final actual-process validation

The exact ordinary creator row is enabled. `tests/OriginalProcessStarPiecePlacementTests.cpp` passed against the restored original zone-transform path: assertions at frame 56, all 120 requested frames completed, exit 0, PID 80725 reaped, 3.694 seconds, and normal scene retirement verified. The probe uses `ModelManager::getJ3DModel()` so either original model-storage route is valid.

The seven actual actors were matched one-for-one to the original transformed placements and separately identified from the unchanged 70-entry pool and group-created StarPieces. Their original Floating state, unit scale/default flags, no-group ownership, actual animation/Binder/sensors/shadow/pointer storage, borrowed material color and director construction count passed. No pool membership, authored transform, story/switch state or original initialization behavior was changed by the test.

Run `python3 notes/original-crystal-cage-20260919/run_probe.py star-piece` from the repository root. Exact executable/process evidence is in that directory's `star-piece-placement-run.json` and deterministic `star-piece-placement-run.log.gz`; the common final `verification.json` also records hashes. This proves initialization and retirement, not collection gameplay.
