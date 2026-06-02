#include "Game/AreaObj/CollisionArea.hpp"
#include "Game/AreaObj/AreaForm.hpp"
#include "Game/MapObj/DynamicCollisionObj.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Inline.hpp"

DynamicCollisionObj::~DynamicCollisionObj() {}

TVec3f TVec3f::operator-() const {
    TVec3f ret;
    JMathInlineVEC::PSVECNegate(this, &ret);
    return ret;
}

bool AreaObj::isValid() const {
    return mIsValid && _15 && mIsAwake;
}

AreaPolygon::AreaPolygon() : DynamicCollisionObj("エリアポリゴン") {
    mForm = nullptr;
    _128 = nullptr;
    _12C.zero();
}

AreaPolygon::~AreaPolygon() {}

void AreaPolygon::init(const JMapInfoIter &rIter) {
    if (MR::isValidInfo(rIter)) {
        MR::initDefaultPos(this, rIter);
    }
    else {
        MR::extractMtxTrans(_128, &mPosition);
        mRotation.x = 0.0f;
        mRotation.y = 0.0f;
        mRotation.z = 0.0f;
    }

    mScale.x = 1.0f;
    mScale.y = 1.0f;
    mScale.z = 1.0f;
    mKCLFile = nullptr;
    mPositionNum = 4;
    _94 = 2;

    mPositions = new TVec3f[4];
    setSurface(0);
    initHitSensor(1);

    TVec3f sensorOffset(0.0f, 0.0f, 0.0f);
    MR::addHitSensorEye(this, "body", 8, 1.0f, sensorOffset);

    mIndices = new TriangleIndexing[_94];
    _9C = new TVec3f[_94];

    mIndices[0].mIndex[0] = 0;
    mIndices[0].mIndex[1] = 1;
    mIndices[0].mIndex[2] = 2;

    mIndices[1].mIndex[0] = 0;
    mIndices[1].mIndex[1] = 2;
    mIndices[1].mIndex[2] = 3;

    createCollision();
    makeActorAppeared();
}

void AreaPolygon::setMtx(MtxPtr matrix, const TVec3f& a2, f32 a3) {
    _128 = matrix;
    _12C = a2;
    _138 = a3;
}

void AreaPolygon::setSurfaceAndSync(s32 a1) {
    setSurface(a1);

    if (a1 != -1) {
        syncCollision();
    }
}

