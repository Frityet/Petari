#include "Game/Util/ActorCameraUtil.hpp"

#include "Game/Camera/CameraTargetArg.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CameraUtilCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    void createActorCameraName(char* pName, u32 size, const LiveActor* pActor, const ActorCameraInfo* pInfo) {
        if ((pInfo->mCameraSetID & 0x8000) != 0) {
            std::snprintf(pName, size, "%s共通%03d", pActor->mName, pInfo->mCameraSetID - 0x8000);
        } else {
            std::snprintf(pName, size, "%s固有%03d", pActor->mName, pInfo->mCameraSetID);
        }
    }

    void createMultiActorCameraName(char* pName, u32 size, const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName) {
        const auto camera_name = pCameraName != nullptr ? pCameraName : "";
        if ((pInfo->mCameraSetID & 0x8000) != 0) {
            std::snprintf(pName, size, "%s共通%s%03d", pActor->mName, camera_name, pInfo->mCameraSetID - 0x8000);
        } else {
            std::snprintf(pName, size, "%s固有%s%03d", pActor->mName, camera_name, pInfo->mCameraSetID);
        }
    }

    [[nodiscard]] std::string animationEventName(const LiveActor* actor, const char* camera_name) {
        if (actor == nullptr || camera_name == nullptr) {
            throw std::invalid_argument("Actor animation camera requires an actor and camera name.");
        }
        return std::string(actor->mName) + camera_name;
    }

    [[nodiscard]] std::span< const std::uint8_t > animationResource(const LiveActor* actor, std::string_view camera_name) {
        const auto retail_name = std::string(camera_name) + ".camn";
        if (const auto resource = smgpc::compat::actor_model_resource_data_if_present(actor, retail_name)) {
            return *resource;
        }
        // Some extracted archives retain the format spelling as the suffix;
        // accept that archive identity without synthesizing animation data.
        const auto format_name = std::string(camera_name) + ".canm";
        if (const auto resource = smgpc::compat::actor_model_resource_data_if_present(actor, format_name)) {
            return *resource;
        }
        throw std::runtime_error("Actor model archive does not contain " + retail_name + " or " + format_name + ".");
    }

}  // namespace

namespace MR {

