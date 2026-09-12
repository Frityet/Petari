# Upstream merge: player core conflict audit

Merge inputs: ours `ad4f2b113`, upstream/master `cae223c32`, common base `d1ae0a05c`. Reference submodule branch `pcp-decomp`. This directory preserves the available three index stages before resolution. No native Game sources changed in this merge task. Parent owns aggregate original-compiler validation and publication.

## Resolved and staged by compatibility audit agent

| File | Resolution |
| --- | --- |
| Mario.hpp | Use upstream's typed `CubeCameraArea* _568`; preserve existing typed `HitSensor* _A38`. Keep unchanged movement/draw bit order. Accept merged const-correct safety-position return, void probe return types, and two-component `_6C8` representation. |
| MarioAnimator.hpp | Use semantic callback fields `mCallbackEnded`, `mCallbackId`, `mCallbackTable`, matching upstream callback source. |
| Mario.cpp | Use upstream source ordering and add original `XanimeCore::getJointTransform` and `MarioState::draw3D`. Preserve eleven already-proven original bodies: checkForceGrounding, inputStick, createDirectionMtx, createCorrectionMtx, createAngleMtx, fixHeadFrontVecByGravity, postureCtrl, updateLookOfs, updateGroundInfo, update, fixFrontVecByGravity. This preserves the restored ground-result assignment and unordered floating-point guards. Drop old FORCE_OPERATOR compilation-only helper. Normalize status spelling to MarioStatus_Recovery. |
| MarioAnimationEfx.cpp | Accept upstream semantic callback fields and reviewed switch flow. Retail confirms upstream frame >=40 stop condition. Preserve ours' required `luigiAnimeSwapTable` data and direct XanimeResource include; upstream omits this table but MarioAnimator.cpp still refers to it. |
| MarioBee.cpp | Accept reviewed upstream equivalent control flow, typed player modes/const access, and correct bee-spin animation spelling. |
| MarioCollision.cpp | Accept upstream original source cohort, including corrected probe constants, camera direction, return types, and direct SDK operations. Preserve previously validated checkGround body and its jumping/grounded snap gate. Expand the four local triangleNormal helper calls to exact `*triangle->getNormal(0)` SDK expressions, avoiding a redundant helper dependency. |
| MarioFlip.cpp | Resolve reviewed equivalent conflict blocks with upstream forms; retain recovered bodies and surrounding merge. |
| MarioModule.cpp | Use typed null casts and SDK TVec3::scaleAdd instead of duplicate helper call. SDK definition was inspected: destination += source * scale, same original operation. |
| MarioMove.cpp | Accept upstream source with retail-confirmed retention factor, draw flag, animation string, and homing-assignment gate. |
| MarioSpecial.cpp | Accept reviewed upstream typed source/control flow. Preserve ours' isHeadPushEnableArea `_37` movement flag because upstream `_17` is a retail-inconsistent regression. Retain descriptive names and f32 expression order in isNotReflectGlassGround. |
| MarioWait.cpp | Accept upstream source and four newly supplied original MarioState default bodies. Retail confirms the two wait animation strings which ours had interchanged. Preserve initializer-list constructor with original field initializations. |
| MarioWarp.cpp | Preserve reviewed ours conflict bodies, named player modes, explicit numeric conversions, and required scope around the case-local Mario pointer. Integrate typed WarpCube pointer and const table access from upstream. All ten recovered methods retained. |

Root independently resolved/staged MarioSlope.cpp and MarioSpin.cpp after ownership transfer. No shared build or commit was performed by this agent.

## Retail evidence for substantive choices

Assembly paths below are under `notes/gateway-audit-20260907/restoration/retail/asm/Game/Player/`.

* `MarioFaint.s` doFlipWeak, `802D97A4` loads movement word 8; `802D97A8` extracts bit 27. `_1B` upstream is correct, `_1C` ours was incorrect. Sent to state-family owner, which owns that file. Earlier 99.87805% function similarity did not make this one-bit difference harmless.
* `MarioCollision.s` isIgnoreTriangle `802CF218` loads @72901, `.float 0.001`; ours used 0.01. checkBaseTransBall lower probe uses @73033, `.float 40`; ours shared a 50 offset for upper and lower probes. Original camera code at `802D4EEC` loads 100 and `802D4F0C` subtracts the gravity displacement: upstream position - gravity *100 is correct; ours added it.
* `MarioAnimationEfx.s` squatSpinCheck `802CE15C` compares frame with 40, then `802CE160` combines greater/equal. The stop condition is >=40, matching upstream; ours <=40 was wrong.
* `MarioMove.s` retainMoveDir `802EC284` loads @62697 (`.float 0.06`), where ours used 0.99. `802EC2D0` ORs 0x200 into draw word 0x18: bit22 (`_16`), not ours `_9`. Label `lbl_805C8CE9` decodes to `その場足踏み`. doLockOnHoming `802EC91C` skips both direction update and `_334` assignment when `_750` is nonzero; upstream correctly puts both under that guard.
* `MarioWait.s` start at `802CEB00` uses lbl_805C472A, bytes decode to `戦闘ウエイト`; update at `802CECE4` uses lbl_805C4737, bytes decode to `特殊ウエイト1B`. Both labels are stored in the adjacent MarioAnimationEfx.s retail data. Upstream ordering is correct.
* `MarioSpecial.s` isHeadPushEnableArea `802F3CC8` loads movement word **0xC**, the second word; `802F3CD4` extracts bit23. This is overall movement bit55 (`_37`), so ours is correct and upstream `_17` is rejected.

## Validation

`coverage.json` compares qualified method definitions from both saved sides against the ten merged source files. All 249 definitions from the union remain, including six upstream additions. No unexpected duplicate qualified definitions were introduced. This source inventory does not prove behavior; the retail observations above establish the listed substantive choices.

Parent's first 117-target original-compiler aggregate reported 116 targets passed/current and one retained checkGround helper dependency. That dependency was replaced with its exact original SDK expansion and the twelve files staged. Parent owns the final repeat receipt; do not treat source resolution alone as gameplay validation.
