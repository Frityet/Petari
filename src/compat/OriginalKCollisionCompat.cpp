#include "Game/Map/KCollision.hpp"
#include "Game/Util/MathUtil.hpp"
#include <algorithm>
#include <aurora/ppc_math.hpp>
#include "resource/KCollisionResource.hpp"

// Original root bodies; raw resource relocation is the native architecture boundary.

Fxyz& Fxyz::operator=(const Fxyz& rOther) {
    x = rOther.x;
    y = rOther.y;
    z = rOther.z;

    return *this;
}

KCollisionServer::KCollisionServer() {
    mFile = nullptr;
    mapInfo = new JMapInfo();
    mMaxVertexDistance = 1.0f;
}

void KCollisionServer::init(void* pData, const void* pMapData) {
    setData(pData);

    if (pMapData != nullptr) {
        mapInfo->attach(pMapData);
    }
}

s32 KCollisionServer::toIndex(const KC_PrismData* pPrism) const {
    return pPrism - (mFile->mPrisms + 1);
}

TVec3f* KCollisionServer::getFaceNormal(const KC_PrismData* pPrism) const {
    return &mFile->mNorms[pPrism->mNormalIndex];
}

TVec3f* KCollisionServer::getEdgeNormal1(const KC_PrismData* pPrism) const {
    return &mFile->mNorms[pPrism->mEdgeIndices[0]];
}

TVec3f* KCollisionServer::getEdgeNormal2(const KC_PrismData* pPrism) const {
    return &mFile->mNorms[pPrism->mEdgeIndices[1]];
}

TVec3f* KCollisionServer::getEdgeNormal3(const KC_PrismData* pPrism) const {
    return &mFile->mNorms[pPrism->mEdgeIndices[2]];
}

TVec3f* KCollisionServer::getNormal(u32 index) const {
    return &mFile->mNorms[index];
}

void KCollisionServer::calXvec(const Fxyz* pVecA, const Fxyz* pVecB, Fxyz* pDst) const {
    pDst->x = pVecA->z * pVecB->y - pVecA->y * pVecB->z;
    pDst->y = pVecA->x * pVecB->z - pVecA->z * pVecB->x;
    pDst->z = pVecA->y * pVecB->x - pVecA->x * pVecB->y;
}

TVec3f KCollisionServer::getPos(const KC_PrismData* pPrism, int vertexIndex) const {
    switch (vertexIndex) {
    case 0: {
        TVec3f* pos = &mFile->mPos[pPrism->mPositionIndex];

        return TVec3f(pos->x, pos->y, pos->z);
    }
    case 1: {
        Fxyz* pos = reinterpret_cast< Fxyz* >(&mFile->mPos[pPrism->mPositionIndex]);
        Fxyz* edge2 = reinterpret_cast< Fxyz* >(&mFile->mNorms[pPrism->mEdgeIndices[2]]);

        Fxyz finalPos;

        calXvec(reinterpret_cast< Fxyz* >(&mFile->mNorms[pPrism->mEdgeIndices[1]]), reinterpret_cast< Fxyz* >(&mFile->mNorms[pPrism->mNormalIndex]),
                &finalPos);

        f32 sideLength = pPrism->mHeight / (finalPos.x * edge2->x + finalPos.y * edge2->y + finalPos.z * edge2->z);

        finalPos.x = pos->x + sideLength * finalPos.x;
        finalPos.y = pos->y + sideLength * finalPos.y;
        finalPos.z = pos->z + sideLength * finalPos.z;

        return TVec3f(finalPos.x, finalPos.y, finalPos.z);
    }
    case 2: {
        Fxyz* pos = reinterpret_cast< Fxyz* >(&mFile->mPos[pPrism->mPositionIndex]);
        Fxyz* edge2 = reinterpret_cast< Fxyz* >(&mFile->mNorms[pPrism->mEdgeIndices[2]]);

        Fxyz finalPos;

        calXvec(reinterpret_cast< Fxyz* >(&mFile->mNorms[pPrism->mNormalIndex]), reinterpret_cast< Fxyz* >(&mFile->mNorms[pPrism->mEdgeIndices[0]]),
                &finalPos);

        f32 sideLength = pPrism->mHeight / (finalPos.x * edge2->x + finalPos.y * edge2->y + finalPos.z * edge2->z);

        finalPos.x = pos->x + sideLength * finalPos.x;
        finalPos.y = pos->y + sideLength * finalPos.y;
        finalPos.z = pos->z + sideLength * finalPos.z;

        return TVec3f(finalPos.x, finalPos.y, finalPos.z);
    }
    default:
        return TVec3f(0.0f, 0.0f, 0.0f);
    }
}