    void initAnimCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName) {
        if (pInfo == nullptr) {
            throw std::invalid_argument("Actor animation camera requires ActorCameraInfo.");
        }
        const auto event_name = animationEventName(pActor, pCameraName);
        smgpc::compat::declare_event_camera_animation(*pInfo, event_name, animationResource(pActor, pCameraName));
    }

    ActorCameraInfo* createActorCameraInfo(const JMapInfoIter& rIter) {
        const auto info = ActorCameraInfo(rIter);
        auto* camera_system = smgpc::compat::active_camera_system_for_camera_util();
        if (camera_system == nullptr) {
            throw std::logic_error("ActorCameraInfo allocation requires the active stage RuntimeContext.");
        }
        return camera_system->create_actor_camera_info(info.mCameraSetID, info.mZoneID);
    }

    bool createActorCameraInfoIfExist(const JMapInfoIter& rIter, ActorCameraInfo** pInfo) {
        if (pInfo == nullptr) {
            return false;
        }
        const auto probe = ActorCameraInfo(rIter);
        if (probe.mCameraSetID == -1) {
            return false;
        }
        *pInfo = createActorCameraInfo(rIter);
        return true;
    }

    bool initActorCamera(const LiveActor* pActor, const JMapInfoIter& rIter, ActorCameraInfo** pInfo) {
        return initMultiActorCamera(pActor, rIter, pInfo, nullptr);
    }

    bool initMultiActorCamera(const LiveActor* pActor, const JMapInfoIter& rIter, ActorCameraInfo** pInfo, const char* pCameraName) {
        if (pInfo == nullptr) {
            return false;
        }
        if (*pInfo == nullptr) {
            *pInfo = createActorCameraInfo(rIter);
        }
        return initMultiActorCameraNoInit(pActor, *pInfo, pCameraName);
    }

    bool initMultiActorCameraNoInit(const LiveActor* pActor, ActorCameraInfo* pInfo, const char* pCameraName) {
        if (pActor == nullptr || pInfo == nullptr || pInfo->mCameraSetID < 0) {
            return false;
        }
        auto name = std::array< char, 0x100U >{};
        if (pCameraName != nullptr) {
            createMultiActorCameraName(name.data(), name.size(), pActor, pInfo, pCameraName);
        } else {
            createActorCameraName(name.data(), name.size(), pActor, pInfo);
        }
        declareEventCamera(pInfo, name.data());
        return true;
    }

    void initActorCameraProgrammable(const LiveActor* pActor) {
        if (pActor != nullptr) {
            declareEventCameraProgrammable(pActor->mName);
        }
    }

    bool startActorCameraNoTarget(const LiveActor* pActor, const ActorCameraInfo* pInfo, s32 frames) {
        return startMultiActorCameraNoTarget(pActor, pInfo, nullptr, frames);
    }

    bool startActorCameraTargetPlayer(const LiveActor* pActor, const ActorCameraInfo* pInfo, s32 frames) {
        return startMultiActorCameraTargetPlayer(pActor, pInfo, nullptr, frames);
    }

    bool startActorCameraTargetSelf(const LiveActor* pActor, const ActorCameraInfo* pInfo, s32 frames) {
        return startMultiActorCameraTargetSelf(pActor, pInfo, nullptr, frames);
    }

    bool startActorCameraTargetOther(const LiveActor* pActor, const ActorCameraInfo* pInfo, const CameraTargetArg& rTarget, s32 frames) {
        return startMultiActorCameraTargetOther(pActor, pInfo, nullptr, rTarget, frames);
    }

    bool startMultiActorCameraNoTarget(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames) {
        return startMultiActorCameraTargetOther(pActor, pInfo, pCameraName, CameraTargetArg{}, frames);
    }

    bool startMultiActorCameraTargetPlayer(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames) {
        auto target = CameraTargetArg{};
        setCameraTargetToPlayer(&target);
        return startMultiActorCameraTargetOther(pActor, pInfo, pCameraName, target, frames);
    }

    bool startMultiActorCameraTargetSelf(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames) {
        if (pActor == nullptr) {
            return false;
        }
        return startMultiActorCameraTargetOther(pActor, pInfo, pCameraName, CameraTargetArg(pActor), frames);
    }

    bool startMultiActorCameraTargetOther(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName,
                                          const CameraTargetArg& rTarget, s32 frames) {
        if (pActor == nullptr || pInfo == nullptr || pInfo->mCameraSetID < 0) {
            return false;
        }
        auto name = std::array< char, 0x100U >{};
        if (pCameraName != nullptr) {
            createMultiActorCameraName(name.data(), name.size(), pActor, pInfo, pCameraName);
        } else {
            createActorCameraName(name.data(), name.size(), pActor, pInfo);
        }
        startEventCamera(pInfo, name.data(), rTarget, frames);
        return true;
    }

    void startAnimCameraTargetPlayer(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames, f32 speed) {
        auto target = CameraTargetArg{};
        setCameraTargetToPlayer(&target);
        startAnimCameraTargetOther(pActor, pInfo, pCameraName, target, frames, speed);
    }

    void startAnimCameraTargetSelf(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames, f32 speed) {
        startAnimCameraTargetOther(pActor, pInfo, pCameraName, CameraTargetArg(pActor), frames, speed);
    }

    void startAnimCameraTargetOther(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, const CameraTargetArg& rTarget,
                                    s32 frames, f32 speed) {
        const auto name = animationEventName(pActor, pCameraName);
        startEventCameraAnim(pInfo, name.c_str(), rTarget, frames, speed);
    }

    void startActorCameraProgrammable(const LiveActor* pActor, s32 frames) {
        if (pActor != nullptr) {
            startGlobalEventCameraNoTarget(pActor->mName, frames);
        }
    }

    bool endActorCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, bool endForce, s32 frames) {
        return endMultiActorCamera(pActor, pInfo, nullptr, endForce, frames);
    }

    bool endMultiActorCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, bool endForce, s32 frames) {
        if (pActor == nullptr || pInfo == nullptr || pInfo->mCameraSetID < 0) {
            return false;
        }
        auto name = std::array< char, 0x100U >{};
        if (pCameraName != nullptr) {
            createMultiActorCameraName(name.data(), name.size(), pActor, pInfo, pCameraName);
        } else {
            createActorCameraName(name.data(), name.size(), pActor, pInfo);
        }
        endEventCamera(pInfo, name.data(), endForce, frames);
        return true;
    }

    bool endActorCameraAtLanding(const LiveActor* pActor, const ActorCameraInfo* pInfo, s32 frames) {
        return endMultiActorCameraAtLanding(pActor, pInfo, nullptr, frames);
    }

    bool endMultiActorCameraAtLanding(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames) {
        if (pActor == nullptr || pInfo == nullptr || pInfo->mCameraSetID < 0) {
            return false;
        }
        auto name = std::array< char, 0x100U >{};
        if (pCameraName != nullptr) {
            createMultiActorCameraName(name.data(), name.size(), pActor, pInfo, pCameraName);
        } else {
            createActorCameraName(name.data(), name.size(), pActor, pInfo);
        }
        endEventCameraAtLanding(pInfo, name.data(), frames);
        return true;
    }

    void endActorCameraProgrammable(const LiveActor* pActor, s32 frames, bool endForce) {
        if (pActor != nullptr) {
            endGlobalEventCamera(pActor->mName, frames, endForce);
        }
    }

    bool isActiveActorCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo) {
        return isActiveMultiActorCamera(pActor, pInfo, nullptr);
    }

    bool isActiveMultiActorCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName) {
        if (pActor == nullptr || pInfo == nullptr || pInfo->mCameraSetID < 0) {
            return false;
        }
        auto name = std::array< char, 0x100U >{};
        if (pCameraName != nullptr) {
            createMultiActorCameraName(name.data(), name.size(), pActor, pInfo, pCameraName);
        } else {
            createActorCameraName(name.data(), name.size(), pActor, pInfo);
        }
        return isEventCameraActive(pInfo, name.data());
    }

    void setProgrammableCameraParam(const LiveActor* pActor, const TVec3f& rWPoint, const TVec3f& rEye, const TVec3f& rUpVec) {
        if (pActor != nullptr) {
            MR::setProgrammableCameraParam(pActor->mName, rWPoint, rEye, rUpVec, true);
        }
    }

    void setProgrammableCameraParamFovy(const LiveActor* pActor, f32 fovy) {
        if (pActor != nullptr) {
            MR::setProgrammableCameraParamFovy(pActor->mName, fovy);
        }
    }

    void endAnimCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames, bool endForce) {
        const auto name = animationEventName(pActor, pCameraName);
        endEventCamera(pInfo, name.c_str(), endForce, frames);
    }

    s32 getAnimCameraFrame(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName) {
        const auto name = animationEventName(pActor, pCameraName);
        return MR::getAnimCameraFrame(pInfo, name.c_str());
    }

    bool isAnimCameraEnd(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName) {
        const auto name = animationEventName(pActor, pCameraName);
        return MR::isAnimCameraEnd(pInfo, name.c_str());
    }

    s32 getActorCameraFrames(const LiveActor* pActor, const ActorCameraInfo* pInfo) {
        return getMultiActorCameraFrames(pActor, pInfo, nullptr);
    }

    s32 getMultiActorCameraFrames(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName) {
        if (pActor == nullptr || pInfo == nullptr || pInfo->mCameraSetID < 0) {
            return 0;
        }
        auto name = std::array< char, 0x100U >{};
        if (pCameraName != nullptr) {
            createMultiActorCameraName(name.data(), name.size(), pActor, pInfo, pCameraName);
        } else {
            createActorCameraName(name.data(), name.size(), pActor, pInfo);
        }
        return static_cast< s32 >(getEventCameraFrames(pInfo, name.data()));
    }

    bool isExistActorCamera(const ActorCameraInfo* pInfo) {
        return pInfo != nullptr && pInfo->mCameraSetID != -1;
    }

}  // namespace MR
