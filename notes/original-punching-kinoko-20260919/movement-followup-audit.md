# Follow-up MarioMove direction/turning audit

This read-only review follows the separately validated retainMoveDir restoration. The current main chase uses SHA256 `d71704b6e2ed0e6805a00c450e46c30e930c4fecba81d85bdd6becfea765b877`; this audit makes no source changes or claims about its eventual trajectory.

## One additional substantive mismatch

`src/Game/Player/MarioMove.cpp:143` reads `mActor->_1C8` when comparing against `mWallBackHangStickPower`. Current canonical `decomp/src/Game/Player/MarioMove.cpp:134` instead reads `mStickPos.z`. These are different objects/fields, despite sharing the retail offset0x1C8. The actor field is separately initialized to zero in both native and canonical MarioActor sources.

Direct Korean retail DOL evidence confirms the donor: instruction0x802EA01C copies incoming Mario `this` to r28. At0x802EA3D0 the actor pointer is loaded into r3, but0x802EA3D4 is `C03C01C8`, `lfs f1,0x1c8(r28)`. This therefore reads Mario's stick vector Z, not its actor's field. The following comparison against the constant table value conditionally clears Mario+0x3C0. The full original donor mainMove compiles at98.94262% fuzzy similarity, and the complete reference text was independently matched to the DOL with only actual relocation fields masked. See `movement-followup-retail-evidence.json` and the existing MarioMove verification artifacts.

The expected effect is on `_3C0` drop-wait maintenance while `mMovementStates._1` is set. This does not directly reverse movement direction, and there is no claim that it explains the earlier chase trace. The evidence supports a later exact one-expression donor restoration; none is applied in this read-only checkpoint.

## Remaining reviewed direction/turning bodies

The strict CP932-only normalizer produced `MarioMove-current-donor.diff`. Manual control-flow and operation review covered mainMove, isEnableTurn, recordTurnSlipAngle, decideInertia and its Ice/Slip variants, calcShadowDir, calcMoveDir, checkLockOnHoming, doLockOnHoming and fixPositionInTower. The large diff is mostly naming/formatting, early returns versus chained else, direct const/bee-state field access versus identical accessors, JGeometry cross versus PSVECCrossProduct, and explicit vector intermediates.

No second concrete semantic mismatch was established in those reviewed functions beyond the field above and the already restored retainMoveDir. The explicit masks agree with the named native PPC-layout fields: `_10._13`=0x1000, movement `_A`=0x200000 and draw `_5`=0x04000000. The numeric status values equal their canonical enums (Magic0x11, Skate0x1F); isBeeWallWalk returns precisely `mBeeWallWalk != 0`. Camera-ground projection branch signs and output combination in calcMoveDir agree with the donor.

This is source/control-flow review, not an exhaustive proof of floating-point bit identity or a live regression for every movement state. Existing canonical function-level MWCC matches are retained in the evidence JSON (99.44–100% for the reviewed smaller functions). No additional tests or native builds were performed for this read-only follow-up while the parent chase was running.

## Single-expression donor restoration applied

After parent checkpoint `b04188e8f`, the parent authorized replacing only `mActor->_1C8` with canonical `mStickPos.z` in that comparison. The working-tree Game diff is exactly that one expression. Canonical Mario.hpp declares `TVec3f mStickPos` at0x1C0; its third float is0x1C8. No decomp change is required, and no new state fixture or main relink is performed while the existing chase is alive. The next coordinated ordinary build/run will validate execution. Root's current chase reached Guide Goal/Talk1 before this edit, using the earlier binary, so that progression is not attributed to this restoration.

The next coordinated shared build included that expression restoration and passed. The actual original-process trample regression then completed2100frames/exit0 on that same Game archive (`notes/original-mario-actor-sensor-20260919/injected-action-process2.json/log`). This supplies ordinary bounded execution coverage, not a directed test of the drop-wait branch. No separate mainMove state fixture was added.
