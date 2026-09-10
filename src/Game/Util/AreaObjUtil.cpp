#include "Game/Util/AreaObjUtil.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/RestartCube.hpp"
#include "Game/AreaObj/WaterArea.hpp"
#include "Game/Map/WaterAreaHolder.hpp"
#include "Game/Map/WaterInfo.hpp"
#include "Game/Map/OceanBowl.hpp"
#include "Game/Map/OceanRing.hpp"
#include "Game/Map/OceanSphere.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"
#include "Game/Util/PlayerUtil.hpp"

namespace MR {

    inline AreaObj* getAreaIn(const char* pName, const TVec3f& rPos) {
        return getAreaObjContainer()->getAreaObj(pName, rPos);
    }

    AreaObjMgr* getAreaObjManager(const char* pMgrName) {
        return MR::getAreaObjContainer()->getManager(pMgrName);
    }

    AreaObj* getAreaObj(const char* pAreaName, const TVec3f& rVec) {
        return MR::getAreaObjContainer()->getAreaObj(pAreaName, rVec);
    }

    bool isInAreaObj(const char* pAreaName, const TVec3f& rVec) {
        return MR::getAreaObjContainer()->getAreaObj(pAreaName, rVec);
    }

    s32 getAreaObjArg(const AreaObj* pObj, s32 which) {
        switch (which) {
        case 0:
            return pObj->mObjArg0;
        case 1:
            return pObj->mObjArg1;
        case 2:
            return pObj->mObjArg2;
        case 3:
            return pObj->mObjArg3;
        case 4:
            return pObj->mObjArg4;
        case 5:
            return pObj->mObjArg5;
        case 6:
            return pObj->mObjArg6;
        case 7:
            return pObj->mObjArg7;
        default:
            return -1;
        }
    }

    bool calcAreaMoveVelocity(TVec3f* pVelocity, const TVec3f& rPos) {
        AreaObj* area = getAreaIn("AreaMoveSphere", rPos);
        if (area == nullptr) {
            pVelocity->zero();
            return false;
        }

        AreaFormSphere* form = static_cast< AreaFormSphere* >(area->mForm);
        TVec3f center;
        form->calcPos(&center);
        TVec3f up;
        form->calcUpVec(&up);
        TVec3f direction = center;
        direction -= rPos;
        normalizeOrZero(&direction);
        vecKillElement(up, direction, &up);
        normalizeOrZero(&up);
        s32 speed = getAreaObjArg(area, 0);
        if (speed == -1) {
            speed = 10;
        }
        pVelocity->set(up * speed);
        return true;
    }

    AreaObj* getCurrentAstroOverlookAreaObj() {
        return getAreaIn("AstroOverlookArea", *MR::getPlayerPos());
    }

    bool getWaterAreaObj(WaterInfo* pInfo, const TVec3f& rPos) {
        pInfo->clear();
        WaterArea* pArea = static_cast< WaterArea* >(getAreaIn("Water", rPos));
        if (pArea != nullptr) {
            pInfo->mWaterArea = pArea;
            return true;
        }

        return WaterAreaFunction::tryInOceanArea(rPos, pInfo);
    }

