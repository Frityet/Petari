#include "Game/Map/HitInfo.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Player/J3DModelX.hpp"
#include "Game/Player/JetTurtleShadow.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioShadow.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Player/ModelHolder.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/FootPrint.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

void MarioActor::initShadow() {
    _A08 = 1;

    if (gIsLuigi) {
        _A10 = new ModelHolder("LuigiShadow", true);
    } else {
        _A10 = new ModelHolder("MarioShadow", true);
    }

    _A10->initWithoutIter();

    _A14 = reinterpret_cast< J3DModelX* >(MR::getJ3DModel(_A10));
    _A14->_DD = mModels[0]->_DD;

    for (u32 idx = 0; idx < _A14->_DD; idx++) {
        _A14->mExtraMtxBuffer[idx] = mModels[0]->mExtraMtxBuffer[idx];
    }
}

void MarioActor::calcViewReflectionModel() {
    J3DModelX* model = getJ3DModel();
    Mtx matrix;
    PSMTXCopy(getJ3DModel()->getBaseTRMtx(), matrix);
    Mtx offsetMatrix;
    PSMTXIdentity(offsetMatrix);
    TVec3f position;
    f32 offset = 10.0f;
    if (isAnimationRun("ターンジャンプ")) {
        offset = 25.0f;
    }
    if (_A08 != 6 && _A08 != 7) {
        _A24 = 0;
    }
    if (_A08 != 3) {
        _A25 = 0;
    }

    switch (_A08) {
    case 2: {
        matrix[0][1] = -matrix[0][1];
        matrix[1][1] = -matrix[1][1];
        matrix[2][1] = -matrix[2][1];
        position = mMario->mShadowPos - *mMario->_45C->getNormal(0) * offset;
        TVec3f jointPosition;
        getRealPos("All_Root", &jointPosition);
        TVec3f difference(mPosition);
        difference -= jointPosition;
        PSMTXTrans(offsetMatrix, difference.x, difference.y, difference.z);
        updateReflectAlphaDL(_A09);
        break;
    }
    case 3: {
        TVec3f xDir;
        xDir.set< f32 >(matrix[0][0], matrix[1][0], matrix[2][0]);
        TVec3f yDir;
        yDir.set< f32 >(matrix[0][1], matrix[1][1], matrix[2][1]);
        TVec3f zDir;
        zDir.set< f32 >(matrix[0][2], matrix[1][2], matrix[2][2]);
        const TVec3f& normal = mMario->mSwim->mSurfacePos;
        f32 xDot = MR::vecKillElement(xDir, normal, &xDir);
        f32 yDot = MR::vecKillElement(yDir, normal, &yDir);
        f32 zDot = MR::vecKillElement(zDir, normal, &zDir);
        xDir -= normal * xDot;
        yDir -= normal * yDot;
        zDir -= normal * zDot;
        matrix[0][0] = xDir.x;
        matrix[1][0] = xDir.y;
        matrix[2][0] = xDir.z;
        matrix[0][1] = yDir.x;
        matrix[1][1] = yDir.y;
        matrix[2][1] = yDir.z;
        matrix[0][2] = zDir.x;
        matrix[1][2] = zDir.y;
        matrix[2][2] = zDir.z;
        position = mMario->mSwim->mSurfaceNorm - normal * offset;
        f32 depth = -mMario->mSwim->mWaterDepth;
        if (depth < 40.0f) {
            offset = 0.0f;
        } else if (depth < 200.0f) {
            offset = (depth - 40.0f) / 160.0f;
        } else {
            offset = 1.0f;
        }
        f32 alpha = mCamDirZ.dot(normal);
        MR::clamp01(&alpha);
        alpha *= 1.414f;
        MR::clamp01(&alpha);
        alpha *= 64.0f;
        if (_A25 > static_cast< u8 >(offset * alpha)) {
            _A25--;
        } else if (_A25 < static_cast< u8 >(offset * alpha)) {
            _A25++;
        }
        updateSimpleAlphaDL(_A25);
        _B18 = position;
        break;
    }
    case 6:
    case 7: {
        TVec3f xDir;
        xDir.set< f32 >(matrix[0][0], matrix[1][0], matrix[2][0]);
        TVec3f yDir;
        yDir.set< f32 >(matrix[0][1], matrix[1][1], matrix[2][1]);
        TVec3f zDir;
        zDir.set< f32 >(matrix[0][2], matrix[1][2], matrix[2][2]);
        TVec3f normal(mMario->mSwim->_178);
        normal -= mPosition;
        MR::normalize(&normal);
        f32 xDot = MR::vecKillElement(xDir, normal, &xDir);
        f32 yDot = MR::vecKillElement(yDir, normal, &yDir);
        f32 zDot = MR::vecKillElement(zDir, normal, &zDir);
        xDir -= normal * xDot;
        yDir -= normal * yDot;
        zDir -= normal * zDot;
        matrix[0][0] = xDir.x;
        matrix[1][0] = xDir.y;
        matrix[2][0] = xDir.z;
        matrix[0][1] = yDir.x;
        matrix[1][1] = yDir.y;
        matrix[2][1] = yDir.z;
        matrix[0][2] = zDir.x;
        matrix[1][2] = zDir.y;
        matrix[2][2] = zDir.z;
        position = mMario->mSwim->_178 - normal * offset;
        f32 distance = mMario->mSwim->getWaterEdgeDist();
        offset = 0.0f;
        if (distance < 0.0f) {
        } else if (distance < 1000.0f) {
            offset = 0.5f + 0.5f * (distance / 1000.0f);
        }
        f32 alpha = mCamDirZ.dot(normal);
        MR::clamp01(&alpha);
        alpha *= 1.414f;
        MR::clamp01(&alpha);
        alpha *= 127.0f;
        if (_A24 > static_cast< u8 >(offset * alpha)) {
            _A24--;
        } else if (_A24 < static_cast< u8 >(offset * alpha)) {
            _A24++;
        }
        updateSimpleAlphaDL(_A24);
        _B18 = position;
        break;
    }
    }

    matrix[0][3] = position.x;
    matrix[1][3] = position.y;
    matrix[2][3] = position.z;
    PSMTXCopy(matrix, _E0C);
    if (getCarrySensor() != nullptr) {
        Mtx carryMatrix;
        PSMTXConcat(_BC8, MR::getJointMtx(getCarrySensor()->mHost, 0), carryMatrix);
        PSMTXConcat(_E0C, carryMatrix, _E0C);
        _9A0->calcType0(_E0C);
    }
    PSMTXConcat(_BC8, offsetMatrix, offsetMatrix);
    PSMTXConcat(matrix, offsetMatrix, matrix);
    PSMTXConcat(j3dSys.mViewMtx, matrix, j3dSys.mViewMtx);
    model->viewCalc3(3, nullptr);
    MR::loadViewMtx();
}

