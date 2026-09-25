#include "resource/TextEncoding.hpp"
#include "Game/AreaObj/CollisionArea.hpp"
#include "Game/AreaObj/AreaForm.hpp"
#include "Game/MapObj/DynamicCollisionObj.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "JSystem/JGeometry/TVec.hpp"

DynamicCollisionObj::~DynamicCollisionObj() {
}

AreaPolygon::AreaPolygon() : DynamicCollisionObj(CP932("エリアポリゴン")) {
    mForm = nullptr;
    _128 = nullptr;
    _12C.zero();
}

AreaPolygon::~AreaPolygon() {
}

void AreaPolygon::init(const JMapInfoIter& rIter) {
    if (MR::isValidInfo(rIter)) {
        MR::initDefaultPos(this, rIter);
    } else {
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

    mPositions = new TVec3f[mPositionNum];
    setSurface(0);
    initHitSensor(1);

    TVec3f sensorOffset;
    sensorOffset.x = 0.0f;
    sensorOffset.y = 0.0f;
    sensorOffset.z = 0.0f;
    MR::addHitSensorEye(this, "body", 8, 1.0f, sensorOffset);

    mIndices = new TriangleIndexing[_94];
    _9C = new TVec3f[_94];

    TriangleIndexing& first = mIndices[0];
    first.mIndex[0] = 0;
    first.mIndex[1] = 1;
    first.mIndex[2] = 2;

    TriangleIndexing& second = mIndices[1];
    second.mIndex[0] = 0;
    second.mIndex[1] = 2;
    second.mIndex[2] = 3;

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
        static_cast< AreaFormCube* >(mForm)->calcWorldMtx(&worldMtx);
        static_cast< AreaFormCube* >(mForm)->calcWorldPos(&mPosition);
    } else {
        PSMTXCopy(_128, worldMtx);
        MR::extractMtxTrans(_128, &mPosition);
    }

    TVec3f axisX;
    TVec3f axisY;
    TVec3f axisZ;
    worldMtx.getXDir(axisX);
    worldMtx.getYDir(axisY);
    worldMtx.getZDir(axisZ);
    if (_128 != nullptr) {
        mPosition.add(axisY * _138);
    }

    f32 sizeX = PSVECMag(&axisX);
    MR::normalizeOrZero(&axisX);
    f32 sizeY = PSVECMag(&axisY);
    MR::normalizeOrZero(&axisY);
    f32 sizeZ = PSVECMag(&axisZ);
    MR::normalizeOrZero(&axisZ);
    if (mForm != nullptr) {
        sizeX *= 0.5f * (static_cast< AreaFormCube* >(mForm)->mScale.x * static_cast< AreaFormCube* >(mForm)->getBaseSize());
        sizeY *= 0.5f * (static_cast< AreaFormCube* >(mForm)->mScale.y * static_cast< AreaFormCube* >(mForm)->getBaseSize());
        sizeZ *= 0.5f * (static_cast< AreaFormCube* >(mForm)->mScale.z * static_cast< AreaFormCube* >(mForm)->getBaseSize());
    } else {
        sizeX *= 0.5f * _12C.x;
        sizeY *= 0.5f * _12C.y;
        sizeZ *= 0.5f * _12C.z;
    }

    if (!MR::isPlayerElementModeTeresa()) {
        if (sizeX < 0.0f) {
            sizeX -= 10.0f;
        } else {
            sizeX += 10.0f;
        }
        if (sizeY < 0.0f) {
            sizeY -= 10.0f;
        } else {
            sizeY += 10.0f;
        }
        if (sizeZ < 0.0f) {
            sizeZ -= 10.0f;
        } else {
            sizeZ += 10.0f;
        }
    }

    switch (surface) {
    case 0:
        mPositions[0].set(sizeX, -sizeY, -sizeZ);
        mPositions[1].set(sizeX, sizeY, -sizeZ);
        mPositions[2].set(sizeX, sizeY, sizeZ);
        mPositions[3].set(sizeX, -sizeY, sizeZ);
        break;
    case 1:
        mPositions[0].set(-sizeX, -sizeY, sizeZ);
        mPositions[1].set(-sizeX, sizeY, sizeZ);
        mPositions[2].set(-sizeX, sizeY, -sizeZ);
        mPositions[3].set(-sizeX, -sizeY, -sizeZ);
        break;
    case 2:
        mPositions[0].set(-sizeX, sizeY, -sizeZ);
        mPositions[1].set(-sizeX, sizeY, sizeZ);
        mPositions[2].set(sizeX, sizeY, sizeZ);
        mPositions[3].set(sizeX, sizeY, -sizeZ);
        break;
    case 3:
        mPositions[0].set(-sizeX, -sizeY, sizeZ);
        mPositions[1].set(-sizeX, -sizeY, -sizeZ);
        mPositions[2].set(sizeX, -sizeY, -sizeZ);
        mPositions[3].set(sizeX, -sizeY, sizeZ);
        break;
    case 4:
        mPositions[0].set(-sizeX, -sizeY, sizeZ);
        mPositions[1].set(sizeX, -sizeY, sizeZ);
        mPositions[2].set(sizeX, sizeY, sizeZ);
        mPositions[3].set(-sizeX, sizeY, sizeZ);
        break;
    case 5:
        mPositions[0].set(-sizeX, sizeY, -sizeZ);
        mPositions[1].set(sizeX, sizeY, -sizeZ);
        mPositions[2].set(sizeX, -sizeY, -sizeZ);
        mPositions[3].set(-sizeX, -sizeY, -sizeZ);
        break;
    }

    for (u32 i = 0; i < 4; i++) {
        mPositions[i].set(axisX * mPositions[i].x + axisY * mPositions[i].y + axisZ * mPositions[i].z);
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

CollisionArea::~CollisionArea() {
}

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
                    tStack84 = playerPos - tStack60;
                    tStack84.setLength(dVar4);

                    tStack84 = tStack60 + tStack84 - playerPos;
                }

                if (tStack84.dot(tStack72) > 0) {
                    MR::pushPlayerFromArea(tStack84);
                }
            }
        }
    }
}

