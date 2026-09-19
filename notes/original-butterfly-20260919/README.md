# Original Butterfly compatibility — 2026-09-19

## Audit and scope

The canonical `decomp/src/Game/Map/Butterfly.cpp` is complete (486 lines), and `decomp/configure.py` marks the object Matching. No new decompilation is required. The prior real-disc placement capture has three authored `Butterfly` rows, all in HeavensDoorMysteriousZone common ObjInfo (rows 2–4, placement IDs 29–31), with no switch/demo gate and default arguments. `authored-placement-reference.json` preserves these records as a placement reference; its historical `supported` field predates this work and is not a current runtime result.

The fresh native archive symbol audit (`api-symbol-audit.json`) found four missing non-inline shared APIs: `tryStarPointerCheckWithoutRumble`, `calcStarPointerWorldPointingPos`, `calcStarPointerScreenDistanceToTarget`, and `makeQuatUpNoSupport`. All four already have complete canonical utility bodies. The additional missing name `converge` is an existing inline MathUtil template, not a missing provider.

Butterfly declares one Star Piece for each default-argument placement. This dependency is already the actual original `StarPieceDirector`: original GameScene constructs the director and seventy original pooled StarPiece actors, `ObjUtil` forwards declarations/spawns to it, and neither source is excluded from the native Game archive. Player-sleeping and cap-joint queries likewise use actual Mario owners. The ordinary Butterfly archive supplies the model/animations and `buttBody` joint. No extra actor class or replacement owner is needed.

## Implementation

The parent authorized canonical actor import, the four shared donors and one provisional ordinary factory row. `src/Game/Map/Butterfly.cpp/.hpp` are byte-identical to canonical files (no non-ASCII narrow literals require CP932 wrapping). The three pointer helpers are exact original bodies in the existing real pointer-owner provider; the quaternion helper is the exact body in the existing math provider and uses the already available original matrix helper. `source-equivalence.json` records the donor revision and body hashes. The original actor's existing HeavensDoor posture branch, sleeping-Mario behavior and apparently unusual run-away normalization are retained unchanged.

The ordinary factory row names `Butterfly`, its real constructor and its actual `Butterfly` archive. No late-construction hook, forced actor update, actor-specific compatibility behavior or changed story state is introduced.

## Validation gate

`OriginalProcessButterflyTests` starts the actual original HeavensDoor scenario 1 with fresh save storage and only ordinary scripted pointer coordinates. A completed-frame observer waits for normal construction, then checks all three real placements/instances, actual Binder/model/nerve/pointer target, original animal sensors borrowing the actual animated joint, all three one-piece declarations in the original seventy-actor pool, finite original matrices, actual-camera pointer projection round trips and target-distance query contracts. It observes at least 250 normal frames within a 360-frame process run, then checks actor/director/pool retirement.

Compilation, full executable linkage and this actual-process gate are pending. The root's separately running 12,000-frame main chase has GPU priority; this work must not relink the main executable or start another GPU process during it. A focused actor executable link can proceed only against coordinated coherent production sources. Passing this gate would establish bounded construction, normal process execution and ownership retirement, not all Butterfly reactions, sleeping-Mario perching, a visual match, rabbit capture or Rosalina progression.

The first coordinated focused compile found a native header declaration gap: `initStarPointerTarget` lacked the canonical default zero-vector offset. The exact donor declaration was restored in `src/Game/Util/StarPointerUtil.hpp`; the original two-argument Butterfly call remains unchanged. The failure is captured by the shared batch log under `notes/original-mario-actor-sensor-20260919/smg-pc-original-process-trample-jump-tests-build.log`. No executable had linked at that point.

The third shared compile reached the exact target-distance donor and exposed missing generic `TVec2::operator-=`. The existing canonical inline component subtraction was copied into the native JGeometry vector template, preserving the utility body. No per-actor workaround was used. This broad shared-header dependency triggers a coordinated rebuild; the preceding second compile failed independently in the peer's new Mario sensor donor on an LP64 overload ambiguity.

