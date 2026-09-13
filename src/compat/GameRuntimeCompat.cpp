#include "compat/ClippingDirectorOwnership.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/ClippingActorHolder.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace MR {
    void setClippingTypeSphere(LiveActor* pActor, f32 radius) {
        MR::getClippingDirector()->mActorHolder->setTypeToSphere(pActor, radius, nullptr);
    }

    void setClippingTypeSphere(LiveActor* pActor, f32 radius, const TVec3f* pTrans) {
        MR::getClippingDirector()->mActorHolder->setTypeToSphere(pActor, radius, pTrans);
    }

    void setClippingFar50m(LiveActor* pActor) {
        MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 7);
    }

    void setClippingFar(LiveActor* pActor, f32 clipping) {
        s32 clip = clipping;

        switch (clip) {
        case 50:
            MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 7);
            break;
        case 100:
            MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 6);
            break;
        case 200:
            MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 5);
            break;
        case 300:
            MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 4);
            break;
        case 400:
            MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 3);
            break;
        case 500:
            MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 2);
            break;
        case 600:
            MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 1);
            break;
        case -1:
            MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 0);
            break;
        }
    }

    void setGroupClipping(LiveActor* pActor, const JMapInfoIter& rIter, int a3) {
        MR::getClippingDirector()->joinToGroupClipping(pActor, rIter, a3);
        smgpc::scene::current_clipping_director_ownership()->capture_groups();
    }
}
