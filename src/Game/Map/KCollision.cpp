#include "Game/Map/KCollision.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/MathUtil.hpp"

namespace {
    static f32 square(f32 value) {
        return value * value;
    }

    static f32 absF(f32 value) {
        return value < 0.0f ? -value : value;
    }

    static TVec3f toVec(const Fxyz& rVec) {
        return TVec3f(rVec.x, rVec.y, rVec.z);
    }

    static TVec3f addScaled(const TVec3f& rBase, const TVec3f& rVec, f32 scale) {
        return TVec3f(rBase.x + rVec.x * scale, rBase.y + rVec.y * scale, rBase.z + rVec.z * scale);
    }

    static f32 dot3(const TVec3f& rA, const TVec3f& rB) {
        return rA.x * rB.x + rA.y * rB.y + rA.z * rB.z;
    }

    static f32 squareDistance(const TVec3f& rA, const TVec3f& rB) {
        return square(rA.x - rB.x) + square(rA.y - rB.y) + square(rA.z - rB.z);
    }

    static bool containsPrism(KC_PrismData** pBegin, KC_PrismData** pEnd, KC_PrismData* pPrism) {
        for (KC_PrismData** pIter = pBegin; pIter != pEnd; ++pIter) {
            if (*pIter == pPrism) {
                return true;
            }
        }

        return false;
    }

    static KC_PrismData* nextPrismInLeaf(const KCLFile* pFile, u16*& rCursor) {
        ++rCursor;
        u16 index = *rCursor;

        if (index == 0) {
            return nullptr;
        }

        return &pFile->mPrisms[index];
    }

    static bool isFlatOctree(const KCLFile* pFile) {
        return pFile->mBlockXYShift == -1 && pFile->mBlockXShift == -1;
    }

    static void makeSphereBounds(const Fxyz& rCenter, f32 radius, TVec3f* pMin, TVec3f* pMax) {
        pMin->set(rCenter.x - radius, rCenter.y - radius, rCenter.z - radius);
        pMax->set(rCenter.x + radius, rCenter.y + radius, rCenter.z + radius);
    }

    static bool checkAABBOverlap(const TVec3f& rMinA, const TVec3f& rMaxA, const TVec3f& rMinB, const TVec3f& rMaxB) {
        return rMaxA.x >= rMinB.x && rMaxA.y >= rMinB.y && rMaxA.z >= rMinB.z && rMaxB.x >= rMinA.x && rMaxB.y >= rMinA.y && rMaxB.z >= rMinA.z;
    }

    static void createTriangleBounds(KCollisionServer* pServer, KC_PrismData* pPrism, TVec3f* pMin, TVec3f* pMax) {
        TVec3f vertices[3] = {
            pServer->getPos(pPrism, 0),
            pServer->getPos(pPrism, 1),
            pServer->getPos(pPrism, 2),
        };

        MR::createBoundingBox(vertices, 3, pMin, pMax);
    }

    static TVec3f closestPointOnTriangle(const TVec3f& rPoint, const TVec3f& rA, const TVec3f& rB, const TVec3f& rC, u8* pHitCode) {
        TVec3f ab = rB - rA;
        TVec3f ac = rC - rA;
        TVec3f ap = rPoint - rA;
        f32 d1 = dot3(ab, ap);
        f32 d2 = dot3(ac, ap);

        if (d1 <= 0.0f && d2 <= 0.0f) {
            *pHitCode = 5;
            return rA;
        }

        TVec3f bp = rPoint - rB;
        f32 d3 = dot3(ab, bp);
        f32 d4 = dot3(ac, bp);

        if (d3 >= 0.0f && d4 <= d3) {
            *pHitCode = 6;
            return rB;
        }

        f32 vc = d1 * d4 - d3 * d2;

        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            f32 v = d1 / (d1 - d3);
            *pHitCode = 3;
            return addScaled(rA, ab, v);
        }

        TVec3f cp = rPoint - rC;
        f32 d5 = dot3(ab, cp);
        f32 d6 = dot3(ac, cp);

        if (d6 >= 0.0f && d5 <= d6) {
            *pHitCode = 7;
            return rC;
        }

