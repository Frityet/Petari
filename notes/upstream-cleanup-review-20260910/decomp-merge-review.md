# Upstream reference merge review — 2026-09-10

The `pcp-decomp` merge is resolved and staged for parent review, intentionally uncommitted. The safety branch `codex/pre-cleanup-upstream-20260910` points at pre-merge `db38bcbac05d051fa9b319306a7ba43cb84f37ae`. `MERGE_HEAD` is exact fetched SMGCommunity commit `16906807c69697fdea5d7efbb4642313c1b14204`. The common base is `151d5a53a3c5dd0ea295ddc66f2c8b958b4dff88` (43 local commits and 104 incoming commits). No root production source or Xmake configuration was changed by this merge task.

The incoming delta has 386 paths; the resolved result changes 384 paths relative to local HEAD. Full path/category inventories and before/base/upstream blobs are retained here. Root integration should map from the resolved decomp tree using `db38bcb` as the source base, preserving native architecture and ownership boundaries. Root remains a flattened PC repository; importing upstream's Wii build/root/include hierarchy directly would be inappropriate. The previous sync's ancestry recording plus explicit source mapping remains the safe root integration path.

## Conflict dispositions

| Path | Resolution and reason |
| --- | --- |
| `include/Game/Effect/ParticleEmitterHolder.hpp` | Adopt upstream declaration spelling and offset comments; layout unchanged. |
| `src/Game/Effect/ParticleEmitterHolder.cpp` | Retain complete locally recovered seven methods. Fresh Wii proof remains 100% text and all seven methods 100%; incoming formatting/alternative STL spellings offer no behavioral improvement. |
| `include/Game/Player/Mario.hpp` | Retain recovered bool `damagePolygonCheck`; adopt upstream const sensor pointer and bool `_60D` names/types; retain other independently recovered declarations. |
| `include/Game/Player/MarioParalyze.hpp` and `src/Game/Player/MarioDamageParalyze.cpp` | Adopt upstream `mTimer`/`mNotDecLife` names and equivalent five method bodies. Preserve original nerve initializers present in local and retail TU. Full text99.877815%; five methods99.6–100%, vtable100%. |
| `src/Game/Player/MarioActorSensor.cpp` | Preserve complete recovered sensor/trample/attack bodies where incoming only reformatted comment stubs. Keep nonconflicting incoming changes. |
| `src/Game/Player/MarioMove.cpp` | Preserve full recovered movement bodies. Incoming calcMoveDir explicitly lacks the negative-gravity operation; adopting it would reverse the reference basis. Keep disjoint incoming comment edit. |
| `src/Game/Player/MarioSpecial.cpp` | Preserve full locally recovered functions, including updateOnimasu absent upstream. Adopt const sensor access without casts. Full text99.26617%; isHeadPushEnableArea retains actual high-word `_37`, not incoming `_17`. |
| `include/Game/Util/AreaObjUtil.hpp` and `src/Game/Util/AreaObjUtil.cpp` | Preserve complete bool water query and complete area movement implementation; incoming still has comment-only alternatives and wrong water return declaration. Adopt eight disjoint upstream helpers and real upstream AreaForm member renames. Add missing declarations for world box/restart helpers. |
| `src/Game/Player/Mario.cpp` | Resolve by function, not concatenation. Keep51 unique definitions. Adopt four upstream body edits; preserve nine independently recovered overlapping bodies and the recently verified ground-check return assignment. See detailed inventory below. |

The ten cleanly merged local-overlap paths were retained and inspected: MarioWait.hpp, MSL functional.hpp, EffectKeeper.cpp, CollisionParts.cpp, WarpPod.cpp, NameObjCategoryList.cpp, MarioJump.cpp, MarioWalk.cpp, StopSceneController.cpp and LiveActorUtil.cpp. Their affected full source TUs compile. In particular the MSL const/ref functor helpers remain present alongside upstream overloads.

The eight adopted area helpers are calcCubeAxisZ, calcCubeWorldBox, getCubeLocalBox, calcCubeLocalPos, calcCylinderPos, calcCylinderUpVec, getCylinderRadius and tryToUpdatePlayerRestartIdInfo. The first seven match100%; restart matches99.28571%. The retained water query has the same source operations, with `_14/_20/_24` changed only to upstream radius/height field names. Its existing88.19% bounded match is not a claim of complete gameplay fidelity.

## Mario per-symbol review

