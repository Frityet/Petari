# PunchingKinoko dependency audit and bounded recovery — 2026-09-19

The existing canonical PunchingKinoko implementation is complete. Its MWCC object has 98.034805% text similarity; the GroundChecker dependency compiles at 100%. The native archive lacks GroundChecker and thirteen non-inline utility APIs. Exact symbol inventory is in api-symbol-audit.json and native-defined-symbols.log.gz.

ShadowVolumeLine recovery published as decomp commit cc77fe564c4da3f8ee35605134e2add83628022a; origin/pcp-decomp remote SHA independently verified.

Native import and factory registration remain withheld. No PunchingKinoko, GroundChecker, shadow, math, sensor, or factory source changes landed in the port. This is a deliberate coherent stopping point after the user's priority changed to explicit source CP932 conversion.

## Dependency closure still required

- Original GroundChecker source/header and original PunchingKinoko source/header.
- Original sensor wrappers: sendMsgEnemyAttackFlipWeak, sendMsgEnemyAttackFlipWeakJump, sendMsgEnemyAttackFlipToDir, sendMsgEnemyAttackFlipMaximumToDir, sendMsgEnemyAttackToBindedSensor, sendMsgToEnemyAttackBlow; directional wrappers also need sendMsgEnemyAttackMsgToDir. Existing original binder sorting/deduplication send helper is present.
- Original matrix helpers scaleMtxToDir, orthogonalize, turnMtxToYDirRate; the latter needs original turnQuatYDirRate. Donors exist.
- Original calcStarPointerWorldVelocityDirectionOnPlane, using actual controller past pointer position/velocity and camera projection. Donor exists.
- initShadowController, addShadowVolumeSphere, addShadowVolumeLine, setShadowDropDirectionPtr need real original controller/drawer ownership. ShadowControllerOwnership already owns original sphere drawers, but VolumeLine currently has no drawer. Do not use a null drawer as successful support.

## ShadowVolumeLine recovery

The line drawer donor lacked drawShape. Recovered it in decomp/src/Game/LiveActor/ShadowVolumeLine.cpp from the complete retail assembly. It builds the eight endpoints using the two controllers' drop positions, directions, widths and drop lengths; rejects degenerate line/cross products; emits two four-vertex quads and a ten-vertex triangle strip in the original winding/order. No invented defaults or actor-specific geometry.

MWCC/objdiff: drawShape 97.91011%; full 1012-byte text 98.114624%; data, constants and vtable 100%. verify-shadow-line.py independently compares every reference text instruction with main.dol range 0x8016ef38–0x8016f32c, masking only 62 documented ELF relocation bitfields; PASS. Retail DOL SHA1 25c5959534b3c21246c6c7e42021b916b41fb578. Existing destructor register differences are inherited.

No native ShadowVolumeLine import was made, and this evidence does not establish native drawing or PunchingKinoko gameplay.
