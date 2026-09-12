#include "Game/Map/NamePosHolder.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Util/JMapLinkInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/MapUtil.hpp"
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

}  // namespace MR
