# Shared provider recovery, 2026-09-07

Recovered MR::calcSpherePos (16 bytes) and getSphereRadius (12 bytes) in the
regular AreaObjUtil reference. Both compile with the original MW compiler and
match retail RMGK01 at 100%. The former reads the actual AreaFormSphere and
calls calcPos; the latter reads its radius member at 0x14. Typed declarations
were added to AreaObjUtil.hpp.

ModelUtil::getTexture's old JUTTexture* return/cast was corrected to ResTIMG*:
the actual ResourceHolder file table contains raw BTI bytes, and the recovered
MarioFoo::init consumes this pointer in JUTTexture(ResTIMG*). The 8-byte original
tail-call remains 100%; the missing shared header declaration now has the same
type as that original caller.

Full Wii compiler commands, symbol/disassembly proof and native import/test
results are in the embedding port's same-named notes directory. The compact
recovery-summary.json records the exact reference paths and current symbol
scores. Existing completed DirectDraw/Mtx/LiveActor routines were freshly
verified for import; their reference bodies were not changed.