KC_PrismData* KCollisionServer::getPrismData(u32 index) const {
    return &mFile->mPrisms[1 + index];
}

s32 KCollisionServer::getTriangleNum() const {
    return (reinterpret_cast< u8* >(mFile->mOctree) - reinterpret_cast< u8* >(mFile->mPrisms + 1)) / sizeof(KC_PrismData);
}

JMapInfoIter KCollisionServer::getAttributes(u32 index) const {
    KC_PrismData* prism = &mFile->mPrisms[1 + index];

    return JMapInfoIter(mapInfo, prism->mAttribute);
}

s32* KCollisionServer::searchBlock(s32* a1, const u32& rX, const u32& rY, const u32& rZ) const {
    KCLFile* file = mFile;
    s32 blockWidthShift = file->mBlockWidthShift;
    u8* octree = reinterpret_cast< u8* >(file->mOctree);
    *a1 = blockWidthShift;

#if defined(TARGET_PC)
    // The special one-root layout stores -1 shifts. Retail discards that
    // intermediate offset; avoid evaluating negative C++ shifts on the host.
    s32 offset = 0;
    if (file->mBlockXYShift != -1 || file->mBlockXShift != -1) {
        offset = ((rX >> blockWidthShift) | ((rZ >> blockWidthShift) << file->mBlockXYShift) | ((rY >> blockWidthShift) << file->mBlockXShift)) * 4;
    }
#else
    s32 offset = ((rX >> blockWidthShift) | ((rZ >> blockWidthShift) << file->mBlockXYShift) | ((rY >> blockWidthShift) << file->mBlockXShift)) * 4;
    if (file->mBlockXYShift == -1 && file->mBlockXShift == -1) {
        offset = 0;
    }
#endif

    while ((offset = *reinterpret_cast< s32* >(octree + offset)) >= 0) {
        octree += offset;
        s32 uVar7 = --(*a1);

        offset = ((((rZ >> uVar7) & 1) << 2) | (((rY >> uVar7) & 1) << 1) | ((rX >> uVar7) & 1)) * 4;
    }

    return reinterpret_cast< s32* >(octree + (offset & 0x7FFFFFFF));
}

void KCollisionServer::setData(void* data) {
    mFile = smgpc::resource::require_native_kcollision_file(data);
}

bool KCollisionServer::isBinaryInitialized(const void* data) {
    return smgpc::resource::is_native_kcollision_file(data);
}