void MarioActor::calcViewWallShadowModel() {
    _A0C = 0;
    if ((_A08 & 2) || mBeeWallWalk) {
        return;
    }

    TVec3f direction;
    s32 distance = 0;
    const AreaObj* area = MR::getAreaObj("ExtraWallCheckArea", _2A0);
    if (area != nullptr) {
        MR::calcCubeAxisZ(area, &direction);
        distance = MR::getAreaObjArg(area, 0);
        _20C = area;
        _210 = 0;
    } else {
        area = MR::getAreaObj("ExtraWallCheckCylinder", _2A0);
        if (area != nullptr) {
            distance = calcCylinderToCenter(area, &direction);
            _20C = area;
            _210 = 1;
        }
    }

    bool leavingArea = false;
    if (area == nullptr || static_cast< f32 >(distance) < 0.0f) {
        if (_208 >= 1000.0f) {
            return;
        }
        _208 += 10.0f;
        if (_208 >= 1000.0f) {
            _208 = 1000.0f;
        }
        switch (_210) {
        case 0:
            MR::calcCubeAxisZ(_20C, &direction);
            distance = MR::getAreaObjArg(_20C, 0);
            break;
        case 1:
            distance = calcCylinderToCenter(_20C, &direction);
            break;
        }
        leavingArea = true;
    }

    Triangle triangle;
    TVec3f hitPosition;
    if (!MR::getFirstPolyOnLineToMap(&hitPosition, &triangle, _2A0, direction * distance)) {
        return;
    }
    _1F0 = hitPosition;
    _1FC = *triangle.getNormal(0);
    if (!leavingArea) {
        _208 = 0.9f * _208 + 0.1f * (hitPosition - _2A0).length();
    }
    updateRandomTexture(_208);

    TVec3f normalizedDirection(direction);
    MR::normalizeOrZero(&normalizedDirection);
    J3DModelX* model = getJ3DModel();
    J3DModelX* simpleModel = getSimpleModel();
    PSMTXCopy(model->getBaseTRMtx(), simpleModel->getBaseTRMtx());
    TPos3f* matrix = reinterpret_cast< TPos3f* >(simpleModel->getBaseTRMtx());
    TVec3f xDir;
    matrix->getXDir(xDir);
    MR::vecKillElement(xDir, normalizedDirection, &xDir);
    TVec3f yDir;
    matrix->getYDir(yDir);
    MR::vecKillElement(yDir, normalizedDirection, &yDir);
    TVec3f zDir;
    matrix->getZDir(zDir);
    MR::vecKillElement(zDir, normalizedDirection, &zDir);
    matrix->setXDir(xDir);
    matrix->setYDir(yDir);
    matrix->setZDir(zDir);

    TVec3f offset;
    MR::vecKillElement(mPosition - hitPosition, getGravityVec(), &offset);
    TVec3f position = mPosition + normalizedDirection * (10.0f + offset.length());
    MR::setMtxTrans(*matrix, position.x, position.y, position.z);
    PSMTXConcat(*matrix, _BC8, *matrix);
    simpleModel->viewCalcRef(3, model);
    _A0C = 1;
}

