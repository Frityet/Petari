#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/Map/NamePosHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/Util/JMapLinkInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <cmath>
#include <cstring>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
bool equal(const TVec3f& a, const TVec3f& b) {
    return std::abs(a.x-b.x) < 0.001f && std::abs(a.y-b.y) < 0.001f && std::abs(a.z-b.z) < 0.001f;
}
const NamePosHolder* retired_identity = nullptr;
void verify() {
    auto* stage = MR::getStageDataHolder();
    const auto domain = MR::getSceneObjHolder()->nativeAllocationDomain();
    require(stage && domain, "Actual GameScene owns the original stage tables and heap");
    NamePosHolder* owner;
    {
        const smgpc::compat::JkrAllocationScope game(domain);
        owner = static_cast<NamePosHolder*>(MR::createSceneObj(SceneObj_NamePosHolder));
    }
    require(owner && owner == MR::getNamePosHolder() && owner->mPosNum == stage->getGeneralPosNum() && owner->mPosNum > 0,
            "Original NamePosHolder contains every retail general-position row");
    require(JKRHeap::findFromRoot(owner->mInfos) == &domain->heap(), "Original position records belong to the actual scene heap");
    retired_identity = owner;
    for (s32 index = 0; index < owner->mPosNum; ++index) {
        const auto iter = stage->getGeneralPosInfoFromDataIndex(index);
        const auto& actual = owner->mInfos[index];
        const char* name = nullptr;
        TVec3f position, rotation;
        require(iter.getValue("PosName", &name), "Every original position row exposes its authored name");
        MR::getJMapInfoTrans(iter, &position);
        MR::getJMapInfoRotate(iter, &rotation);
        const JMapLinkInfo link(iter, false);
        require(std::strcmp(actual.mName, name) == 0 && equal(actual.mPosition, position) && equal(actual.mRotation, rotation),
                "Original NamePos records preserve table order and apply authored zone transforms exactly once");
        require(actual.mLinkInfo && actual.mLinkInfo->_0 == link._0 && actual.mLinkInfo->_4 == link._4 && actual.mLinkInfo->_8 == link._8,
                "Each original position retains its exact authored zone and object link fields");
        require(JKRHeap::findFromRoot(actual.mLinkInfo) == &domain->heap(), "Original link records retain scene heap ownership");
        // Named lookups select the first matching row, including duplicate names.
        s32 first = 0;
        while (std::strcmp(owner->mInfos[first].mName, name) != 0) ++first;
        require(MR::tryFindNamePos(name, &position, &rotation) && equal(position, owner->mInfos[first].mPosition) &&
                    equal(rotation, owner->mInfos[first].mRotation), "Original named lookup retains first-row precedence");
    }
    TVec3f position(101, 102, 103), rotation(201, 202, 203);
    require(!MR::tryFindNamePos("Missing resource diagnostic position", &position, &rotation) && position.x == 101 && rotation.x == 201,
            "Missing original position lookup leaves outputs unchanged");
    std::fprintf(stderr, "[name-pos] checked %d original authored rows\n", owner->mPosNum);
}
}
int main() {
    const int result = smgpc::test::run_stage_resource_process("original-name-pos", verify);
    if (result == 0 && smgpc::compat::has_name_obj_runtime_state(retired_identity)) {
        std::fprintf(stderr, "FAIL original NamePosHolder identity survived normal scene retirement\n");
        return 1;
    }
    return result;
}