    bool getWaterAreaInfo(WaterInfo* pInfo, const TVec3f& rPos, const TVec3f& rGravity, bool isSecondArea) {
        if (pInfo->mOceanBowl != nullptr) {
            return pInfo->mOceanBowl->calcWaterInfo(rPos, rGravity, pInfo);
        }
        if (pInfo->mOceanRing != nullptr) {
            return pInfo->mOceanRing->calcWaterInfo(rPos, rGravity, pInfo);
        }
        if (pInfo->mOceanSphere != nullptr) {
            return pInfo->mOceanSphere->calcWaterInfo(rPos, rGravity, pInfo);
        }

        const WaterArea* area = pInfo->mWaterArea;
        switch (area->mFormType) {
        case AreaForm::Type_Cube1:
        case AreaForm::Type_Cube2: {
            AreaFormCube* form = static_cast< AreaFormCube* >(area->mForm);
            TVec3f localPos;
            form->calcLocalPos(&localPos, rPos);
            TVec3f up;
            up.set(0.0f, 1.0f, 0.0f);
            TVec3f top(0.0f, form->mBounding.f.y, 0.0f);
            TVec3f fromTop(localPos);
            fromTop -= top;
            TVec3f fromBottom(localPos);
            pInfo->mCamWaterDepth = __fabsf(MR::vecKillElement(fromTop, up, &fromTop));
            pInfo->_4 = __fabsf(MR::vecKillElement(fromBottom, up, &fromBottom));
            pInfo->mSurfacePos.set(top + fromTop);
            TPos3f matrix;
            form->calcWorldMtx(&matrix);
            PSMTXMultVec(matrix.toMtxPtr(), &pInfo->mSurfacePos, &pInfo->mSurfacePos);
            TVec3f rotation;
            form->calcWorldRotate(&rotation);
            TPos3f rotationMatrix;
            MR::makeMtxRotate(rotationMatrix.toMtxPtr(), rotation.x, rotation.y, rotation.z);
            rotationMatrix.getYDir(up);
            pInfo->mSurfaceNormal.set(up);
            if (area->mObjArg0 > 0) {
                TVec3f flow(0.0f, 0.0f, 1.0f);
                MR::calcCubeAxisZ(area, &flow);
                pInfo->mStreamVec.set(flow);
                pInfo->mStreamVec.scale(area->mObjArg0);
            }
            break;
        }
        case AreaForm::Type_Sphere: {
            AreaFormSphere* form = static_cast< AreaFormSphere* >(area->mForm);
            TVec3f center(0.0f, 0.0f, 0.0f);
            form->calcPos(&center);
            f32 radius = form->mRadius;
            TVec3f radial(rPos);
            radial -= center;
            f32 height = MR::vecKillElement(radial, -rGravity, &radial);
            f32 halfHeight = radius * JMACosRadian((PSVECMag(&radial) / radius) * PI * 0.5f);
            pInfo->mCamWaterDepth = halfHeight - height;
            pInfo->_4 = halfHeight + height;
            TVec3f normal(rPos);
            normal -= center;
            MR::normalizeOrZero(&normal);
            pInfo->mSurfaceNormal.set(normal);
            pInfo->mSurfacePos.set(center + normal * radius);
            break;
        }
        case AreaForm::Type_Cylinder: {
            AreaFormCylinder* form = static_cast< AreaFormCylinder* >(area->mForm);
            TVec3f center(0.0f, 0.0f, 0.0f);
            TVec3f up(0.0f, 0.0f, 1.0f);
            form->calcPos(&center);
            form->calcUpVec(&up);
            if (__fabsf(up.dot(rGravity)) > 0.707f) {
                TVec3f radial;
                pInfo->_4 = MR::vecKillElement(rPos - center, up, &radial);
                pInfo->mCamWaterDepth = form->mHeight - pInfo->_4;
                pInfo->mSurfacePos.set(rPos + up * pInfo->mCamWaterDepth);
            } else {
                TVec3f radial;
                MR::vecKillElement(rPos - center, up, &radial);
                f32 distance = PSVECMag(&radial);
                pInfo->mCamWaterDepth = form->mRadius - distance;
                pInfo->_4 = form->mRadius + distance;
                pInfo->mSurfacePos.set(rPos - rGravity * pInfo->mCamWaterDepth);
            }
            pInfo->mSurfaceNormal.set(-rGravity);
            if (area->mObjArg0 > 0) {
                TVec3f flow(0.0f, 0.0f, 0.0f);
                form->calcUpVec(&flow);
                pInfo->mStreamVec.set(flow);
                pInfo->mStreamVec.scale(area->mObjArg0);
            }
            break;
        }
        default:
            break;
        }

        if (!isSecondArea) {
            TVec3f checkPos(pInfo->mSurfacePos);
            checkPos -= rGravity * 5.0f;
            WaterInfo nextInfo;
            MR::getWaterAreaObj(&nextInfo, checkPos);
            if (nextInfo.isInWater()) {
                MR::getWaterAreaInfo(&nextInfo, checkPos, rGravity, true);
                pInfo->mCamWaterDepth += nextInfo.mCamWaterDepth;
                pInfo->mSurfacePos -= rGravity * nextInfo.mCamWaterDepth;
            }
        }
        return area->isInVolume(rPos);
    }

    bool calcWhirlPoolAccelInfo(const TVec3f& rPos, TVec3f* pVelocity) {
        return WaterAreaFunction::tryInWhirlPoolAccelerator(rPos, pVelocity);
    }

    void calcCubePos(const AreaObj* pAreaObj, TVec3f* pPos) {
        static_cast< AreaFormCube* >(pAreaObj->mForm)->calcWorldPos(pPos);
    }

    void calcCubeRotate(const AreaObj* pAreaObj, TVec3f* pRotate) {
        static_cast< AreaFormCube* >(pAreaObj->mForm)->calcWorldRotate(pRotate);
    }

    void calcSpherePos(TVec3f* pPos, const AreaObj* pAreaObj) {
        static_cast< AreaFormSphere* >(pAreaObj->mForm)->calcPos(pPos);
    }

    f32 getSphereRadius(const AreaObj* pAreaObj) {
        return static_cast< AreaFormSphere* >(pAreaObj->mForm)->mRadius;
    }

    void calcCylinderCenterPos(TVec3f* pPos, const AreaObj* pAreaObj) {
        static_cast< AreaFormCylinder* >(pAreaObj->mForm)->calcCenterPos(pPos);
    }

    void calcCubeAxisZ(const AreaObj* pArea, TVec3f* pPos) {
        TVec3f rotate;
        pArea->getForm< AreaFormCube >()->calcWorldRotate(&rotate);
        TRot3f rotation;
        MR::makeMtxRotate(rotation, rotate.x, rotate.y, rotate.z);
        rotation.getZDir2(*pPos);
    }

    void calcCubeWorldBox(TDirBox3f* pBox, const AreaObj* pArea) {
        pArea->getForm< AreaFormCube >()->calcWorldBox(pBox);
    }

    TBox3f* getCubeLocalBox(const AreaObj* pArea) {
        return &pArea->getForm< AreaFormCube >()->mBounding;
    }

    void calcCubeLocalPos(TVec3f* pVec, const AreaObj* pArea, const TVec3f& rVec) {
        pArea->getForm< AreaFormCube >()->calcLocalPos(pVec, rVec);
    }

    void calcCylinderPos(TVec3f* pVec, const AreaObj* pArea) {
        pArea->getForm< AreaFormCylinder >()->calcPos(pVec);
    }

    void calcCylinderUpVec(TVec3f* pVec, const AreaObj* pArea) {
        pArea->getForm< AreaFormCylinder >()->calcUpVec(pVec);
    }

    f32 getCylinderRadius(const AreaObj* pArea) {
        return pArea->getForm< AreaFormCylinder >()->mRadius;
    }

    void tryToUpdatePlayerRestartIdInfo(const TVec3f& rVec) {
        RestartCube* pCube = MR::getAreaObj< RestartCube >("RestartCube", rVec);
        if (pCube != nullptr) {
            pCube->updatePlayerRestartIdInfo();
        }
    }

};  // namespace MR