The focused Butterfly executable subsequently linked successfully in 4.418 seconds (`focused-build.log`). Its first actual process reached all three ordinary actors and the actual seventy-actor Star Piece pool at frame 56, then failed the diagnostic's added camera projection round-trip assumption: exit 1, 2.978 seconds, PID 74658 reaped. The complete failed result is preserved as `initial-roundtrip-assumption.json/log`. The actor model/sensor/Binder/declaration checks preceding that assertion passed; this is not yet a passing 360-frame gate. The original camera converts pixel XY through vertical focal length, while actual projection also depends on aspect and shake; a failure-only residual diagnostic is being built before adjusting the test contract. Production code and tolerances remain unchanged.

## Camera contract diagnosis

`camera-residual.json/log` records the second diagnostic failure (exit 1, 2.071 seconds, PID 75248 reaped). Projection was valid, viewport was 608×456 and aspect 4/3, so widescreen was not the cause. At depth 63.3210449 and FOV 31.8907909, pointer (247,180) projected to (247.057892,180.048737). The original `calcCameraDistanceZ` already returns absolute depth, unprojection places the result in front of the camera, and `calcScreenPosition` always initializes output before returning its visibility result.

The actual difference is the original numerical contract: `MR::tan` divides sine/cosine entries at a quantized 14-bit table angle, while projection uses SDK tangent. Independent double arithmetic at table index 725 predicts (247.0573155,180.0482657), within 0.0006 pixel of the measured coordinates. `camera-contract.json` preserves this explanation. The diagnostic now computes the original quantized pinhole ray and actual projection independently in double precision; its 0.05-pixel tolerance is unchanged. The additional world-coordinate bound covers rounded table/focal values and a four-term float matrix product, scaled by 32 float epsilons and the sum of term magnitudes. No production camera/math behavior or owner parameters were changed for this fixture.

## Final actual-process validation — passed

`focused-build2.log` records the final test-only full executable link **PASS** in 4.189 seconds. `original-process-run.json/log` records **360 completed frames**, **exit 0**, **7.959 seconds**, and PID 75819 reaped normally. All three real ordinary factory placements were found at frame 56. The observer verified **304 normal frame samples**, actual models, Binders, nerves, pointer targets, animal sensors borrowing their actual animated buttBody joint, all three one-piece host declarations in the original seventy-actor Star Piece pool, finite original matrices and the independent camera-space contract. Normal scene teardown retired every retained Butterfly, pool actor and director identity.

Final executable SHA256: `1eff471f32c9824b6020821e450b3717dc1c5144f6411e424bd14f5347f6e626`. All target-distance calls in this opening view returned false and preserved caller output; **zero successful target-distance samples** were observed. Successful pointer reactions, Star Piece spawning, sleeping-Mario perching, rendered image parity and later story progression remain outside this bounded gate. No actors were separately ticked, late-constructed or forced into visibility/nerve state. The ordinary factory row is validated for publication under this scope. GPU and normal build slots were released to the parent immediately after the run; no main executable was relinked by this agent.

## Curated files

- `src/Game/Map/Butterfly.cpp/.hpp`: exact existing donor files.
- `src/Game/Util/StarPointerUtil.hpp`: one exact donor declaration restoring its default argument.
- `src/JSystem/JGeometry/TVec.hpp`: exact generic TVec2 subtraction-assignment operator.
- `src/compat/OriginalStarPointerOwnerQueries.cpp`: three exact shared donor helpers.
- `src/compat/GameMathCompat.cpp`: exact shared quaternion helper.
- `src/scene/nameobj/NameObjFactory.cpp`: one ordinary include and factory/archive row.
- `tests/OriginalProcessButterflyTests.cpp` and only its EOF target in `tests/xmake.lua`.
- This note directory (source/symbol/placement evidence, initial failures, numeric diagnosis and passing runtime).

Unrelated pending edits in shared files, especially other test targets and the existing debug-path hunk in `tests/xmake.lua`, belong to their original owners and must be preserved.
