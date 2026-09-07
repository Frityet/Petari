#include "Game/Map/NamePosHolder.hpp"
#include "Game/Util/JMapLinkInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/StageResourceBinding.hpp"

namespace MR {
    s32 getGeneralPosNum() {
        return smgpc::compat::require_stage_resources().general_position_count();
    }

    void getGeneralPosData(const char** ppName, TVec3f* pPos, TVec3f* pRot, JMapLinkInfo** ppLinkInfo, int index) {
        JMapInfoIter iter = smgpc::compat::require_stage_resources().general_position_iter(index);
        iter.getValue("PosName", ppName);
        getJMapInfoTrans(iter, pPos);
        getJMapInfoRotate(iter, pRot);
        *ppLinkInfo = new JMapLinkInfo(iter, false);
    }

    bool tryRegisterNamePosLinkObj(const NameObj* pObj, const JMapInfoIter& rIter) {
        return MR::getNamePosHolder()->tryRegisterLinkObj(pObj, rIter);
    }

    bool findNamePos(const char* pName, MtxPtr pMtx) {
        return MR::tryFindLinkNamePos(nullptr, pName, pMtx);
    }

    bool findNamePos(const char* pName, TVec3f* a2, TVec3f* a3) {
        return getNamePosHolder()->find(nullptr, pName, a2, a3);
    }

    bool tryFindNamePos(const char* pName, MtxPtr pMtx) {
        return tryFindLinkNamePos(nullptr, pName, pMtx);
    }

    bool tryFindNamePos(const char* pName, TVec3f* pParam2, TVec3f* pParam3) {
        return tryFindLinkNamePos(nullptr, pName, pParam2, pParam3);
    }

    void findLinkNamePos(const NameObj* pObj, const char* pName, MtxPtr pMtx) {
        tryFindLinkNamePos(pObj, pName, pMtx);
    }

    bool tryFindLinkNamePos(const NameObj* pObj, const char* pName, MtxPtr pMtx) {
        TVec3f pos(0.0f, 0.0f, 0.0f);
        TVec3f rot(0.0f, 0.0f, 0.0f);
        if (getNamePosHolder()->find(pObj, pName, &pos, &rot)) {
            makeMtxTR(pMtx, pos, rot);
            return true;
        }
        return false;
    }

    bool tryFindLinkNamePos(const NameObj* pObj, const char* pName, TVec3f* pPos, TVec3f* pRot) {
        return getNamePosHolder()->find(pObj, pName, pPos, pRot);
    }
}  // namespace MR
