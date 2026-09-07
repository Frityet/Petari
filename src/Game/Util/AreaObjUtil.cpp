#include "Game/Util/AreaObjUtil.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/WaterArea.hpp"
#include "Game/Map/WaterAreaHolder.hpp"
#include "Game/Map/WaterInfo.hpp"
#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/RestartCube.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

namespace MR {
    f32 getSphereRadius(const AreaObj* pAreaObj) {
        return static_cast< AreaFormSphere* >(pAreaObj->mForm)->_14;
    }

    void calcSpherePos(TVec3f* pPos, const AreaObj* pAreaObj) {
        static_cast< AreaFormSphere* >(pAreaObj->mForm)->calcPos(pPos);
    }


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

    AreaObj* getCurrentAstroOverlookAreaObj() {
        return getAreaIn("AstroOverlookArea", *MR::getPlayerPos());
    }

    void calcCylinderPos(TVec3f* pPos, const AreaObj* pAreaObj) {
        static_cast< AreaFormCylinder* >(pAreaObj->mForm)->calcPos(pPos);
    }

    void calcCubeAxisZ(const AreaObj* pAreaObj, TVec3f* pAxis) {
        TVec3f rotation;
        static_cast< AreaFormCube* >(pAreaObj->mForm)->calcWorldRotate(&rotation);
        Mtx matrix;
        MR::makeMtxRotate(matrix, rotation.x, rotation.y, rotation.z);
        pAxis->set< f32 >(matrix[0][2], matrix[1][2], matrix[2][2]);
    }

    void tryToUpdatePlayerRestartIdInfo(const TVec3f& rPos) {
        RestartCube* cube = static_cast< RestartCube* >(getAreaIn("RestartCube", rPos));

        if (cube != nullptr) {
            cube->updatePlayerRestartIdInfo();
        }
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

    void calcCylinderCenterPos(TVec3f* pPos, const AreaObj* pAreaObj) {
        static_cast< AreaFormCylinder* >(pAreaObj->mForm)->calcCenterPos(pPos);
    }

};  // namespace MR
