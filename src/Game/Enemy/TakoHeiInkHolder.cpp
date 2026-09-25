#include "resource/TextEncoding.hpp"
#include "Game/Enemy/TakoHeiInkHolder.hpp"
#include "Game/Enemy/TakoHeiInk.hpp"
#include "Game/Scene/SceneObjHolder.hpp"

namespace {
    static const s32 sMaxTakoHeiInk = 16;
};  // namespace

TakoHeiInkHolder::TakoHeiInkHolder() : DeriveActorGroup< TakoHeiInk >(CP932("タコヘイ墨管理"), ::sMaxTakoHeiInk) {
    TakoHeiInk* pInk;

    for (int i = 0; i < ::sMaxTakoHeiInk; i++) {
        pInk = new TakoHeiInk(CP932("タコヘイ墨"));
        pInk->initWithoutIter();
        registerActor(pInk);
    }
}

namespace MR {
    void createTakoHeiInkHolder() {
        createSceneObj(SceneObj_TakoHeiInkHolder);
    }

    bool spurtTakoHeiInk(const TVec3f& rTrans, const TVec3f& rVelocity) {
        TakoHeiInk* pInk = MR::getSceneObj< TakoHeiInkHolder >(SceneObj_TakoHeiInkHolder)->getDeadMember();

        if (pInk == nullptr) {
            return false;
        }

        pInk->start(rTrans, rVelocity, false);

        return true;
    }
};  // namespace MR