        f32 vb = d5 * d2 - d1 * d6;

        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            f32 w = d2 / (d2 - d6);
            *pHitCode = 2;
            return addScaled(rA, ac, w);
        }

        f32 va = d3 * d6 - d5 * d4;

        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            TVec3f bc = rC - rB;
            f32 w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            *pHitCode = 4;
            return addScaled(rB, bc, w);
        }

        f32 denom = 1.0f / (va + vb + vc);
        f32 v = vb * denom;
        f32 w = vc * denom;
        *pHitCode = 1;
        return TVec3f(rA.x + ab.x * v + ac.x * w, rA.y + ab.y * v + ac.y * w, rA.z + ab.z * v + ac.z * w);
    }

    static bool hitSphereImpl(KCollisionServer* pServer, KC_PrismData* pPrism, Fxyz* pCenter, f32 radius, f32 maxDistance, f32* pDistance,
                              u8* pHitCode, f32 thickness) {
        *pHitCode = 0;

        if (pPrism->mHeight <= 0.0f) {
            return false;
        }

        TVec3f center = toVec(*pCenter);
        TVec3f p0 = pServer->getPos(pPrism, 0);
        TVec3f p1 = pServer->getPos(pPrism, 1);
        TVec3f p2 = pServer->getPos(pPrism, 2);
        const TVec3f& normal = *pServer->getFaceNormal(pPrism);
        TVec3f local = center - p0;
        f32 planeDistance = dot3(local, normal);
        f32 faceDistance = radius - planeDistance;

        *pDistance = faceDistance;

        if (faceDistance < 0.0f) {
            return false;
        }

        TVec3f projected = addScaled(center, normal, -planeDistance);
        u8 hitCode = 0;
        TVec3f closest = closestPointOnTriangle(projected, p0, p1, p2, &hitCode);

        if (hitCode == 1) {
            if (faceDistance > thickness || faceDistance > maxDistance) {
                return false;
            }

            *pHitCode = 1;
            return true;
        }

        f32 lateralSq = squareDistance(projected, closest);

        if (lateralSq >= square(radius)) {
            return false;
        }

        f32 normalReach = MR::sqrt< f32 >(square(radius) - lateralSq);
        f32 distance = normalReach - planeDistance;

        if (distance < 0.0f || distance > thickness || distance > maxDistance) {
            return false;
        }

        *pDistance = distance;
        *pHitCode = hitCode;
        return true;
    }

    static u32 stepAxis(u32 value, u32 blockSize) {
        return blockSize - (value & (blockSize - 1));
    }
}  // namespace

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

    if (isBinaryInitialized(pData)) {
        mFile->mPos = reinterpret_cast< TVec3f* >(reinterpret_cast< u8* >(mFile) + mFile->mPosOffset);
        mFile->mNorms = reinterpret_cast< TVec3f* >(reinterpret_cast< u8* >(mFile) + mFile->mNormOffset);
        mFile->mPrisms = reinterpret_cast< KC_PrismData* >(reinterpret_cast< u8* >(mFile) + mFile->mPrismOffset);
        mFile->mOctree = reinterpret_cast< void* >(reinterpret_cast< u8* >(mFile) + mFile->mOctreeOffset);
    }
}

void KCollisionServer::calcFarthestVertexDistance() {
    f32 maxDistanceSq = 0.0f;

    if (isFlatOctree(mFile)) {
        s32 shift;
        u32 coord = 0;
        u16* cursor = reinterpret_cast< u16* >(searchBlock(&shift, coord, coord, coord));

        while (KC_PrismData* prism = nextPrismInLeaf(mFile, cursor)) {
            if (isNearParallelNormal(prism)) {
                prism->mHeight = -absF(prism->mHeight);
                continue;
            }

            for (s32 vertex = 0; vertex < 3; vertex++) {
                TVec3f pos = getPos(prism, vertex);
                f32 distanceSq = dot3(pos, pos);

                if (maxDistanceSq < distanceSq) {
                    maxDistanceSq = distanceSq;
                }
            }
        }

        mMaxVertexDistance = MR::sqrt< f32 >(maxDistanceSq);
        return;
    }

    s32 triCount = getTriangleNum();

    for (s32 i = 0; i < triCount; i++) {
        KC_PrismData* prism = getPrismData(i);
        if (isNearParallelNormal(prism)) {
            prism->mHeight = -absF(prism->mHeight);
            continue;
        }

        for (s32 vertex = 0; vertex < 3; vertex++) {
            TVec3f pos = getPos(prism, vertex);
            f32 distanceSq = dot3(pos, pos);

            if (maxDistanceSq < distanceSq) {
                maxDistanceSq = distanceSq;
            }
        }
    }

    mMaxVertexDistance = MR::sqrt< f32 >(maxDistanceSq);
}