void MarioActor::drawShadow() const {
    bool hidden = _482 || _481;
    if (hidden || !(_A08 & 1) || mPlayerMode == 6) {
        return;
    }
    if (mBeeWallWalk && mMario->mMovementStates._1) {
        return;
    }
    if (mMario->isStatusActive(18) && _1A1) {
        return;
    }
    if (_924 != nullptr && selectNoShadow(_924)) {
        return;
    }
    if (mMario->mMovementStates._F && mMario->_544 > 1) {
        return;
    }

    J3DModelX* model = getSimpleModel();
    Mtx scale;
    PSMTXScale(scale, mScale.x, mScale.y, mScale.z);
    PSMTXConcat(scale, _BC8, scale);
    PSMTXConcat(getJ3DModel()->getBaseTRMtx(), scale, model->getBaseTRMtx());
    _214->calcView(model, 2, getJ3DModel());
    if (mMario->mSinkTimer > 180) {
        return;
    }
    if (mMario->mSinkTimer > 128) {
        MR::hideJointAndChildren(model, "Hip");
    }
    if (mMario->mSinkTimer > 150) {
        MR::hideJointAndChildren(model, "ShoulderL");
        MR::hideJointAndChildren(model, "ShoulderR");
    }
    if (_A58 || mMario->mSinkTimer > 120) {
        MR::hideJoint(model, "HandL0");
        MR::hideJoint(model, "HandR0");
    }
    _214->drawAndCaptureTex(model, mPosition);
    _214->clearAlphaBuffer();
    if (mBeeWallWalk) {
        _214->_307 = false;
    } else {
        _214->_307 = true;
    }
    _214->draw();
    MR::showJointAndChildren(model, "JointRoot");
}

void MarioActor::decideShadowMode() {
    if (mMario->mMovementStates._2 && !mMario->isNotReflectGlassGround() && !mMario->isStatusActive(18) &&
        (mMario->_962 == 14 || mMario->_962 == 33)) {
        _A08 = 2;
        f32 ratio = mMario->mVerticalSpeed / 500.0f;
        if (ratio >= 0.75f) {
            ratio = 0.75f;
        }
        u32 alpha = 64;
        u8 targetAlpha = alpha * (1.0f - ratio);
        for (u32 i = 0; i < 4; i++) {
            if (_A09 < targetAlpha) {
                _A09++;
            } else if (_A09 > targetAlpha) {
                _A09--;
            }
        }
    } else if (_A09) {
        _A09--;
        if (_A09) {
            _A09--;
        }
        if (_A09) {
            _A09--;
        }
        if (_A09) {
            _A09--;
        }
        if (_A09 == 0) {
            _A08 = 1;
        } else {
            _A08 = 2;
        }
    } else {
        _A08 = 1;
    }

    if (mMario->isSwimming() && MR::isInWater(mCamPos)) {
        if (_A25 || mCamDirZ.dot(-getGravityVector()) > 0.0f) {
            _A08 = 3;
        }
        if (_A24 || (mMario->mSwim->getWaterEdgeDist() > 0.0f && mMario->mSwim->getWaterEdgeDist() < 1000.0f)) {
            _A08 = 7;
        }
    }
    _214->setUpdateFlag();
}

