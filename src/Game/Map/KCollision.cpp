#include "Game/Map/KCollision.hpp"
#include "Game/Camera/CameraPolygonCodeUtil.hpp"
#include "Game/Map/CollisionCode.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/MathUtil.hpp"
#include <algorithm>

void DUMMY_KColloision() {
    TVec3f a, b, c;

    if (a == b) {
        c = a + b;
    }
}

void DUMMY_KColloision2() {
    TVec3f a, b, c;

    if (a == b) {
        c = a + b;
    }
}

Fxyz& Fxyz::operator=(const Fxyz& rOther) {
    x = rOther.x;
    y = rOther.y;
    z = rOther.z;

    return *this;
}

void KCollisionServer::V3u::setUsingCast(const TVec3f& rPos) {
    x = static_cast< s32 >(rPos.x);
    y = static_cast< s32 >(rPos.y);
    z = static_cast< s32 >(rPos.z);
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

void KCollisionServer::setData(void* pData) {
    mFile = reinterpret_cast< KCLFile* >(pData);

    if (!isBinaryInitialized(pData)) {
        mFile->mPos = reinterpret_cast< TVec3f* >(reinterpret_cast< u8* >(mFile) + mFile->mPosOffset);
        mFile->mNorms = reinterpret_cast< TVec3f* >(reinterpret_cast< u8* >(mFile) + mFile->mNormOffset);
        mFile->mPrisms = reinterpret_cast< KC_PrismData* >(reinterpret_cast< u8* >(mFile) + mFile->mPrismOffset);
        mFile->mOctree = reinterpret_cast< void* >(reinterpret_cast< u8* >(mFile) + mFile->mOctreeOffset);
    }
}

bool KCollisionServer::calcFarthestVertexDistance() {
    u32 triCount = getTriangleNum();
    f32 maxDistance = 0.0f;
    bool isValid = true;

    for (u32 i = 0; i < triCount; i++) {
        KC_PrismData* prism = getPrismData(i);
        JMapInfoIter iter = getAttributes(i);

        if (!iter.isValid()) {
            isValid = false;
        } else {
            MR::registerCameraCode(MR::getCollisionDirector()->mCode->getCameraID(iter));
        }

        if (isNearParallelNormal(prism)) {
            prism->mHeight = -MR::abs(prism->mHeight);
        } else {
            for (s32 j = 0; j < 3; j++) {
                TVec3f pos = getPos(prism, j);
                f32 distance = pos.squared();

                if (maxDistance < distance) {
                    maxDistance = distance;
                }
            }
        }
    }

    mMaxVertexDistance = MR::sqrt(maxDistance);
    return isValid;
}

bool KCollisionServer::isBinaryInitialized(const void* pData) {
    return reinterpret_cast< const s32* >(pData)[0] < 0;
}

KC_PrismData* KCollisionServer::checkPoint(Fxyz* pPos, f32 scale, f32* pDistance) {
    f32 thickness = mFile->mThickness * scale;
    u32 x = static_cast< s32 >(pPos->x - mFile->mMin.x);
    if ((x & mFile->mXMask) != 0) {
        return nullptr;
    }
    u32 y = static_cast< s32 >(pPos->y - mFile->mMin.y);
    if ((y & mFile->mYMask) != 0) {
        return nullptr;
    }
    u32 z = static_cast< s32 >(pPos->z - mFile->mMin.z);
    if ((z & mFile->mZMask) != 0) {
        return nullptr;
    }

    s32 shift;
    u16* pList = reinterpret_cast< u16* >(searchBlock(&shift, x, y, z));
    while (*++pList != 0) {
        KC_PrismData* pPrism = &mFile->mPrisms[*pList];
        if (pPrism->mHeight <= 0.0f) {
            continue;
        }
        const TVec3f* pOrigin = &mFile->mPos[pPrism->mPositionIndex];
        Fxyz offset;
        offset.x = pPos->x - pOrigin->x;
        offset.y = pPos->y - pOrigin->y;
        offset.z = pPos->z - pOrigin->z;
        const TVec3f* pNormal = &mFile->mNorms[pPrism->mEdgeIndices[0]];
        if (offset.x * pNormal->x + offset.y * pNormal->y + offset.z * pNormal->z > 0.0f) {
            continue;
        }
        pNormal = &mFile->mNorms[pPrism->mEdgeIndices[1]];
        if (offset.x * pNormal->x + offset.y * pNormal->y + offset.z * pNormal->z > 0.0f) {
            continue;
        }
        pNormal = &mFile->mNorms[pPrism->mEdgeIndices[2]];
        if (offset.x * pNormal->x + offset.y * pNormal->y + offset.z * pNormal->z > pPrism->mHeight) {
            continue;
        }
        pNormal = &mFile->mNorms[pPrism->mNormalIndex];
        f32 distance = -offset.x * pNormal->x - offset.y * pNormal->y - offset.z * pNormal->z;
        if (distance < 0.0f || thickness < distance) {
            continue;
        }
        *pDistance = distance;
        return pPrism;
    }
    return nullptr;
}

u32 KCollisionServer::checkArea3D(Fxyz* pPointA, Fxyz* pPointB, KC_PrismData** pPrisms, u32 capacity) {
    u16* pLongestList = nullptr;
    u16* pPreviousList = nullptr;
    u32 count = 0;
    Fxyz points[2];
    points[0] = *pPointA;
    points[1] = *pPointB;
    TVec3f minimum;
    TVec3f maximum;
    MR::createBoundingBox(reinterpret_cast< TVec3f* >(points), 2, &minimum, &maximum);
    if (minimum.x == maximum.x) {
        minimum.x -= 1.0f;
        maximum.x += 1.0f;
    }
    if (minimum.y == maximum.y) {
        minimum.y -= 1.0f;
        maximum.y += 1.0f;
    }
    if (minimum.z == maximum.z) {
        minimum.z -= 1.0f;
        maximum.z += 1.0f;
    }
    V3u localMinimum;
    V3u localMaximum;
    if (!outCheck(&minimum, &maximum, &localMinimum, &localMaximum)) {
        return 0;
    }

    u32 z = localMinimum.z;

    do {
        u32 y = localMinimum.y;
        s32 stepZ = 1000000;

        do {
            u32 x = localMinimum.x;
            s32 stepY = 1000000;
            s32 longestY = 0;

            do {
                s32 shift;
                u16* pList = reinterpret_cast< u16* >(searchBlock(&shift, x, y, z));
                shift = 1 << shift;
                s32 width = shift;
                s32 mask = width - 1;
                s32 remainingZ = width - (z & mask);
                s32 stepX = width - (x & mask);
                s32 remainingY = width - (y & mask);

                if (remainingZ < stepZ) {
                    stepZ = remainingZ;
                }

                if (remainingY < stepY) {
                    stepY = remainingY;
                }

                if (remainingY > longestY && pList[1] != 0) {
                    longestY = remainingY;
                    pLongestList = pList;
                }

                if (pPreviousList == nullptr || pList != pPreviousList) {
                    while (*++pList != 0) {
                        KC_PrismData* pPrism = &mFile->mPrisms[*pList];

                        if (pPrism->mHeight <= 0.0f) {
                            continue;
                        }

                        KC_PrismData** pEnd = pPrisms + count;
                        if (std::find(pPrisms, pEnd, static_cast< KC_PrismData* const& >(pPrism)) != pEnd) {
                            continue;
                        }

                        TVec3f points[3];
                        points[0] = getPos(pPrism, 0);
                        points[1] = getPos(pPrism, 1);
                        points[2] = getPos(pPrism, 2);
                        TVec3f prismMinimum;
                        TVec3f prismMaximum;
                        MR::createBoundingBox(points, 3, &prismMinimum, &prismMaximum);
                        if (prismMaximum.x < minimum.x || prismMaximum.y < minimum.y || prismMaximum.z < minimum.z ||
                            maximum.x < prismMinimum.x || maximum.y < prismMinimum.y || maximum.z < prismMinimum.z) {
                            continue;
                        }
                        *pEnd = pPrism;
                        count++;
                        if (count == capacity) {
                            return capacity;
                        }
                    }
                }

                x += stepX;
            } while (x <= static_cast< u32 >(localMaximum.x));

            pPreviousList = pLongestList;
            y += stepY;
        } while (y <= static_cast< u32 >(localMaximum.y));

        z += stepZ;
    } while (z <= static_cast< u32 >(localMaximum.z));

    return count;
}

u32 KCollisionServer::checkSphere(Fxyz* pPos, f32 radius, f32 scale, u32 capacity, KC_PrismData** pPrisms, f32* pDistances, u8* pFeatures) {
    f32 distance = 0.0f;
    u16* pLongestList = nullptr;
    u16* pPreviousList = nullptr;
    u32 count = 0;
    TVec3f maximum;
    maximum.x = pPos->x + radius;
    maximum.y = pPos->y + radius;
    maximum.z = pPos->z + radius;
    TVec3f minimum;
    minimum.x = pPos->x - radius;
    minimum.y = pPos->y - radius;
    minimum.z = pPos->z - radius;
    V3u localMinimum;
    V3u localMaximum;

    if (!outCheck(&minimum, &maximum, &localMinimum, &localMaximum)) {
        return 0;
    }

    u32 z = localMinimum.z;

    do {
        u32 y = localMinimum.y;
        s32 stepZ = 1000000;

        do {
            u32 x = localMinimum.x;
            s32 stepY = 1000000;
            s32 longestY = 0;

            do {
                s32 shift;
                u16* pList = reinterpret_cast< u16* >(searchBlock(&shift, x, y, z));
                s32 width = 1 << shift;
                s32 mask = width - 1;
                s32 remainingZ = width - (z & mask);
                s32 stepX = width - (x & mask);
                s32 remainingY = width - (y & mask);

                if (remainingZ < stepZ) {
                    stepZ = remainingZ;
                }

                if (remainingY < stepY) {
                    stepY = remainingY;
                }

                if (remainingY > longestY && pList[1] != 0) {
                    longestY = remainingY;
                    pLongestList = pList;
                }

                if (pPreviousList == nullptr || pList != pPreviousList) {
                    while (*++pList != 0) {
                        KC_PrismData* pPrism = &mFile->mPrisms[*pList];

                        if (pPrism->mHeight <= 0.0f) {
                            continue;
                        }

                        if (std::find(pPrisms, pPrisms + count, static_cast< KC_PrismData* const& >(pPrism)) != pPrisms + count) {
                            continue;
                        }

                        u8 feature;

                        if (KCHitSphere(pPrism, pPos, radius, scale, &distance, &feature) && count < capacity &&
                            std::find(pPrisms, pPrisms + count, static_cast< KC_PrismData* const& >(pPrism)) == pPrisms + count) {
                            pPrisms[count] = pPrism;
                            pDistances[count] = distance;
                            pFeatures[count] = feature;
                            count++;
                        }
                    }
                }

                x += stepX;
            } while (x <= static_cast< u32 >(localMaximum.x));

            pPreviousList = pLongestList;
            y += stepY;
        } while (y <= static_cast< u32 >(localMaximum.y));

        z += stepZ;
    } while (z <= static_cast< u32 >(localMaximum.z));

    return count;
}

u32 KCollisionServer::checkSphereWithThickness(Fxyz* pPos, f32 radius, f32 scale, u32 capacity, KC_PrismData** pPrisms, f32* pDistances,
                                               u8* pFeatures, f32 thickness) {
    f32 distance = 0.0f;
    u16* pLongestList = nullptr;
    u16* pPreviousList = nullptr;
    u32 count = 0;
    TVec3f maximum;
    maximum.x = pPos->x + radius;
    maximum.y = pPos->y + radius;
    maximum.z = pPos->z + radius;
    TVec3f minimum;
    minimum.x = pPos->x - radius;
    minimum.y = pPos->y - radius;
    minimum.z = pPos->z - radius;
    V3u localMinimum;
    V3u localMaximum;

    if (!outCheck(&minimum, &maximum, &localMinimum, &localMaximum)) {
        return 0;
    }

    u32 z = localMinimum.z;

    do {
        u32 y = localMinimum.y;
        s32 stepZ = 1000000;

        do {
            u32 x = localMinimum.x;
            s32 stepY = 1000000;
            s32 longestY = 0;

            do {
                s32 shift;
                u16* pList = reinterpret_cast< u16* >(searchBlock(&shift, x, y, z));
                s32 width = 1 << shift;
                s32 mask = width - 1;
                s32 remainingZ = width - (z & mask);
                s32 stepX = width - (x & mask);
                s32 remainingY = width - (y & mask);

                if (remainingZ < stepZ) {
                    stepZ = remainingZ;
                }

                if (remainingY < stepY) {
                    stepY = remainingY;
                }

                if (remainingY > longestY && pList[1] != 0) {
                    longestY = remainingY;
                    pLongestList = pList;
                }

                if (pPreviousList == nullptr || pList != pPreviousList) {
                    while (*++pList != 0) {
                        KC_PrismData* pPrism = &mFile->mPrisms[*pList];

                        if (pPrism->mHeight <= 0.0f) {
                            continue;
                        }

                        if (std::find(pPrisms, pPrisms + count, static_cast< KC_PrismData* const& >(pPrism)) != pPrisms + count) {
                            continue;
                        }

                        u8 feature;

                        if (KCHitSphereWithThickness(pPrism, pPos, radius, scale, &distance, &feature, thickness) && count < capacity &&
                            std::find(pPrisms, pPrisms + count, static_cast< KC_PrismData* const& >(pPrism)) == pPrisms + count) {
                            pPrisms[count] = pPrism;
                            pDistances[count] = distance;
                            pFeatures[count] = feature;
                            count++;
                        }
                    }
                }

                x += stepX;
            } while (x <= static_cast< u32 >(localMaximum.x));

            pPreviousList = pLongestList;
            y += stepY;
        } while (y <= static_cast< u32 >(localMaximum.y));

        z += stepZ;
    } while (z <= static_cast< u32 >(localMaximum.z));

    return count;
}

KC_PrismData* KCollisionServer::checkArrow(const TVec3f& rPos, const TVec3f& rOffset, f32* pDistances, u8* pFeatures, u32* pCount,
                                           KC_PrismData** pPrisms, u32 capacity) const {
    if (rOffset.x == 0.0f && rOffset.y == 0.0f && rOffset.z == 0.0f) {
        return nullptr;
    }

    TVec3f direction(rOffset);
    f32 travelled = 0.0f;
    f32 length;
    MR::separateScalarAndDirection(&length, &direction, direction);

    if (MR::isNearZero(direction)) {
        return nullptr;
    }

    TVec3f cursor;
    V3u cell;
    TVec3f localStart(rPos);
    localStart.x -= mFile->mMin.x;
    localStart.y -= mFile->mMin.y;
    localStart.z -= mFile->mMin.z;
    cell.setUsingCast(localStart);

    if (isInsideMinMaxInLocalSpace(cell)) {
        cursor.set(localStart);
    } else {
        TVec3f maximum;
        maximum.x = static_cast< u32 >(~mFile->mXMask);
        maximum.y = static_cast< u32 >(~mFile->mYMask);
        maximum.z = static_cast< u32 >(~mFile->mZMask);

        if (direction.x != 0.0f) {
            f32 boundary = 0.0f < direction.x ? 0.0f : maximum.x;
            travelled = (boundary - localStart.x) / direction.x;

            if (0.0f <= travelled && travelled <= length) {
                TVec3f offset(direction);
                offset.scale(travelled);
                cursor.set(offset);
                cursor += localStart;
                cell.setUsingCast(cursor);

                if (isInsideMinMaxInLocalSpace(cell)) {
                    goto beginTraversal;
                }
            }
        }

        if (direction.y != 0.0f) {
            f32 boundary = 0.0f < direction.y ? 0.0f : maximum.y;
            travelled = (boundary - localStart.y) / direction.y;

            if (0.0f <= travelled && travelled <= length) {
                TVec3f offset(direction);
                offset.scale(travelled);
                cursor.set(offset);
                cursor += localStart;
                cell.setUsingCast(cursor);

                if (isInsideMinMaxInLocalSpace(cell)) {
                    goto beginTraversal;
                }
            }
        }

        if (direction.z != 0.0f) {
            f32 boundary = 0.0f < direction.z ? 0.0f : maximum.z;
            travelled = (boundary - localStart.z) / direction.z;

            if (0.0f <= travelled && travelled <= length) {
                TVec3f offset(direction);
                offset.scale(travelled);
                cursor.set(offset);
                cursor += localStart;
                cell.setUsingCast(cursor);

                if (isInsideMinMaxInLocalSpace(cell)) {
                    goto beginTraversal;
                }
            }
        }

        return nullptr;
    }

beginTraversal:
    u32 count = 0;
    s32 positiveX;
    s32 positiveY;
    s32 positiveZ;
    s32 negativeX;
    s32 negativeY;
    s32 negativeZ;
    s32* pStepX;
    s32* pStepY;
    s32* pStepZ;
    s32 signX;
    s32 signY;
    s32 signZ;

    if (direction.x < 0.0f) {
        pStepX = &negativeX;
        signX = -1;
    } else {
        pStepX = &positiveX;
        signX = 1;
    }

    if (direction.y < 0.0f) {
        pStepY = &negativeY;
        signY = -1;
    } else {
        pStepY = &positiveY;
        signY = 1;
    }

    if (direction.z < 0.0f) {
        pStepZ = &negativeZ;
        signZ = -1;
    } else {
        pStepZ = &positiveZ;
        signZ = 1;
    }

    KC_PrismData* pNearest = nullptr;

    do {
        s32 shift;
        u16* pList = reinterpret_cast< u16* >(searchBlock(&shift, reinterpret_cast< const u32& >(cell.x), reinterpret_cast< const u32& >(cell.y),
                                                          reinterpret_cast< const u32& >(cell.z)));
        s32 width = 1 << shift;
        s32 mask = width - 1;
        positiveX = width - (cell.x & mask);
        positiveY = width - (cell.y & mask);
        positiveZ = width - (cell.z & mask);
        negativeX = -(cell.x & mask);
        negativeY = -(cell.y & mask);
        negativeZ = -(cell.z & mask);

        if (*pStepX == 0) {
            *pStepX = signX;
        }

        if (*pStepY == 0) {
            *pStepY = signY;
        }

        if (*pStepZ == 0) {
            *pStepZ = signZ;
        }

        f32 nearestDistance = 1.0f;
        f32 distance = 1.0f;

        while (*++pList != 0) {
            KC_PrismData* pPrism = &mFile->mPrisms[*pList];

            if (pPrism->mHeight <= 0.0f) {
                continue;
            }

            u8 feature = 0;

            if (KCHitArrow(pPrism, rPos, rOffset, &distance, &feature)) {
                if (pPrisms != nullptr) {
                    pDistances[count] = distance;
                    pPrisms[count] = pPrism;
                    count++;

                    if (distance < nearestDistance) {
                        nearestDistance = distance;
                        pNearest = pPrism;
                    }

                    if (count == capacity) {
                        if (pCount != nullptr) {
                            *pCount = count;
                        }

                        return pNearest;
                    }
                } else if (distance < nearestDistance) {
                    nearestDistance = *pDistances = distance;
                    pNearest = pPrism;
                    *pFeatures = feature;
                }
            }
        }

        if (pPrisms == nullptr && pNearest != nullptr) {
            break;
        }

        f32 stepX = MR::isNearZero(direction.x) ? 1000000000.0f : *pStepX / direction.x;
        f32 stepY = MR::isNearZero(direction.y) ? 1000000000.0f : *pStepY / direction.y;
        f32 stepZ = MR::isNearZero(direction.z) ? 1000000000.0f : *pStepZ / direction.z;

        if (stepY < stepX) {
            stepX = stepY;
        }

        if (stepZ < stepX) {
            stepX = stepZ;
        }

        if (length - travelled <= stepX) {
            break;
        }

        TVec3f offset(direction);
        offset.scale(stepX);
        cursor += offset;
        travelled += stepX;
        cell.setUsingCast(cursor);

        if (!isInsideMinMaxInLocalSpace(cell)) {
            break;
        }
    } while (travelled < length);

    if (pCount != nullptr) {
        *pCount = count;
    }

    return pNearest;
}

bool KCollisionServer::KCHitSphere(KC_PrismData* pPrism, Fxyz* pPos, f32 radius, f32 scale, f32* pDistance, u8* pFeature) {
    f32 thickness = mFile->mThickness * scale;
    f32 radiusSquared = radius * radius;
    *pFeature = 0;
    Fxyz relative;
    f32 distances[4];
    f32 dot01;
    f32 dot12;
    f32 dot20;
    TVec3f* pOrigin = &mFile->mPos[pPrism->mPositionIndex];
    relative.x = pPos->x - pOrigin->x;
    relative.y = pPos->y - pOrigin->y;
    relative.z = pPos->z - pOrigin->z;
    TVec3f* pEdge0 = &mFile->mNorms[pPrism->mEdgeIndices[0]];
    distances[1] = relative.x * pEdge0->x + relative.y * pEdge0->y + relative.z * pEdge0->z;

    if (distances[1] >= radius) {
        return false;
    }

    TVec3f* pEdge1 = &mFile->mNorms[pPrism->mEdgeIndices[1]];
    distances[2] = relative.x * pEdge1->x + relative.y * pEdge1->y + relative.z * pEdge1->z;

    if (distances[2] >= radius) {
        return false;
    }

    TVec3f* pEdge2 = &mFile->mNorms[pPrism->mEdgeIndices[2]];
    distances[3] = relative.x * pEdge2->x + relative.y * pEdge2->y + relative.z * pEdge2->z - pPrism->mHeight;

    if (distances[3] >= radius) {
        return false;
    }

    TVec3f* pNormal = &mFile->mNorms[pPrism->mNormalIndex];
    distances[0] = relative.x * pNormal->x + relative.y * pNormal->y + relative.z * pNormal->z;
    *pDistance = radius - distances[0];

    if (*pDistance < 0.0f) {
        return false;
    }

    if (distances[1] > distances[2]) {
        if (!(distances[1] > distances[3])) {
            goto edge2Region;
        }
    } else {
        if (!(distances[2] > distances[3])) {
            goto edge2Region;
        }

        goto edge1Region;
    }

edge0Region:
    if (distances[1] <= 0.0f) {
        if (thickness < *pDistance) {
            return false;
        }

        *pFeature = 1;
        goto accepted;
    }

    if (distances[2] > distances[3]) {
        dot01 = pEdge0->x * pEdge1->x + pEdge0->y * pEdge1->y + pEdge0->z * pEdge1->z;

        if (!(dot01 * distances[1] > distances[2])) {
            goto vertex0;
        }

        goto edge0;
    } else {
        dot20 = pEdge0->x * pEdge2->x + pEdge0->y * pEdge2->y + pEdge0->z * pEdge2->z;

        if (!(dot20 * distances[1] > distances[3])) {
            goto vertex2;
        }

        goto edge0;
    }

edge1Region:
    if (distances[2] <= 0.0f) {
        if (thickness < *pDistance) {
            return false;
        }

        *pFeature = 1;
        goto accepted;
    }

    if (distances[3] > distances[1]) {
        dot12 = pEdge1->x * pEdge2->x + pEdge1->y * pEdge2->y + pEdge1->z * pEdge2->z;

        if (!(dot12 * distances[2] > distances[3])) {
            goto vertex1;
        }

        goto edge1;
    } else {
        dot01 = pEdge1->x * pEdge0->x + pEdge1->y * pEdge0->y + pEdge1->z * pEdge0->z;

        if (!(dot01 * distances[2] > distances[1])) {
            goto vertex0;
        }

        goto edge1;
    }

edge2Region:
    if (distances[3] <= 0.0f) {
        if (thickness < *pDistance) {
            return false;
        }

        *pFeature = 1;
        goto accepted;
    }

    if (distances[1] > distances[2]) {
        dot20 = pEdge2->x * pEdge0->x + pEdge2->y * pEdge0->y + pEdge2->z * pEdge0->z;

        if (!(dot20 * distances[3] > distances[1])) {
            goto vertex2;
        }

        goto edge2;
    } else {
        dot12 = pEdge2->x * pEdge1->x + pEdge2->y * pEdge1->y + pEdge2->z * pEdge1->z;

        if (!(dot12 * distances[3] > distances[2])) {
            goto vertex1;
        }

        goto edge2;
    }

edge0:
    if (distances[1] > distances[0]) {
        return false;
    }

    *pDistance = radiusSquared - distances[1] * distances[1];
    *pFeature = 2;
    goto calcDistance;

edge1:
    if (distances[2] > distances[0]) {
        return false;
    }

    *pDistance = radiusSquared - distances[2] * distances[2];
    *pFeature = 3;
    goto calcDistance;

edge2:
    if (distances[3] > distances[0]) {
        return false;
    }

    *pDistance = radiusSquared - distances[3] * distances[3];
    *pFeature = 4;
    goto calcDistance;

vertex0: {
    f32 weight0 = (dot01 * distances[2] - distances[1]) / (dot01 * dot01 - 1.0f);
    f32 weight1 = distances[2] - weight0 * dot01;
    relative.x = weight0 * pEdge0->x + weight1 * pEdge1->x;
    relative.y = weight0 * pEdge0->y + weight1 * pEdge1->y;
    relative.z = weight0 * pEdge0->z + weight1 * pEdge1->z;
    *pFeature = 5;
    goto checkVertex;
}

vertex1: {
    f32 weight1 = (dot12 * distances[3] - distances[2]) / (dot12 * dot12 - 1.0f);
    f32 weight2 = distances[3] - weight1 * dot12;
    relative.x = weight1 * pEdge1->x + weight2 * pEdge2->x;
    relative.y = weight1 * pEdge1->y + weight2 * pEdge2->y;
    relative.z = weight1 * pEdge1->z + weight2 * pEdge2->z;
    *pFeature = 6;
    goto checkVertex;
}

vertex2: {
    f32 weight2 = (dot20 * distances[1] - distances[3]) / (dot20 * dot20 - 1.0f);
    f32 weight0 = distances[1] - weight2 * dot20;
    relative.x = weight2 * pEdge2->x + weight0 * pEdge0->x;
    relative.y = weight2 * pEdge2->y + weight0 * pEdge0->y;
    relative.z = weight2 * pEdge2->z + weight0 * pEdge0->z;
    *pFeature = 7;
}

checkVertex: {
    f32 squaredDistance = relative.x * relative.x + relative.y * relative.y + relative.z * relative.z;
    f32 distance = MR::sqrt(squaredDistance);

    if (distance > distances[0] || distance >= radius) {
        *pFeature = 0;
        return false;
    }

    *pDistance = radiusSquared - squaredDistance;
}

calcDistance:
    *pDistance = MR::sqrt(*pDistance) - distances[0];

    if (*pDistance < 0.0f || thickness < *pDistance) {
        *pFeature = 0;
        return false;
    }

accepted:
    return true;
}

bool KCollisionServer::KCHitSphereWithThickness(KC_PrismData* pPrism, Fxyz* pPos, f32 radius, f32 scale, f32* pDistance, u8* pFeature,
                                                  f32 requestedThickness) {
    f32 thickness = requestedThickness * scale;
    f32 radiusSquared = radius * radius;
    *pFeature = 0;
    Fxyz relative;
    f32 distances[4];
    f32 dot01;
    f32 dot12;
    f32 dot20;
    TVec3f* pOrigin = &mFile->mPos[pPrism->mPositionIndex];
    relative.x = pPos->x - pOrigin->x;
    relative.y = pPos->y - pOrigin->y;
    relative.z = pPos->z - pOrigin->z;
    TVec3f* pEdge0 = &mFile->mNorms[pPrism->mEdgeIndices[0]];
    distances[1] = relative.x * pEdge0->x + relative.y * pEdge0->y + relative.z * pEdge0->z;

    if (distances[1] >= radius) {
        return false;
    }

    TVec3f* pEdge1 = &mFile->mNorms[pPrism->mEdgeIndices[1]];
    distances[2] = relative.x * pEdge1->x + relative.y * pEdge1->y + relative.z * pEdge1->z;

    if (distances[2] >= radius) {
        return false;
    }

    TVec3f* pEdge2 = &mFile->mNorms[pPrism->mEdgeIndices[2]];
    distances[3] = relative.x * pEdge2->x + relative.y * pEdge2->y + relative.z * pEdge2->z - pPrism->mHeight;

    if (distances[3] >= radius) {
        return false;
    }

    TVec3f* pNormal = &mFile->mNorms[pPrism->mNormalIndex];
    distances[0] = relative.x * pNormal->x + relative.y * pNormal->y + relative.z * pNormal->z;
    *pDistance = radius - distances[0];

    if (*pDistance < 0.0f) {
        return false;
    }

    if (distances[1] > distances[2]) {
        if (!(distances[1] > distances[3])) {
            goto edge2Region;
        }
    } else {
        if (!(distances[2] > distances[3])) {
            goto edge2Region;
        }

        goto edge1Region;
    }

edge0Region:
    if (distances[1] <= 0.0f) {
        if (thickness < *pDistance) {
            return false;
        }

        *pFeature = 1;
        goto accepted;
    }

    if (distances[2] > distances[3]) {
        dot01 = pEdge0->x * pEdge1->x + pEdge0->y * pEdge1->y + pEdge0->z * pEdge1->z;

        if (!(dot01 * distances[1] > distances[2])) {
            goto vertex0;
        }

        goto edge0;
    } else {
        dot20 = pEdge0->x * pEdge2->x + pEdge0->y * pEdge2->y + pEdge0->z * pEdge2->z;

        if (!(dot20 * distances[1] > distances[3])) {
            goto vertex2;
        }

        goto edge0;
    }

edge1Region:
    if (distances[2] <= 0.0f) {
        if (thickness < *pDistance) {
            return false;
        }

        *pFeature = 1;
        goto accepted;
    }

    if (distances[3] > distances[1]) {
        dot12 = pEdge1->x * pEdge2->x + pEdge1->y * pEdge2->y + pEdge1->z * pEdge2->z;

        if (!(dot12 * distances[2] > distances[3])) {
            goto vertex1;
        }

        goto edge1;
    } else {
        dot01 = pEdge1->x * pEdge0->x + pEdge1->y * pEdge0->y + pEdge1->z * pEdge0->z;

        if (!(dot01 * distances[2] > distances[1])) {
            goto vertex0;
        }

        goto edge1;
    }

edge2Region:
    if (distances[3] <= 0.0f) {
        if (thickness < *pDistance) {
            return false;
        }

        *pFeature = 1;
        goto accepted;
    }

    if (distances[1] > distances[2]) {
        dot20 = pEdge2->x * pEdge0->x + pEdge2->y * pEdge0->y + pEdge2->z * pEdge0->z;

        if (!(dot20 * distances[3] > distances[1])) {
            goto vertex2;
        }

        goto edge2;
    } else {
        dot12 = pEdge2->x * pEdge1->x + pEdge2->y * pEdge1->y + pEdge2->z * pEdge1->z;

        if (!(dot12 * distances[3] > distances[2])) {
            goto vertex1;
        }

        goto edge2;
    }

edge0:
    *pDistance = radiusSquared - distances[1] * distances[1];
    *pFeature = 2;
    goto calcDistance;

edge1:
    *pDistance = radiusSquared - distances[2] * distances[2];
    *pFeature = 3;
    goto calcDistance;

edge2:
    *pDistance = radiusSquared - distances[3] * distances[3];
    *pFeature = 4;
    goto calcDistance;

vertex0: {
    f32 weight0 = (dot01 * distances[2] - distances[1]) / (dot01 * dot01 - 1.0f);
    f32 weight1 = distances[2] - weight0 * dot01;
    relative.x = weight0 * pEdge0->x + weight1 * pEdge1->x;
    relative.y = weight0 * pEdge0->y + weight1 * pEdge1->y;
    relative.z = weight0 * pEdge0->z + weight1 * pEdge1->z;
    *pFeature = 5;
    goto checkVertex;
}

vertex1: {
    f32 weight1 = (dot12 * distances[3] - distances[2]) / (dot12 * dot12 - 1.0f);
    f32 weight2 = distances[3] - weight1 * dot12;
    relative.x = weight1 * pEdge1->x + weight2 * pEdge2->x;
    relative.y = weight1 * pEdge1->y + weight2 * pEdge2->y;
    relative.z = weight1 * pEdge1->z + weight2 * pEdge2->z;
    *pFeature = 6;
    goto checkVertex;
}

vertex2: {
    f32 weight2 = (dot20 * distances[1] - distances[3]) / (dot20 * dot20 - 1.0f);
    f32 weight0 = distances[1] - weight2 * dot20;
    relative.x = weight2 * pEdge2->x + weight0 * pEdge0->x;
    relative.y = weight2 * pEdge2->y + weight0 * pEdge0->y;
    relative.z = weight2 * pEdge2->z + weight0 * pEdge0->z;
    *pFeature = 7;
}

checkVertex: {
    f32 squaredDistance = relative.x * relative.x + relative.y * relative.y + relative.z * relative.z;
    f32 distance = MR::sqrt(squaredDistance);

    if (distance >= radius) {
        *pFeature = 0;
        return false;
    }

    *pDistance = radiusSquared - squaredDistance;
}

calcDistance: {
    f32 distance = MR::sqrt(*pDistance);

    if (distances[0] + distance < 0.0f) {
        *pFeature = 0;
        return false;
    }

    *pDistance = distance - distances[0];

    if (*pDistance < 0.0f || thickness < *pDistance) {
        *pFeature = 0;
        return false;
    }
}

accepted:
    return true;
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

bool KCollisionServer::KCHitArrow(KC_PrismData* pPrism, const TVec3f& rPos, const TVec3f& rOffset, f32* pDistance, u8* pFeature) const {
    TVec3f* pOrigin = &mFile->mPos[pPrism->mPositionIndex];
    TVec3f* pNormal = &mFile->mNorms[pPrism->mNormalIndex];
    TVec3f relative;
    PSVECSubtract(&rPos, pOrigin, &relative);
    f32 faceDistance = PSVECDotProduct(&relative, pNormal);

    if (faceDistance <= 0.0f) {
        *pFeature = 0;
        return false;
    }

    f32 faceMovement = PSVECDotProduct(pNormal, &rOffset);

    if (0.0f < faceDistance + faceMovement) {
        *pFeature = 0;
        return false;
    }

    f32 fraction = faceDistance / -faceMovement;
    TVec3f position = rOffset * fraction;
    position += relative;
    bool edge0 = false;
    bool edge1 = false;
    bool edge2 = false;
    f32 distance = PSVECDotProduct(&position, &mFile->mNorms[pPrism->mEdgeIndices[0]]);

    if (0.01f < distance) {
        *pFeature = 0;
        return false;
    }

    if (0.0f <= distance && distance <= 0.01f) {
        edge0 = true;
    }

    distance = PSVECDotProduct(&position, &mFile->mNorms[pPrism->mEdgeIndices[1]]);

    if (0.01f < distance) {
        *pFeature = 0;
        return false;
    }

    if (0.0f <= distance && distance <= 0.01f) {
        edge1 = true;
    }

    distance = PSVECDotProduct(&position, &mFile->mNorms[pPrism->mEdgeIndices[2]]);

    if (0.01f + pPrism->mHeight < distance) {
        *pFeature = 0;
        return false;
    }

    if (0.0f <= distance && distance <= 0.01f) {
        edge2 = true;
    }

    *pDistance = fraction;

    if (edge0) {
        if (edge1) {
            if (edge2) {
                *pFeature = 1;
            } else {
                *pFeature = 5;
            }
        } else if (edge2) {
            *pFeature = 7;
        } else {
            *pFeature = 2;
        }
    } else if (edge1) {
        if (edge2) {
            *pFeature = 6;
        } else {
            *pFeature = 3;
        }
    } else if (edge2) {
        *pFeature = 4;
    } else {
        *pFeature = 1;
    }

    return true;
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

// Register mismatch
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