KC_PrismData* KCollisionServer::checkPoint(Fxyz* pPoint, f32 thicknessScale, f32* pDistance) {
    KCLFile* file = mFile;
    V3u local;
    objectSpaceToLocalSpace(&local, toVec(*pPoint));

    if ((local.x & file->mXMask) != 0 || (local.y & file->mYMask) != 0 || (local.z & file->mZMask) != 0) {
        return nullptr;
    }

    s32 shift;
    u32 x = static_cast< u32 >(local.x);
    u32 y = static_cast< u32 >(local.y);
    u32 z = static_cast< u32 >(local.z);
    u16* cursor = reinterpret_cast< u16* >(searchBlock(&shift, x, y, z));
    TVec3f point = toVec(*pPoint);
    f32 maxThickness = file->mThickness * thicknessScale;

    while (KC_PrismData* prism = nextPrismInLeaf(file, cursor)) {
        if (prism->mHeight <= 0.0f) {
            continue;
        }

        TVec3f diff = point - file->mPos[prism->mPositionIndex];

        if (dot3(diff, file->mNorms[prism->mEdgeIndices[0]]) > 0.0f) {
            continue;
        }

        if (dot3(diff, file->mNorms[prism->mEdgeIndices[1]]) > 0.0f) {
            continue;
        }

        if (dot3(diff, file->mNorms[prism->mEdgeIndices[2]]) > prism->mHeight) {
            continue;
        }

        f32 distance = thicknessScale - dot3(diff, file->mNorms[prism->mNormalIndex]);

        if (distance >= 0.0f && distance <= maxThickness) {
            *pDistance = distance;
            return prism;
        }
    }

    return nullptr;
}

u32 KCollisionServer::checkArea3D(Fxyz* pPosA, Fxyz* pPosB, KC_PrismData** pPrisms, u32 maxCount) {
    if (pPrisms == nullptr || maxCount == 0) {
        return 0;
    }

    TVec3f points[2] = {
        toVec(*pPosA),
        toVec(*pPosB),
    };
    TVec3f queryMin;
    TVec3f queryMax;

    MR::createBoundingBox(points, 2, &queryMin, &queryMax);

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

    V3u minPoint;
    V3u maxPoint;

    if (!outCheck(&queryMin, &queryMax, &minPoint, &maxPoint)) {
        return 0;
    }

    u32 count = 0;
    u16* previousLeaf = nullptr;

    for (u32 z = static_cast< u32 >(minPoint.z); z <= static_cast< u32 >(maxPoint.z);) {
        u32 zStep = 0xF4240;

        for (u32 y = static_cast< u32 >(minPoint.y); y <= static_cast< u32 >(maxPoint.y);) {
            u32 yStep = 0xF4240;

            for (u32 x = static_cast< u32 >(minPoint.x); x <= static_cast< u32 >(maxPoint.x);) {
                s32 shift;
                u16* leaf = reinterpret_cast< u16* >(searchBlock(&shift, x, y, z));
                u32 blockSize = 1U << shift;
                u32 stepX = stepAxis(x, blockSize);
                u32 stepY = stepAxis(y, blockSize);
                u32 stepZ = stepAxis(z, blockSize);

                if (stepZ < zStep) {
                    zStep = stepZ;
                }

                if (stepY < yStep) {
                    yStep = stepY;
                }

                if (leaf != previousLeaf) {
                    u16* cursor = leaf;

                    while (KC_PrismData* prism = nextPrismInLeaf(mFile, cursor)) {
                        if (prism->mHeight <= 0.0f || containsPrism(pPrisms, pPrisms + count, prism)) {
                            continue;
                        }

                        TVec3f triMin;
                        TVec3f triMax;
                        createTriangleBounds(this, prism, &triMin, &triMax);

                        if (checkAABBOverlap(queryMin, queryMax, triMin, triMax)) {
                            pPrisms[count++] = prism;

                            if (count == maxCount) {
                                return count;
                            }
                        }
                    }
                }

                previousLeaf = leaf;

                if (stepX < 1) {
                    stepX = 1;
                }

                x += stepX;
            }

            if (yStep < 1) {
                yStep = 1;
            }

            y += yStep;
        }

        if (zStep < 1) {
            zStep = 1;
        }

        z += zStep;
    }

    return count;
}