void MarioActor::calcViewSilhouetteModel() {
    J3DModelX* model = getJ3DModel();
    J3DModelX* simpleModel = getSimpleModel();
    TVec3f direction(mPosition);
    direction -= mCamPos;
    MR::normalizeOrZero(&direction);

    PSMTXCopy(model->getBaseTRMtx(), simpleModel->getBaseTRMtx());
    TPos3f* matrix = reinterpret_cast< TPos3f* >(simpleModel->getBaseTRMtx());
    TVec3f xDir;
    matrix->getXDir(xDir);
    MR::vecKillElement(xDir, direction, &xDir);
    TVec3f yDir;
    matrix->getYDir(yDir);
    MR::vecKillElement(yDir, direction, &yDir);
    TVec3f zDir;
    matrix->getZDir(zDir);
    MR::vecKillElement(zDir, direction, &zDir);
    matrix->setXDir(xDir);
    matrix->setYDir(yDir);
    matrix->setZDir(zDir);

    TVec3f position;
    if (_EA4) {
        TVec3f jointPosition;
        getRealPos("All_Root", &jointPosition);
        position = jointPosition - direction * mConst->getTable()->mSilhouetteZoffset;
    } else {
        position = mPosition;
    }
    MR::setMtxTrans(*matrix, position.x, position.y, position.z);
    PSMTXConcat(*matrix, _BC8, *matrix);
    simpleModel->viewCalcRef(1, model);
}

f32 MarioActor::calcCylinderToCenter(const AreaObj* pAreaObj, TVec3f* pVec) {
    TVec3f upVec;
    MR::calcCylinderUpVec(&upVec, pAreaObj);

    TVec3f centerPos;
    MR::calcCylinderCenterPos(&centerPos, pAreaObj);

    MR::vecKillElement(_2A0 - centerPos, upVec, pVec);

    if (MR::normalizeOrZero(pVec)) {
        if (_208 >= 1000.0f) {
            return -1.0f;
        }

        _208 += 10.0f;

        if (_208 >= 1000.0f) {
            _208 = 1000.0f;
        }

        return -1.0f;
    }

    return MR::getAreaObjArg(pAreaObj, 0);
}

void MarioActor::calcViewBlurModel() {
    J3DModelX* model = getJ3DModel();
    if (model->_1E5) {
        if (_A6E == 1) {
            _A6E = 2;
        }
        model->_1E5 = 0;
    }
    if (MR::isDemoActive()) {
        if (_A6E) {
            _A6E = 5;
        }
        return;
    }
    if (_1C1 || !_A6E) {
        return;
    }
    if (_A6E >= 3) {
        _A6E--;
        return;
    }
    if (_A6E == 2) {
        for (u32 i = 0; i < 8; i++) {
            for (u32 j = 0; j < getModelData()->getJointNum(); j++) {
                PSMTXCopy(*model->getDrawMtx(j), _A70[i][j]);
                PSMTXCopy(*model->getDrawMtx(j), _A70[i + 8][j]);
            }
        }
        _B12 = 0;
        _A6E = 1;
    } else if (_37C % 3 == 0) {
        _B12 = (_B12 + 1) & 7;
        for (u32 j = 0; j < getModelData()->getJointNum(); j++) {
            PSMTXCopy(*model->getDrawMtx(j), _A70[(1 - _B10) * 8 + _B12][j]);
        }
    }
    _B10 = 1 - _B10;
    model->_1E4 = 1;
}

void MarioActor::calcViewFootPrint() {
    if (!_934 && mMario->mMovementStates._1 &&
        (mMario->mTargetWalkSpeedIndex || mMario->_1C.mIsUnderwater || mMario->mWalkSpeed > 0.1f || mMario->isPlayerModeHopper()) &&
        !mMario->mSinkTimer && !mMario->mDrawStates._9 && !mMario->mDrawStates._A && mMario->_1C._13 && mMario->_1C._14 &&
        (mMario->checkCurrentFloorCodeSevere(13) || mMario->checkCurrentFloorCodeSevere(26))) {
        _B48->addPrint(mPosition, mMario->mFrontVec, mMario->_368, false);
    }

    if (_B48->isValid(_37C) && MR::isInWater(*_B48->getPrintPos(_37C))) {
        _B48->invalidate(_37C);
    }
}

void MarioActor::drawSilhouette() const {
    bool hidden = _482 || _483 || _481;
    if (hidden || mMario->isStatusActive(26)) {
        return;
    }
    if (mMario->isStatusActive(27)) {
        if (!MR::isExistMapCollision(mCamPos, mPosition - mCamPos)) {
            return;
        }
        if (!MR::isExistMapCollision(mCamPos, _2AC - mCamPos)) {
            return;
        }
    }
    if (mMario->mSinkTimer || mPlayerMode == 6) {
        return;
    }
    if (mMario->isStatusActive(18) && _1A1) {
        return;
    }
    if (_EA4 || _934) {
        return;
    }
    J3DModelX* model = getSimpleModel();
    model->setDrawView(1);
    model->mFlags.clear();
    model->mFlags._1C = true;
    model->directDraw(nullptr);
    model->mFlags._1C = false;
    GXSetAlphaUpdate(GX_TRUE);
    GXSetColorUpdate(GX_TRUE);
    GXSetDstAlpha(GX_TRUE, 0);
}