void AreaPolygon::setSurface(s32 surface) {
    TPos3f worldMtx;

    if (mForm != nullptr) {
        AreaFormCube* cube = static_cast< AreaFormCube* >(mForm);
        cube->calcWorldMtx(&worldMtx);
        cube->calcWorldPos(&mPosition);
    }
    else {
        PSMTXCopy(_128, worldMtx.toMtxPtr());
        MR::extractMtxTrans(_128, &mPosition);
    }

    TVec3f xDir;
    TVec3f yDir;
    TVec3f zDir;
    worldMtx.getXDir(xDir);
    worldMtx.getYDir(yDir);
    worldMtx.getZDir(zDir);

    if (_128 != nullptr) {
        TVec3f offset(yDir);
        offset.scale(_138);
        mPosition.add(offset);
    }

    f32 xExtent = PSVECMag(&xDir);
    MR::normalizeOrZero(&xDir);
    f32 yExtent = PSVECMag(&yDir);
    MR::normalizeOrZero(&yDir);
    f32 zExtent = PSVECMag(&zDir);
    MR::normalizeOrZero(&zDir);

    if (mForm != nullptr) {
        AreaFormCube* cube = static_cast< AreaFormCube* >(mForm);
        xExtent *= 0.5f * (cube->mScale.x * cube->getBaseSize());
        yExtent *= 0.5f * (cube->mScale.y * cube->getBaseSize());
        zExtent *= 0.5f * (cube->mScale.z * cube->getBaseSize());
    }
    else {
        xExtent *= 0.5f * _12C.x;
        yExtent *= 0.5f * _12C.y;
        zExtent *= 0.5f * _12C.z;
    }

    if (!MR::isPlayerElementModeTeresa()) {
        if (xExtent < 0.0f) {
            xExtent -= 10.0f;
        }
        else {
            xExtent += 10.0f;
        }

        if (yExtent < 0.0f) {
            yExtent -= 10.0f;
        }
        else {
            yExtent += 10.0f;
        }

        if (zExtent < 0.0f) {
            zExtent -= 10.0f;
        }
        else {
            zExtent += 10.0f;
        }
    }

    switch (surface) {
    case 0:
        mPositions[0].set(xExtent, -yExtent, -zExtent);
        mPositions[1].set(xExtent, yExtent, -zExtent);
        mPositions[2].set(xExtent, yExtent, zExtent);
        mPositions[3].set(xExtent, -yExtent, zExtent);
        break;
    case 1:
        mPositions[0].set(-xExtent, -yExtent, zExtent);
        mPositions[1].set(-xExtent, yExtent, zExtent);
        mPositions[2].set(-xExtent, yExtent, -zExtent);
        mPositions[3].set(-xExtent, -yExtent, -zExtent);
        break;
    case 2:
        mPositions[0].set(-xExtent, yExtent, -zExtent);
        mPositions[1].set(-xExtent, yExtent, zExtent);
        mPositions[2].set(xExtent, yExtent, zExtent);
        mPositions[3].set(xExtent, yExtent, -zExtent);
        break;
    case 3:
        mPositions[0].set(-xExtent, -yExtent, zExtent);
        mPositions[1].set(-xExtent, -yExtent, -zExtent);
        mPositions[2].set(xExtent, -yExtent, -zExtent);
        mPositions[3].set(xExtent, -yExtent, zExtent);
        break;
    case 4:
        mPositions[0].set(-xExtent, -yExtent, zExtent);
        mPositions[1].set(xExtent, -yExtent, zExtent);
        mPositions[2].set(xExtent, yExtent, zExtent);
        mPositions[3].set(-xExtent, yExtent, zExtent);
        break;
    case 5:
        mPositions[0].set(-xExtent, yExtent, -zExtent);
        mPositions[1].set(xExtent, yExtent, -zExtent);
        mPositions[2].set(xExtent, -yExtent, -zExtent);
        mPositions[3].set(-xExtent, -yExtent, -zExtent);
        break;
    }

    for (u32 i = 0; i < 4; i++) {
        TVec3f zPart(zDir);
        zPart.scale(mPositions[i].z);

        TVec3f yPart(yDir);
        yPart.scale(mPositions[i].y);

        TVec3f xPart(xDir);
        xPart.scale(mPositions[i].x);

        TVec3f position(xPart);
        position.add(yPart);

        TVec3f worldPos(position);
        worldPos.add(zPart);
        mPositions[i].set(worldPos);
    }
}

void AreaPolygon::invalidate() {
    MR::invalidateCollisionParts(mParts);
}

void AreaPolygon::validate() {
    MR::validateCollisionParts(mParts);
}

CollisionArea::CollisionArea(int formType, const char* pName) : AreaObj(formType, pName) {
    _3C = 0;
    _40 = 0.0f;
    _44.zero();
    _50 = 0;
    _54 = 0;
    _58 = false;
    _5C = 0;
    _60 = 0;
    mPolygon = nullptr;
    mIsValid = false;
}

CollisionArea::~CollisionArea() {}

void CollisionArea::init(const JMapInfoIter& rIter) {
    AreaObj::init(rIter);
    MR::connectToSceneAreaObj(this);

    _50 = -1;
    _3C = 0;
    _40 = 0.0f;
    _54 = -1;
    _5C = 0;
    _60 = -1;

    MR::getJMapInfoArg0NoInit(rIter, &_50);
    MR::getJMapInfoArg1NoInit(rIter, &_54);
    MR::getJMapInfoArg2NoInit(rIter, &_5C);
    MR::getJMapInfoArg3NoInit(rIter, &_60);

    _58 = false;

    if (_60 == 0) {
        mPolygon = nullptr;
    } else {
        mPolygon = new AreaPolygon();
        mPolygon->mForm = mForm;
        mPolygon->init(rIter);
        MR::validateCollisionParts(mPolygon->mParts);

        if (!isValid()) {
            MR::invalidateCollisionParts(mPolygon->mParts);
        }
    }

    mIsValid = isValid();
}