u32 KCollisionServer::checkSphere(Fxyz* pCenter, f32 radius, f32 maxDistance, u32 maxCount, KC_PrismData** pPrisms, f32* pDistances, u8* pHitCodes) {
    if (pPrisms == nullptr || pDistances == nullptr || pHitCodes == nullptr || maxCount == 0) {
        return 0;
    }

    TVec3f queryMin;
    TVec3f queryMax;
    makeSphereBounds(*pCenter, radius, &queryMin, &queryMax);

    V3u minPoint;
    V3u maxPoint;

    if (!outCheck(&queryMin, &queryMax, &minPoint, &maxPoint)) {
        return 0;
    }

    u32 count = 0;
    u16* previousLeaf = nullptr;

    for (u32 z = static_cast< u32 >(minPoint.z); z <= static_cast< u32 >(maxPoint.z);) {
        u32 zStep = 0xF4240;

        for (u32 y = static_cast< u32 >(minPoint.y); y <= static_cast< u32 >(maxPoint.y);) {
            u32 yStep = 0xF4240;

            for (u32 x = static_cast< u32 >(minPoint.x); x <= static_cast< u32 >(maxPoint.x);) {
                s32 shift;
                u16* leaf = reinterpret_cast< u16* >(searchBlock(&shift, x, y, z));
                u32 blockSize = 1U << shift;
                u32 stepX = stepAxis(x, blockSize);
                u32 stepY = stepAxis(y, blockSize);
                u32 stepZ = stepAxis(z, blockSize);

                if (stepZ < zStep) {
                    zStep = stepZ;
                }

                if (stepY < yStep) {
                    yStep = stepY;
                }

                if (leaf != previousLeaf) {
                    u16* cursor = leaf;

                    while (KC_PrismData* prism = nextPrismInLeaf(mFile, cursor)) {
                        if (prism->mHeight <= 0.0f || containsPrism(pPrisms, pPrisms + count, prism)) {
                            continue;
                        }

                        f32 distance = 0.0f;
                        u8 hitCode = 0;

                        if (KCHitSphere(prism, pCenter, radius, maxDistance, &distance, &hitCode) && count < maxCount &&
                            !containsPrism(pPrisms, pPrisms + count, prism)) {
                            pPrisms[count] = prism;
                            pDistances[count] = distance;
                            pHitCodes[count] = hitCode;
                            count++;
                        }
                    }
                }

                previousLeaf = leaf;

                if (stepX < 1) {
                    stepX = 1;
                }

                x += stepX;
            }

            if (yStep < 1) {
                yStep = 1;
            }

            y += yStep;
        }

        if (zStep < 1) {
            zStep = 1;
        }

        z += zStep;
    }

    return count;
}

u32 KCollisionServer::checkSphereWithThickness(Fxyz* pCenter, f32 radius, f32 maxDistance, u32 maxCount, KC_PrismData** pPrisms, f32* pDistances,
                                               u8* pHitCodes, f32 thickness) {
    if (pPrisms == nullptr || pDistances == nullptr || pHitCodes == nullptr || maxCount == 0) {
        return 0;
    }

    TVec3f queryMin;
    TVec3f queryMax;
    makeSphereBounds(*pCenter, radius, &queryMin, &queryMax);

    V3u minPoint;
    V3u maxPoint;

    if (!outCheck(&queryMin, &queryMax, &minPoint, &maxPoint)) {
        return 0;
    }

    u32 count = 0;
    u16* previousLeaf = nullptr;

    for (u32 z = static_cast< u32 >(minPoint.z); z <= static_cast< u32 >(maxPoint.z);) {
        u32 zStep = 0xF4240;

        for (u32 y = static_cast< u32 >(minPoint.y); y <= static_cast< u32 >(maxPoint.y);) {
            u32 yStep = 0xF4240;

            for (u32 x = static_cast< u32 >(minPoint.x); x <= static_cast< u32 >(maxPoint.x);) {
                s32 shift;
                u16* leaf = reinterpret_cast< u16* >(searchBlock(&shift, x, y, z));
                u32 blockSize = 1U << shift;
                u32 stepX = stepAxis(x, blockSize);
                u32 stepY = stepAxis(y, blockSize);
                u32 stepZ = stepAxis(z, blockSize);

                if (stepZ < zStep) {
                    zStep = stepZ;
                }

                if (stepY < yStep) {
                    yStep = stepY;
                }

                if (leaf != previousLeaf) {
                    u16* cursor = leaf;

                    while (KC_PrismData* prism = nextPrismInLeaf(mFile, cursor)) {
                        if (prism->mHeight <= 0.0f || containsPrism(pPrisms, pPrisms + count, prism)) {
                            continue;
                        }

                        f32 distance = 0.0f;
                        u8 hitCode = 0;

                        if (KCHitSphereWithThickness(prism, pCenter, radius, maxDistance, &distance, &hitCode, thickness) && count < maxCount &&
                            !containsPrism(pPrisms, pPrisms + count, prism)) {
                            pPrisms[count] = prism;
                            pDistances[count] = distance;
                            pHitCodes[count] = hitCode;
                            count++;
                        }
                    }
                }

                previousLeaf = leaf;

                if (stepX < 1) {
                    stepX = 1;
                }

                x += stepX;
            }

            if (yStep < 1) {
                yStep = 1;
            }

            y += yStep;
        }

        if (zStep < 1) {
            zStep = 1;
        }

        z += zStep;
    }

    return count;
}