bool CollisionArea::hitCheck(const TVec3f& rPosition, f32 radius, TVec3f* pContact, TVec3f* pNormal) {
    AreaFormCube* form = static_cast< AreaFormCube* >(mForm);
    s32 surface = -1;
    TPos3f worldMtx;
    form->calcWorldMtx(&worldMtx);
    form->calcWorldPos(&_44);

    TVec3f localContact;
    TVec3f axisX;
    TVec3f axisY;
    TVec3f axisZ;
    worldMtx.getXDir(axisX);
    worldMtx.getYDir(axisY);
    worldMtx.getZDir(axisZ);
    f32 sizeX = PSVECMag(&axisX);
    MR::normalizeOrZero(&axisX);
    f32 sizeY = PSVECMag(&axisY);
    MR::normalizeOrZero(&axisY);
    f32 sizeZ = PSVECMag(&axisZ);
    MR::normalizeOrZero(&axisZ);
    sizeX *= 0.5f * (form->mScale.x * form->getBaseSize());
    sizeY *= 0.5f * (form->mScale.y * form->getBaseSize());
    sizeZ *= 0.5f * (form->mScale.z * form->getBaseSize());

    TVec3f bounds(sizeX + radius, sizeY + radius, sizeZ + radius);
    TVec3f relativePosition(rPosition - _44);
    f32 localX = relativePosition.dot(axisX);
    f32 localY = relativePosition.dot(axisY);
    f32 localZ = relativePosition.dot(axisZ);
    TVec3f distance;
    distance.set< f32 >(MR::abs(localX), MR::abs(localY), MR::abs(localZ));
    if (distance.x >= bounds.x || distance.y >= bounds.y || distance.z >= bounds.z) {
        return false;
    }

    u32 outsideCount = 0;
    bool outsideX = false;
    bool outsideY = false;
    bool outsideZ = false;
    if (distance.x >= sizeX) {
        outsideX = true;
        outsideCount++;
    }
    if (distance.y >= sizeY) {
        outsideY = true;
        outsideCount++;
    }
    if (distance.z >= sizeZ) {
        outsideZ = true;
        outsideCount++;
    }
    _3C = outsideCount;

    if (outsideCount == 3) {
        TVec3f corner;
        corner.zero();
        if (localX < 0.0f) {
            corner.add(-axisX * sizeX);
        } else {
            corner.add(axisX * sizeX);
        }
        if (localY < 0.0f) {
            corner.add(-axisY * sizeY);
        } else {
            corner.add(axisY * sizeY);
        }
        if (localZ < 0.0f) {
            corner.add(-axisZ * sizeZ);
        } else {
            corner.add(axisZ * sizeZ);
        }
        TVec3f difference(corner - rPosition);
        if (PSVECMag(&difference) >= radius) {
            return false;
        }
        pContact->set(corner);
        pNormal->set(axisX + axisY + axisZ);
        MR::normalizeOrZero(pNormal);
        return true;
    }

    if (outsideCount == 0) {
        TVec3f penetration(sizeX - distance.x, sizeY - distance.y, sizeZ - distance.z);
        if (penetration.x < penetration.y && penetration.x < penetration.z) {
            _40 = penetration.x;
            outsideX = true;
        } else if (penetration.y < penetration.x && penetration.y < penetration.z) {
            _40 = penetration.y;
            outsideY = true;
        } else {
            _40 = penetration.z;
            outsideZ = true;
        }
        outsideCount = 1;
    }

    if (outsideCount == 2) {
        localContact.set(localX, localY, localZ);
        pNormal->zero();
        if (outsideX) {
            if (localX < 0.0f) {
                localContact.x = -sizeX;
                pNormal->sub(axisX);
            } else {
                localContact.x = sizeX;
                pNormal->add(axisX);
            }
        }
        if (outsideY) {
            if (localY < 0.0f) {
                localContact.y = -sizeY;
                pNormal->sub(axisY);
            } else {
                localContact.y = sizeY;
                pNormal->add(axisY);
            }
        }
        if (outsideZ) {
            if (localZ < 0.0f) {
                localContact.z = -sizeZ;
                pNormal->sub(axisZ);
            } else {
                localContact.z = sizeZ;
                pNormal->add(axisZ);
            }
        }
    }

    if (outsideCount == 1) {
        if (outsideX) {
            if (localX >= 0.0f) {
                pNormal->set(axisX);
                localContact.set(sizeX, localY, localZ);
                surface = 0;
            } else {
                pNormal->set(-axisX);
                localContact.set(-sizeX, localY, localZ);
                surface = 1;
            }
        }
        if (outsideY) {
            if (localY >= 0.0f) {
                pNormal->set(axisY);
                localContact.set(localX, sizeY, localZ);
                surface = 2;
            } else {
                pNormal->set(-axisY);
                localContact.set(localX, -sizeY, localZ);
                surface = 3;
            }
        }
        if (outsideZ) {
            if (localZ >= 0.0f) {
                pNormal->set(axisZ);
                localContact.set(localX, localY, sizeZ);
                surface = 4;
            } else {
                pNormal->set(-axisZ);
                localContact.set(localX, localY, -sizeZ);
                surface = 5;
            }
        }
    }

    pContact->set(axisX * localContact.x + axisY * localContact.y + axisZ * localContact.z + _44);
    MR::normalizeOrZero(pNormal);
    // The retail shift produces zero for the -1 edge-contact sentinel.
    if (mPolygon != nullptr && surface >= 0 && (_60 & (1U << surface))) {
        mPolygon->setSurfaceAndSync(surface);
    }
    return true;
}