void CollisionArea::movement() {
    if (!isValid()) {
        if (mIsValid) {
            if (mPolygon != nullptr) {
                MR::invalidateCollisionParts(mPolygon->mParts);
            }

            mIsValid = false;
        }
    } else {
        if (!mIsValid) {
            if (mPolygon != nullptr) {
                MR::validateCollisionParts(mPolygon->mParts);
            }

            mIsValid = true;
        }

        if (_54 == -1 && MR::isPlayerTeresaDisappear()) {
            _58 = true;

            if (mPolygon != nullptr) {
                MR::invalidateCollisionParts(mPolygon->mParts);
            }
        } else {
            TVec3f playerPos = *MR::getPlayerCenterPos();
            f32 dVar4 = static_cast< f32 >(_5C);

            if (_5C == 0) {
                dVar4 = MR::getPlayerHitRadius();
            }

            if (_58) {
                dVar4 += 5.0f;
            }

            TVec3f tStack60;
            TVec3f tStack72;

            if (!hitCheck(playerPos, dVar4, &tStack60, &tStack72)) {
                if (_58) {
                    _58 = false;

                    if (mPolygon != nullptr) {
                        MR::validateCollisionParts(mPolygon->mParts);
                    }
                }
            } else if (!_58 && _50 == -1) {
                TVec3f tStack84;

                if (_3C == 0) {
                    tStack84 = tStack72;
                    tStack84.setLength(dVar4 + _40);
                } else {
                    TVec3f tStack96;
                    TVec3f tStack108 = playerPos;
                    tStack108.sub(tStack60);

                    tStack84 = tStack108;
                    tStack84.setLength(dVar4);

                    tStack96 = tStack60;
                    tStack96.add(tStack84);

                    TVec3f tStack120 = tStack96;
                    tStack120.sub(playerPos);
                    tStack84 = tStack120;
                }

                if (tStack84.dot(tStack72) > 0) {
                    MR::pushPlayerFromArea(tStack84);
                }
            }
        }
    }
}

