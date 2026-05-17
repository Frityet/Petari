#include "Game/Util/LightUtil.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"

namespace MR {
    namespace {
        [[nodiscard]] const ActorLightInfo* actorLightInfoForType(const AreaLightInfo* pInfo, s32 type) {
            if (pInfo == nullptr) {
                return nullptr;
            }

            switch (type) {
            case MR::LightType_Player:
                return &pInfo->mPlayerLight;
            case MR::LightType_Strong:
                return &pInfo->mStrongLight;
            case MR::LightType_Weak:
                return &pInfo->mWeakLight;
            case MR::LightType_Planet:
                return &pInfo->mPlanetLight;
            default:
                return nullptr;
            }
        }
    }  // namespace

    void loadLight(s32 type) {
        if (type == MR::LightType_None) {
            return;
        }

        const auto* info = actorLightInfoForType(LightFunction::getAreaLightInfo(ZoneLightID{}), type);
        if (info != nullptr) {
            LightFunction::loadActorLightInfo(info);
        }
    }

    void initActorLightInfoLightType(LiveActor* pActor, s32 type) {
        if (pActor != nullptr && pActor->mActorLightCtrl != nullptr) {
            pActor->mActorLightCtrl->_4 = type;
        }
    }

    void initActorLightInfoDrawBuffer(LiveActor*, DrawBuffer*) {
    }

    void loadLightPlayer() {
        MR::loadLight(MR::LightType_Player);
    }
}  // namespace MR
