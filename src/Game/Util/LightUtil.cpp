#include "Game/Util/LightUtil.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/LightDirector.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/Color.hpp"
#include "runtime/RuntimeContext.hpp"

#include <stdexcept>

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

        if (type == MR::LightType_Player) {
            if (auto* holder = MR::getSceneObjHolder(); holder != nullptr) {
                if (auto* director = static_cast< LightDirector* >(holder->getObj(SceneObj_LightDirector)); director != nullptr) {
                    director->loadLightPlayer();
                    return;
                }
            }
            if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
                if (const auto* player_light = runtime->scene_lights().player_light_ctrl(); player_light != nullptr) {
                    // LightDirector::loadLightPlayer loads the registered
                    // ActorLightCtrl.  Loading slots 0..2 here deliberately
                    // preserves independently managed point-light slots.
                    player_light->loadLight();
                    return;
                }
            }
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

    void initActorLightInfoDrawBuffer(LiveActor* pActor, DrawBuffer* pDrawBuffer) {
        if (pActor != nullptr && pActor->mActorLightCtrl != nullptr) {
            pActor->mActorLightCtrl->_8 = pDrawBuffer;
        }
    }

    void loadLightPlayer() {
        MR::loadLight(MR::LightType_Player);
    }

    void requestPointLight(const LiveActor* pActor, TVec3f position, Color8 color, f32 brightness, s32 duration) {
        if (pActor == nullptr) {
            throw std::invalid_argument("a point-light request requires a LiveActor");
        }
        auto* holder = MR::getSceneObjHolder();
        if (holder == nullptr) {
            throw std::logic_error("a point-light request requires an active SceneObjHolder");
        }
        auto* director = static_cast< LightDirector* >(holder->getObj(SceneObj_LightDirector));
        if (director == nullptr) {
            throw std::logic_error("a point-light request requires the scene-owned LightDirector");
        }
        if (director->mPointCtrl == nullptr) {
            throw std::logic_error("a point-light request requires an initialized LightPointCtrl");
        }
        director->mPointCtrl->requestPointLight(pActor, position, color, brightness, duration);
    }
}  // namespace MR
