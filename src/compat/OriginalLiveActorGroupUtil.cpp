#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LiveActorGroupArray.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"

// Original LiveActorUtil group wrappers; the complete group implementation
// is compiled from Game/LiveActor/LiveActorGroupArray.cpp.
namespace {
    bool isShowModel(LiveActor* pActor) NO_INLINE {
        return !pActor->mFlag.mIsHiddenModel;
    }

    template < typename T >
    s32 countGroupMember(const LiveActor* pActor, T pPred) NO_INLINE;

    template <>
    s32 countGroupMember< bool (*)(LiveActor*) >(const LiveActor* pActor, bool (*pPred)(LiveActor*)) NO_INLINE {
        LiveActorGroupArray* pGroupArray = MR::getSceneObj< LiveActorGroupArray >(SceneObj_LiveActorGroupArray);
        LiveActorGroup* pGroup = pGroupArray->getLiveActorGroup(pActor);
        s32 count = 0;
        if (pGroup == nullptr) {
            return 0;
        }

        for (s32 i = 0; i < pGroup->getObjNum(); i++) {
            LiveActor* pMember = pGroup->getActor(i);
            if (pMember == pActor) {
                continue;
            }

            if (pPred(pMember)) {
                count++;
            }
        }

        return count;
    }

    template <>
    s32 countGroupMember< bool (*)(const LiveActor*) >(const LiveActor* pActor, bool (*pPred)(const LiveActor*)) NO_INLINE {
        LiveActorGroupArray* pGroupArray = MR::getSceneObj< LiveActorGroupArray >(SceneObj_LiveActorGroupArray);
        LiveActorGroup* pGroup = pGroupArray->getLiveActorGroup(pActor);
        s32 count = 0;
        if (pGroup == nullptr) {
            return 0;
        }

        for (s32 i = 0; i < pGroup->getObjNum(); i++) {
            LiveActor* pMember = pGroup->getActor(i);
            if (pMember == pActor) {
                continue;
            }

            if (pPred(pMember)) {
                count++;
            }
        }

        return count;
    }
}  // namespace

namespace MR {
    MsgSharedGroup* joinToGroupArray(LiveActor* pActor, const JMapInfoIter& rIter, const char* pName, s32 maxCount) {
        if (!rIter.isValid()) {
            return nullptr;
        }

        s32 groupId;
        if (getJMapInfoGroupID(rIter, &groupId)) {
            LiveActorGroupArray* pGroupArray = MR::getSceneObj< LiveActorGroupArray >(SceneObj_LiveActorGroupArray);
            return (MsgSharedGroup*)pGroupArray->entry(pActor, rIter, pName, maxCount);
        }

        return nullptr;
    }

    LiveActorGroup* getGroupFromArray(const LiveActor* pActor) {
        LiveActorGroupArray* pGroupArray = MR::getSceneObj< LiveActorGroupArray >(SceneObj_LiveActorGroupArray);
        return pGroupArray->getLiveActorGroup(pActor);
    }

    s32 countHideGroupMember(const LiveActor* pActor) {
        return countGroupMember(pActor, isHiddenModel);
    }

    s32 countShowGroupMember(const LiveActor* pActor) {
        return countGroupMember(pActor, ::isShowModel);
    }

}  // namespace MR