u32 KCollisionServer::checkArea3D(Fxyz* pMin, Fxyz* pMax, KC_PrismData** pOut, u32 maxCount) {
    s32* skipList = nullptr;
    s32* nextSkipList = nullptr;
    u32 foundCount = 0;

    Fxyz box[2];
    box[0] = *pMin;
    box[1] = *pMax;

    TVec3f queryMin;
    TVec3f queryMax;
    MR::createBoundingBox((TVec3f*)box, 2, &queryMin, &queryMax);

    if (queryMin.x == queryMax.x) {
        queryMin.x -= 1.0f;
        queryMax.x += 1.0f;
    }

    if (queryMin.y == queryMax.y) {
        queryMin.y -= 1.0f;
        queryMax.y += 1.0f;
    }

    if (queryMin.z == queryMax.z) {
        queryMin.z -= 1.0f;
        queryMax.z += 1.0f;
    }

    V3u pointMin;
    V3u pointMax;

    if (!outCheck(&queryMin, &queryMax, &pointMin, &pointMax)) {
        return 0;
    }

    s32 shift;
    u32 z = pointMin.z;

    do {
        u32 y = pointMin.y;
        u32 zStep = 1000000;

        do {
            u32 x = pointMin.x;
            u32 yStep = 1000000;
            u32 bestRemY = 0;

            do {
                s32* list = searchBlock(&shift, x, y, z);
                u32 blockSize = 1U << shift;
                u32 mask = blockSize - 1;
                u32 remX = blockSize - (x & mask);
                u32 remY = blockSize - (y & mask);
                u32 remZ = blockSize - (z & mask);

                if (remZ < zStep) {
                    zStep = remZ;
                }

                if (remY < yStep) {
                    yStep = remY;
                }

                if (remY > bestRemY && ((u16*)list)[1] != 0) {
                    bestRemY = remY;
                    nextSkipList = list;
                }

                if (skipList == nullptr || list != skipList) {
                    u16* prismList = (u16*)list;

                    while (*++prismList != 0) {
                        KC_PrismData* prism = &mFile->mPrisms[*prismList];

                        if (prism->mHeight <= 0.0f) {
                            continue;
                        }

                        KC_PrismData** end = pOut + foundCount;

                        if (std::find(pOut, end, (KC_PrismData* const&)prism) != end) {
                            continue;
                        }

                        TVec3f verts[3];
                        verts[0] = getPos(prism, 0);
                        verts[1] = getPos(prism, 1);
                        verts[2] = getPos(prism, 2);

                        TVec3f prismMin;
                        TVec3f prismMax;
                        MR::createBoundingBox(verts, 3, &prismMin, &prismMax);

                        if (prismMax.x < queryMin.x) {
                            continue;
                        }

                        if (prismMax.y < queryMin.y) {
                            continue;
                        }

                        if (prismMax.z < queryMin.z) {
                            continue;
                        }

                        if (queryMax.x < prismMin.x) {
                            continue;
                        }

                        if (queryMax.y < prismMin.y) {
                            continue;
                        }

                        if (queryMax.z < prismMin.z) {
                            continue;
                        }

                        *end = prism;
                        foundCount++;

                        if (foundCount == maxCount) {
                            return maxCount;
                        }
                    }
                }

                x += remX;
            } while (x <= pointMax.x);

            skipList = nextSkipList;
            y += yStep;
        } while (y <= pointMax.y);

        z += zStep;
    } while (z <= pointMax.z);

    return foundCount;
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
    // Preserve Gekko fctiwz saturation when a finite query exceeds the grid's
    // signed coordinate range; native out-of-range casts are undefined.
    pPoint->x = aurora::ppc::truncate_s32(rPos.x - mFile->mMin.x);
    pPoint->y = aurora::ppc::truncate_s32(rPos.y - mFile->mMin.y);
    pPoint->z = aurora::ppc::truncate_s32(rPos.z - mFile->mMin.z);
}


bool KCollisionServer::isNearParallelNormal(const KC_PrismData* pPrism) const {
    TVec3f edge0 = mFile->mNorms[pPrism->mEdgeIndices[0]];
    TVec3f edge1 = mFile->mNorms[pPrism->mEdgeIndices[1]];
    TVec3f edge2 = mFile->mNorms[pPrism->mEdgeIndices[2]];

    bool isNear = false;

    if (MR::isSameDirection(edge0, edge1) || MR::isSameDirection(edge0, edge2) || MR::isSameDirection(edge1, edge2)) {
        isNear = true;
    }

    return isNear;
}
