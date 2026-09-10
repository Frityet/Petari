# Independent Mario.cpp overlap review

Read-only review of `decomp-three-way/src/Game/Player/Mario.cpp.{ours,upstream}` in this notes directory. Initialization agent owns the actual merge and symbol inventory. No source change, compilation, Git index mutation or commit was performed by this reviewer.

## Retain local recovered behavior

| Function | Concrete upstream regression | Retail evidence |
| --- | --- | --- |
| `inputStick` | Drops the explicit0.01 magnitude dead zone and uses `MR::isNearZero(float)` default0.001. | Mario.s `802AC4CC` loads `@79151`; its sdata2 value is0.01 at `806BF4F4`. Current original MathUtil.hpp declares default0.001. Local recovery note reports97.060610% and matching call/constants. |
| `checkForceGrounding` | Chooses the gravity axis for dot>0 instead of dot>0.99. | `802AA3EC–802AA3F8` loads/compares `@78472`, sdata2 value0.99 at `806BF4C8`. Local recovery note reports97.764046%. |
| `postureCtrl` | Loses `!` on the slip-floor gate, running the adjustment on the opposite class of floor. | `802ADDBC` calls isSlipFloorCode, `802ADDC0–DDC4` negates it, and `802ADDC8` skips the adjustment when the negated result is zero. Local posture recovery reports97.36965%. |
| `fixHeadFrontVecByGravity` | Reverses the nonzero-cross comparison that selects front versus side rotation; separately reverses the later `_60D`/actor `_370` gate. | At `802AA990–802AA9B8`, retail clears the selector when abs(cross·front)<abs(cross·side), as local code does. At `802AAAE4–802AAB0C`, the condition is `_60D || !mActor->_370`, also local; upstream uses the opposite expression. |
| `updateGroundInfo` | Discards checkGround's actual return and copies an unrelated movement bit. | Already independently recovered and validated in `notes/original-mario-ground-result-20260910`; keep local `mMovementStates._1 = checkGround()`. |

All assembly addresses refer to `notes/gateway-audit-20260907/restoration/retail/asm/Game/Player/Mario.s`. Existing historical fuzzy scores are supporting provenance, not newly executed proof for this merge.

## Adopt the incoming physical-writeback correction

`writeBackPhyisicalVector` contains a real upstream improvement. Retail `802ACE20–802ACE28` tests word0x1C bit13 (`_1C._D`). When clear, it projects/clips mJumpVec against the collided normal (`802ACE2C–802ACEB8`). When set, it branches to `802ACEBC` and separately tests the gravity/jump angle before redirecting (`802ACEC0–802ACF20`).

The incoming implementation restores this `if (!_1C._D) ... else if (angle < 6 degrees)` split. Local code omits the bit test and instead attaches the redirect to the scalar projected-component comparison. Upstream's continuation for a nonnegative velocity/normal dot, subsequent upper-punch test, Bee-wall branch, and remaining swimming/wall handling retain the local flow; no additional discrepancy was identified in the paired-body diff. Recommend adopting this function and running the merge owner's normal reference/native checks.

## Remaining jointly edited functions

Keep the local bodies of `update`, `createDirectionMtx`, `createCorrectionMtx`, `createAngleMtx`, and `updateLookOfs` for this merge. Paired-body review found no upstream behavioral improvement. Changes are variable/temporary placement, equivalent Boolean expressions and named bit assignments, direct SDK cross products versus vector-method spelling, and equivalent numeric/operator expressions. Keeping the established recovery preserves its existing operation-order and code-generation evidence.

In particular, createAngleMtx's incoming discarded roll sign calculation does not change the final roll assignment, which is zero on both sides. updateLookOfs uses the same wall/swim/ground/sink/animation conditions and constants, but rewrites explicit vector operations and magnitude evaluation order. Local recorded proof is99.52408% for update and98.354385% for updateLookOfs. No new percentage is claimed for the alternative spellings.

The upstream-only constructor, checkKeyLock and setGroundNorm edits are Boolean/temporary cleanup; no behavior difference was identified. Retain local `false` comparisons in `fixFrontVecByGravity` and `setFrontVecKeepSide`: upstream introduces `== nullptr`/`!= nullptr` on `MR::normalizeOrZero`, whose actual declaration returns bool. That spelling is invalid standard C++ and is not an original gameplay change to preserve.

The snapshot function-name inventory found no newly available Mario function names between the two versions: incoming additions largely occupy the normal source positions of functions already recovered and appended locally. Keep one definition per exact signature, including the separate setFrontVecKeepUp overloads. Use the merge owner's exact51-function disposition inventory for the final duplicate check.