bool KCollisionServer::isBinaryInitialized(const void* pData) {
    return reinterpret_cast< const s32* >(pData)[0] < 0;
}

KC_PrismData* KCollisionServer::checkArrow(const TVec3f& rStart, const TVec3f& rDelta, f32* pDistances, u8* pHitCode, u32* pHitCount,
                                           KC_PrismData** pPrisms, u32 maxCount) const {
    if (rDelta.x == 0.0f && rDelta.y == 0.0f && rDelta.z == 0.0f) {
        return nullptr;
    }

    KC_PrismData* bestPrism = nullptr;
    f32 bestDistance = 1.0f;
    u32 count = 0;
    TVec3f end = rStart + rDelta;
    TVec3f points[2] = {
        rStart,
        end,
    };
    TVec3f queryMin;
    TVec3f queryMax;

    MR::createBoundingBox(points, 2, &queryMin, &queryMax);

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

    V3u minPoint;
    V3u maxPoint;

    if (!outCheck(&queryMin, &queryMax, &minPoint, &maxPoint)) {
        return nullptr;
    }

    u16* previousLeaf = nullptr;

    for (u32 z = static_cast< u32 >(minPoint.z); z <= static_cast< u32 >(maxPoint.z);) {
        u32 zStep = 0xF4240;

        for (u32 y = static_cast< u32 >(minPoint.y); y <= static_cast< u32 >(maxPoint.y);) {
            u32 yStep = 0xF4240;

            for (u32 x = static_cast< u32 >(minPoint.x); x <= static_cast< u32 >(maxPoint.x);) {
                s32 shift;
                u16* leaf = reinterpret_cast< u16* >(searchBlock(&shift, x, y, z));
                u32 blockSize = 1U << shift;
                u32 stepX = stepAxis(x, blockSize);
                u32 stepY = stepAxis(y, blockSize);
                u32 stepZ = stepAxis(z, blockSize);

                if (stepZ < zStep) {
                    zStep = stepZ;
                }

                if (stepY < yStep) {
                    yStep = stepY;
                }

                if (leaf != previousLeaf) {
                    u16* cursor = leaf;

                    while (KC_PrismData* prism = nextPrismInLeaf(mFile, cursor)) {
                        if (prism->mHeight <= 0.0f || (pPrisms != nullptr && containsPrism(pPrisms, pPrisms + count, prism))) {
                            continue;
                        }

                        f32 distance = 1.0f;
                        u8 hitCode = 0;

                        if (!KCHitArrow(prism, rStart, rDelta, &distance, &hitCode)) {
                            continue;
                        }

                        if (pPrisms != nullptr) {
                            if (count < maxCount) {
                                pDistances[count] = distance;
                                pPrisms[count] = prism;

                                if (distance < bestDistance) {
                                    bestDistance = distance;
                                    bestPrism = prism;
                                }

                                count++;
                            }

                            if (count == maxCount) {
                                if (pHitCount != nullptr) {
                                    *pHitCount = count;
                                }

                                return bestPrism;
                            }
                        } else if (distance < bestDistance) {
                            *pDistances = distance;
                            bestDistance = distance;
                            bestPrism = prism;

                            if (pHitCode != nullptr) {
                                *pHitCode = hitCode;
                            }
                        }
                    }
                }

                previousLeaf = leaf;

                if (stepX < 1) {
                    stepX = 1;
                }

                x += stepX;
            }

            if (yStep < 1) {
                yStep = 1;
            }

            y += yStep;
        }

        if (zStep < 1) {
            zStep = 1;
        }

        z += zStep;
    }

    if (pHitCount != nullptr) {
        *pHitCount = count;
    }

    return bestPrism;
}