bool CollisionArea::hitCheck(const TVec3f& rPlayerPos, f32 radius, TVec3f* pHitPos, TVec3f* pHitNormal) {
    AreaFormCube* cube = static_cast< AreaFormCube* >(mForm);
    s32 surface = -1;

    TPos3f worldMtx;
    cube->calcWorldMtx(&worldMtx);
    cube->calcWorldPos(&_44);

    TVec3f xDir;
    TVec3f yDir;
    TVec3f zDir;
    worldMtx.getXDir(xDir);
    worldMtx.getYDir(yDir);
    worldMtx.getZDir(zDir);

    f32 xExtent = PSVECMag(&xDir);
    MR::normalizeOrZero(&xDir);
    f32 yExtent = PSVECMag(&yDir);
    MR::normalizeOrZero(&yDir);
    f32 zExtent = PSVECMag(&zDir);
    MR::normalizeOrZero(&zDir);

    xExtent *= 0.5f * (cube->mScale.x * cube->getBaseSize());
    yExtent *= 0.5f * (cube->mScale.y * cube->getBaseSize());
    zExtent *= 0.5f * (cube->mScale.z * cube->getBaseSize());

    TVec3f limit;
    limit.y = yExtent + radius;
    limit.x = xExtent + radius;
    limit.z = zExtent + radius;

    TVec3f rel(rPlayerPos);
    rel.sub(_44);

    f32 xDist = rel.dot(xDir);
    f32 yDist = rel.dot(yDir);
    f32 zDist = rel.dot(zDir);

    TVec3f absDist;
    absDist.x = __fabsf(xDist);
    absDist.y = __fabsf(yDist);
    absDist.z = __fabsf(zDist);

    if (absDist.x >= limit.x || absDist.y >= limit.y || absDist.z >= limit.z) {
        return false;
    }

    u32 hitCount = 0;
    s32 hitX = 0;
    s32 hitY = 0;
    s32 hitZ = 0;

    if (absDist.x >= xExtent) {
        hitX = 1;
        hitCount++;
    }

    if (absDist.y >= yExtent) {
        hitY = 1;
        hitCount++;
    }

    if (absDist.z >= zExtent) {
        hitZ = 1;
        hitCount++;
    }

    _3C = hitCount;

    TVec3f signedDist;

    if (hitCount == 3) {
        TVec3f nearPoint;
        nearPoint.zero();

        if (xDist < 0.0f) {
            TVec3f neg(-xDir);
            TVec3f part(neg);
            part.scale(xExtent);
            nearPoint.add(part);
        }
        else {
            TVec3f part(xDir);
            part.scale(xExtent);
            nearPoint.add(part);
        }

        if (yDist < 0.0f) {
            TVec3f neg(-yDir);
            TVec3f part(neg);
            part.scale(yExtent);
            nearPoint.add(part);
        }
        else {
            TVec3f part(yDir);
            part.scale(yExtent);
            nearPoint.add(part);
        }

        if (zDist < 0.0f) {
            TVec3f neg(-zDir);
            TVec3f part(neg);
            part.scale(zExtent);
            nearPoint.add(part);
        }
        else {
            TVec3f part(zDir);
            part.scale(zExtent);
            nearPoint.add(part);
        }

        TVec3f toPlayer(nearPoint);
        toPlayer.sub(rPlayerPos);

        if (PSVECMag(&toPlayer) >= radius) {
            return false;
        }

        pHitPos->set(nearPoint);

        TVec3f normalXY(xDir);
        normalXY.add(yDir);

        TVec3f normal(normalXY);
        normal.add(zDir);
        pHitNormal->set(normal);
        MR::normalizeOrZero(pHitNormal);
        return true;
    }

    if (hitCount == 0) {
        TVec3f inside;
        inside.x = xExtent - absDist.x;
        inside.y = yExtent - absDist.y;
        inside.z = zExtent - absDist.z;

        if (inside.x < inside.y && inside.x < inside.z) {
            _40 = inside.x;
            hitX = 1;
        }
        else if (inside.y < inside.x && inside.y < inside.z) {
            _40 = inside.y;
            hitY = 1;
        }
        else {
            _40 = inside.z;
            hitZ = 1;
        }

        hitCount = 1;
    }

    if (hitCount == 2) {
        signedDist.x = xDist;
        signedDist.y = yDist;
        signedDist.z = zDist;
        pHitNormal->zero();

        if (hitX) {
            if (xDist < 0.0f) {
                signedDist.x = -xExtent;
                pHitNormal->sub(xDir);
            }
            else {
                signedDist.x = xExtent;
                pHitNormal->add(xDir);
            }
        }

        if (hitY) {
            if (yDist < 0.0f) {
                signedDist.y = -yExtent;
                pHitNormal->sub(yDir);
            }
            else {
                signedDist.y = yExtent;
                pHitNormal->add(yDir);
            }
        }

        if (hitZ) {
            if (zDist < 0.0f) {
                signedDist.z = -zExtent;
                pHitNormal->sub(zDir);
            }
            else {
                signedDist.z = zExtent;
                pHitNormal->add(zDir);
            }
        }
    }

    if (hitCount == 1) {
        if (hitX) {
            if (xDist >= 0.0f) {
                pHitNormal->set(xDir);
                signedDist.x = xExtent;
                signedDist.y = yDist;
                signedDist.z = zDist;
                surface = 0;
            }
            else {
                TVec3f normal(-xDir);
                pHitNormal->set(normal);
                signedDist.x = -xExtent;
                signedDist.y = yDist;
                signedDist.z = zDist;
                surface = 1;
            }
        }

        if (hitY) {
            if (yDist >= 0.0f) {
                pHitNormal->set(yDir);
                signedDist.x = xDist;
                signedDist.y = yExtent;
                signedDist.z = zDist;
                surface = 2;
            }
            else {
                TVec3f normal(-yDir);
                pHitNormal->set(normal);
                signedDist.x = xDist;
                signedDist.y = -yExtent;
                signedDist.z = zDist;
                surface = 3;
            }
        }

        if (hitZ) {
            if (zDist >= 0.0f) {
                pHitNormal->set(zDir);
                signedDist.x = xDist;
                signedDist.y = yDist;
                signedDist.z = zExtent;
                surface = 4;
            }
            else {
                TVec3f normal(-zDir);
                pHitNormal->set(normal);
                signedDist.x = xDist;
                signedDist.y = yDist;
                signedDist.z = -zExtent;
                surface = 5;
            }
        }
    }

    TVec3f zPart(zDir);
    zPart.scale(signedDist.z);

    TVec3f yPart(yDir);
    yPart.scale(signedDist.y);

    TVec3f xPart(xDir);
    xPart.scale(signedDist.x);

    TVec3f localPoint(xPart);
    localPoint.add(yPart);

    TVec3f worldPoint(localPoint);
    worldPoint.add(zPart);

    TVec3f hitPoint(worldPoint);
    hitPoint.add(_44);
    pHitPos->set(hitPoint);

    MR::normalizeOrZero(pHitNormal);

    if (mPolygon != nullptr) {
        if (_60 & (1 << surface)) {
            mPolygon->setSurface(surface);

            if (surface != -1) {
                mPolygon->syncCollision();
            }
        }
    }

    return true;
}
