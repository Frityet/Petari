#include <aurora/exception.hpp>
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <stdexcept>

namespace MR {
    void setClippingFar50m(LiveActor* pActor) {
        smgpc::compat::configure_actor_clipping_far_level(pActor, 7);
    }

    void setClippingTypeSphere(LiveActor* pActor, f32 radius) {
        smgpc::compat::configure_actor_clipping_sphere(pActor, radius, nullptr);
    }

    void setClippingTypeSphere(LiveActor* pActor, f32 radius, const TVec3f* pCenter) {
        smgpc::compat::configure_actor_clipping_sphere(pActor, radius, pCenter);
    }

    void setClippingFar(LiveActor* pActor, f32 distance) {
        switch (static_cast<s32>(distance)) {
        case 50:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 7);
            break;
        case 100:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 6);
            break;
        case 200:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 5);
            break;
        case 300:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 4);
            break;
        case 400:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 3);
            break;
        case 500:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 2);
            break;
        case 600:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 1);
            break;
        case -1:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 0);
            break;
        default:
            break;
        }
    }

    void setGroupClipping(LiveActor* pActor, const JMapInfoIter& rIter, int) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Group clipping requires a LiveActor.");
        }

        auto clipping_group_id = s32{-1};
        if (MR::getJMapInfoClippingGroupID(rIter, &clipping_group_id) && clipping_group_id >= 0) {
            aurora::throw_host_exception<std::logic_error>("Group clipping is unavailable without ClippingGroupHolder.");
        }
    }

}  // namespace MR