bool KCollisionServer::KCHitSphere(KC_PrismData* pPrism, Fxyz* pCenter, f32 radius, f32 maxDistance, f32* pDistance, u8* pHitCode) {
    return hitSphereImpl(this, pPrism, pCenter, radius, maxDistance, pDistance, pHitCode, mFile->mThickness * maxDistance);
}

bool KCollisionServer::KCHitSphereWithThickness(KC_PrismData* pPrism, Fxyz* pCenter, f32 radius, f32 maxDistance, f32* pDistance, u8* pHitCode,
                                                f32 thickness) {
    return hitSphereImpl(this, pPrism, pCenter, radius, maxDistance, pDistance, pHitCode, thickness * maxDistance);
}

bool KCollisionServer::isNearParallelNormal(const KC_PrismData* pPrism) const {
    TVec3f edge0 = mFile->mNorms[pPrism->mEdgeIndices[0]];
    TVec3f edge1 = mFile->mNorms[pPrism->mEdgeIndices[1]];
    TVec3f edge2 = mFile->mNorms[pPrism->mEdgeIndices[2]];

    bool isNear = false;

    if (MR::isSameDirection(edge0, edge1, 0.01f) || MR::isSameDirection(edge0, edge2, 0.01f) || MR::isSameDirection(edge1, edge2, 0.01f)) {
        isNear = true;
    }

    return isNear;
}

bool KCollisionServer::KCHitArrow(KC_PrismData* pPrism, const TVec3f& rStart, const TVec3f& rDelta, f32* pDistance, u8* pHitCode) const {
    *pHitCode = 0;

    if (pPrism->mHeight <= 0.0f) {
        return false;
    }

    TVec3f p0 = getPos(pPrism, 0);
    TVec3f rel = rStart - p0;
    const TVec3f& normal = *getFaceNormal(pPrism);
    f32 startDistance = dot3(rel, normal);
    f32 deltaDistance = dot3(rDelta, normal);

    if (startDistance <= 0.0f || deltaDistance >= 0.0f) {
        return false;
    }

    f32 t = startDistance / -deltaDistance;

    if (t < 0.0f || t > 1.0f) {
        return false;
    }

    TVec3f localHit = addScaled(rel, rDelta, t);
    f32 edge0 = dot3(localHit, *getEdgeNormal1(pPrism));
    f32 edge1 = dot3(localHit, *getEdgeNormal2(pPrism));
    f32 edge2 = dot3(localHit, *getEdgeNormal3(pPrism));

    if (edge0 > 1.0f || edge1 > 1.0f || edge2 > pPrism->mHeight + 1.0f) {
        return false;
    }

    bool edge0Border = edge0 >= 0.0f && edge0 <= 1.0f;
    bool edge1Border = edge1 >= 0.0f && edge1 <= 1.0f;
    bool edge2Border = edge2 >= 0.0f && edge2 <= 1.0f;

    *pDistance = t;

    if (edge0Border) {
        if (edge1Border) {
            *pHitCode = edge2Border ? 1 : 5;
        } else {
            *pHitCode = edge2Border ? 7 : 2;
        }
    } else if (edge1Border) {
        *pHitCode = edge2Border ? 6 : 3;
    } else {
        *pHitCode = edge2Border ? 4 : 1;
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

    JMapInfoIter iter;
    iter.mInfo = mapInfo;
    iter.mIndex = prism->mAttribute;

    return iter;
}

// Register mismatch
s32* KCollisionServer::searchBlock(s32* a1, const u32& rX, const u32& rY, const u32& rZ) const {
    KCLFile* file = mFile;
    s32 blockWidthShift = file->mBlockWidthShift;
    u8* octree = reinterpret_cast< u8* >(file->mOctree);
    *a1 = blockWidthShift;

    s32 offset = ((rX >> blockWidthShift) | ((rZ >> blockWidthShift) << file->mBlockXYShift) | ((rY >> blockWidthShift) << file->mBlockXShift)) * 4;

    if (file->mBlockXYShift == -1 && file->mBlockXShift == -1) {
        offset = 0;
    }

    while ((offset = *reinterpret_cast< s32* >(octree + offset)) >= 0) {
        octree += offset;
        s32 uVar7 = --(*a1);

        offset = ((((rZ >> uVar7) & 1) << 2) | (((rY >> uVar7) & 1) << 1) | ((rX >> uVar7) & 1)) * 4;
    }

    return reinterpret_cast< s32* >(octree + (offset & 0x7FFFFFFF));
}
