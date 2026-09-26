#include "Game/Map/KCollision.hpp"
#include <aurora/ppc_math.hpp>

bool KCollisionServer::isInsideMinMaxInLocalSpace(const V3u& rPoint) const {
    return (rPoint.x & mFile->mXMask) == 0 && (rPoint.y & mFile->mYMask) == 0 && (rPoint.z & mFile->mZMask) == 0;
}

bool KCollisionServer::outCheck(const TVec3f* pPosA, const TVec3f* pPosB, V3u* pPointA, V3u* pPointB) const {
    objectSpaceToLocalSpace(pPointA, *pPosA);
    objectSpaceToLocalSpace(pPointB, *pPosB);

    if (pPointA->x < 0) {
        pPointA->x = 0;
    }

    if (pPointA->y < 0) {
        pPointA->y = 0;
    }

    if (pPointA->z < 0) {
        pPointA->z = 0;
    }

    s32 invertedXMask = ~mFile->mXMask;

    if (invertedXMask < pPointB->x) {
        pPointB->x = invertedXMask;
    }

    s32 invertedYMask = ~mFile->mYMask;

    if (invertedYMask < pPointB->y) {
        pPointB->y = invertedYMask;
    }

    s32 invertedZMask = ~mFile->mZMask;

    if (invertedZMask < pPointB->z) {
        pPointB->z = invertedZMask;
    }

    if (pPointB->x < pPointA->x || pPointB->y < pPointA->y || pPointB->z < pPointA->z) {
        return false;
    }

    return true;
}

void KCollisionServer::objectSpaceToLocalSpace(V3u* pPoint, const TVec3f& rPos) const {
    // Gekko fctiwz saturates; an out-of-range native float-to-int cast is undefined.
    pPoint->x = aurora::ppc::truncate_s32(rPos.x - mFile->mMin.x);
    pPoint->y = aurora::ppc::truncate_s32(rPos.y - mFile->mMin.y);
    pPoint->z = aurora::ppc::truncate_s32(rPos.z - mFile->mMin.z);
}