`mario-symbol-dispositions.json` inventories all51 definitions. Four upstream-only body edits were adopted: constructor boolean spelling, checkKeyLock boolean spelling, equivalent setGroundNorm temporary reduction, and the substantive writeBackPhyisicalVector repair. The latter restores the original `_1C._D` branch around jump-component clipping. Independent retail review by depth_completion and initialization_completion confirms `0x802ACE20–0x802ACE28` load word0x1C and test bit13; false selects the projected jump component branch, true selects the gravity-angle redirect at0x802ACEBC. The old local source incorrectly selected the redirect as the `f28 > 0` arm.

`mario-writeback-retail.s` and `mario-writeback-retail-constants.json` retain the full function plus float constants. The angle threshold is0.104719758 (`0x3dd67750`, equivalent to `MR::toRadian(6.0f)`), velocity projection compares0, rebound factor is0.5, upper-punch normal threshold0.707, wall probe distance300. All other numeric expressions in the adopted body preserve the retail operations. Full original Mario.cpp compiles before/after; writeBackPhyisicalVector improves97.61364→98.42975%, full text98.23053→98.29829%.

The nine overlapping recovered methods retained are update, checkForceGrounding, inputStick, createDirectionMtx, createCorrectionMtx, createAngleMtx, fixHeadFrontVecByGravity, postureCtrl and updateLookOfs. Concrete rejected regressions include inputStick's retail0.01 threshold versus incoming default0.001; checkForceGrounding's retail0.99 versus incoming0; postureCtrl's missing slip-floor negation; and two reversed fixHeadFrontVecByGravity predicates. Remaining matrix/update differences provide no demonstrated behavioral improvement. Existing standard-C++ bool comparisons in fixFrontVecByGravity/setFrontVecKeepSide are preserved against incoming comparisons to nullptr. The `mMovementStates._1 = checkGround()` return assignment stays intact.

## Two additional integration fixes

Incoming SwitchWatcherHolder::movement walks all256 storage slots through FixedArray::callAllFunc. Retail0x8019F58C loads the live count at holder+0x40C and captures the end before callbacks;0x8019F5C0 compares to that cached end. The reference now calls std::for_each with Vector begin/end and its original member functor, preserving virtual dispatch and the captured active range. Full TU compiles; movement73.882355→100%, full text95.12088→100%. Baseline incoming source/object and assembly are retained. Root owns its architecture-appropriate iterator spelling and teardown.

Incoming TVec3 float-pointer conversions make13 historical unary dereferences of `getAirGravityVec()` ambiguous. That method already returns `const TVec3f&`. Remove only these redundant dereferences in four MarioCollision methods, with no conditional or gameplay operand changes. Baseline compiled using the pre-merge TVec header overlay; merged source compiles with incoming header. All affected scores improve: createAtField71.576645→73.5657, calcDistToCeil62.86624→65.458595, calcFrontFloor79.43963→80.36223, getLastSafetyTrans92.90909→94.76363. The checkGround body and all source after it are byte-identical, and its score remains87.42658. See pointer compile/score manifests.

The new float-pointer conversions also expose a preexisting ambiguous `_250 != stick` expression in native MarioWalk. Root deliberately retains its established native Vec pointer surface pending separate retail analysis; this merge does not invent a component comparison or change movement semantics there.

## Validation and exact limits

- **276/276 full RMGK01 Game translation units compile successfully**: every incoming Game source plus every locally changed/recovered Game source since the common base. `broad-compile-results.json` preserves the initial275/276 result; `final-wii-compile-results.json` includes the corrected MarioCollision rerun, command, output exit and final source SHA for each TU. This is original-object evidence, not a full Wii executable link or native runtime result.
- The51 Mario definitions are unique. Compiled object symbol inventory across the cohort finds no duplicate global text functions. Repeated named nerve instance data appears in original header/initializer cohorts; the inventory records it without claiming a full Wii link.
- XanimePlayer.cpp, MarioActor.cpp, MarioActorParts.cpp, MarioAnimator.cpp and MatrixControl.cpp are byte-identical to their verified pre-merge sources. MarioCollision::checkGround is unchanged. Recent jump/grounding/string/base-pose fixes are preserved.
- All conflict markers/unmerged index entries are removed; no unstaged decomp tracked changes remain. Cached diff-check has only two inherited upstream extra blank lines at EOF in OSStateTM.c and scsystem.c; the resolution edits add no whitespace diagnostics.
- No further decomp or root source changes are pending from this merge agent. Native build/runtime and final merge review/commit belong to parent.

The final decomp manifest records all384 staged source hashes and the exact refs. No decomp commit or push has been made by this merge task.
