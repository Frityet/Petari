#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LiveActorGroupArray.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

// Original LiveActorUtil group wrappers; the complete group implementation
// is compiled from Game/LiveActor/LiveActorGroupArray.cpp.
namespace {
    bool isShowModel(LiveActor* pActor) NO_INLINE {
        return !pActor->mFlag.mIsHiddenModel;
    }

    void callFuncAllGroupMember(const LiveActor* pActor, void (*pFunc)(LiveActor*)) NO_INLINE {
        LiveActorGroupArray* pGroupArray = MR::getSceneObj< LiveActorGroupArray >(SceneObj_LiveActorGroupArray);
        LiveActorGroup* pGroup = pGroupArray->getLiveActorGroup(pActor);
        if (pGroup == nullptr) {
            return;
        }

        for (s32 i = 0; i < pGroup->getObjNum(); i++) {
            LiveActor* pMember = pGroup->getActor(i);
            if (pMember == pActor) {
                continue;
            }

            pFunc(pMember);
        }
    }

    void callMethodAllGroupMember(const LiveActor* pActor, void (LiveActor::*pMethod)()) NO_INLINE {
        LiveActorGroupArray* pGroupArray = MR::getSceneObj< LiveActorGroupArray >(SceneObj_LiveActorGroupArray);
        LiveActorGroup* pGroup = pGroupArray->getLiveActorGroup(pActor);
        if (pGroup == nullptr) {
            return;
        }

        for (s32 i = 0; i < pGroup->getObjNum(); i++) {
            LiveActor* pMember = pGroup->getActor(i);
            if (pMember == pActor) {
                continue;
            }

            (pMember->*pMethod)();
        }
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

    void callMakeActorDeadAllGroupMember(const LiveActor* pActor) {
        ::callMethodAllGroupMember(pActor, &LiveActor::makeActorDead);
    }

    void callKillAllGroupMember(const LiveActor* pActor) {
        ::callMethodAllGroupMember(pActor, &LiveActor::kill);
    }

    void callMakeActorAppearedAllGroupMember(const LiveActor* pActor) {
        ::callMethodAllGroupMember(pActor, &LiveActor::makeActorAppeared);
    }

    void callAppearAllGroupMember(const LiveActor* pActor) {
        ::callMethodAllGroupMember(pActor, &LiveActor::appear);
    }

    void callRequestMovementOnAllGroupMember(const LiveActor* pActor) {
        ::callFuncAllGroupMember(pActor, requestMovementOn);
    }

    void callInvalidateClippingAllGroupMember(const LiveActor* pActor) {
        ::callFuncAllGroupMember(pActor, invalidateClipping);
    }

    void callValidateClippingAllGroupMember(const LiveActor* pActor) {
        ::callFuncAllGroupMember(pActor, validateClipping);
    }

    s32 countHideGroupMember(const LiveActor* pActor) {
        return countGroupMember(pActor, isHiddenModel);
    }

    s32 countShowGroupMember(const LiveActor* pActor) {
        return countGroupMember(pActor, ::isShowModel);
    }

}  // namespace MR
