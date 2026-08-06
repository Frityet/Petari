#include "Game/Util/DemoUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "runtime/RuntimeContext.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoUtilCompat.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace {
    struct DemoAction {
        const Nerve* nerve = nullptr;
        std::unique_ptr<MR::FunctorBase> functor;
    };

    struct DemoCast {
        LiveActor* actor = nullptr;
        s32 group_id = -1;
        s32 cast_id = -1;
        std::unordered_map<std::string, DemoAction> actions;
    };

    std::unordered_map<const LiveActor*, DemoCast> sDemoCasts;

    std::string action_key(const char* pName) {
        return pName != nullptr ? pName : "";
    }

    void trace_demo_cast(const char* event, const DemoCast& cast, const char* pAction = nullptr) {
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            runtime->emit_semantic_trace_event(
                "demo", event,
                "actor=" + std::string(cast.actor != nullptr && cast.actor->getName() != nullptr ? cast.actor->getName() : "") +
                    ";group=" + std::to_string(cast.group_id) + ";cast=" + std::to_string(cast.cast_id) +
                    (pAction != nullptr ? ";action=" + std::string(pAction) : std::string{}));
        }
#else
        static_cast<void>(event);
        static_cast<void>(cast);
        static_cast<void>(pAction);
#endif
    }

    bool execute_action(DemoCast& cast, const char* pAction) {
        const auto found = cast.actions.find(action_key(pAction));
        if (found == cast.actions.end()) {
            return false;
        }
        if (found->second.nerve != nullptr && cast.actor != nullptr) {
            cast.actor->setNerve(found->second.nerve);
        }
        if (found->second.functor != nullptr) {
            (*found->second.functor)();
        }
        trace_demo_cast("action_started", cast, pAction);
        return true;
    }
}  // namespace

namespace smgpc::compat {
    void release_demo_runtime_state(const LiveActor* actor) {
        release_active_demo_for_owner(actor);
        sDemoCasts.erase(actor);
    }

    bool has_registered_demo_cast(const LiveActor* actor) {
        return sDemoCasts.contains(actor);
    }

    std::size_t registered_demo_action_count(const LiveActor* actor) {
        const auto found = sDemoCasts.find(actor);
        return found != sDemoCasts.end() ? found->second.actions.size() : 0U;
    }
}  // namespace smgpc::compat

namespace MR {
    bool tryRegisterDemoCast(LiveActor* pActor, const JMapInfoIter& rIter) {
        if (pActor == nullptr) {
            return false;
        }
        const auto group_id = MR::getDemoGroupID(rIter);
        const auto cast_id = MR::getDemoCastID(rIter);
        if (group_id < 0 || cast_id < 0) {
            return false;
        }
        auto cast = DemoCast{
            .actor = pActor,
            .group_id = group_id,
            .cast_id = cast_id,
        };
        auto& registered = sDemoCasts.insert_or_assign(pActor, std::move(cast)).first->second;
        trace_demo_cast("cast_registered", registered);
        return true;
    }

    bool tryRegisterDemoActionFunctor(const LiveActor* pActor, const MR::FunctorBase& rFunctor, const char* pActionName) {
        const auto found = sDemoCasts.find(pActor);
        if (found == sDemoCasts.end()) {
            return false;
        }
        auto action = DemoAction{};
        action.functor.reset(rFunctor.clone(nullptr));
        found->second.actions[action_key(pActionName)] = std::move(action);
        trace_demo_cast("functor_registered", found->second, pActionName);
        return true;
    }

    void registerDemoActionNerve(const LiveActor* pActor, const Nerve* pNerve, const char* pActionName) {
        const auto found = sDemoCasts.find(pActor);
        if (found == sDemoCasts.end()) {
            return;
        }
        auto& action = found->second.actions[action_key(pActionName)];
        action.nerve = pNerve;
        trace_demo_cast("nerve_registered", found->second, pActionName);
    }

    void startTimeKeepDemoMarioPuppetable(NameObj* pObj, const char* pDemoName, const char* pPartName) {
        auto* actor = dynamic_cast<LiveActor*>(pObj);
        if (actor != nullptr) {
            const auto found = sDemoCasts.find(actor);
            if (found != sDemoCasts.end()) {
                (void)execute_action(found->second, pPartName);
            }
        }
        (void)MR::tryStartDemoMarioPuppetable(actor, pDemoName);
    }

    void timeKeepDemoFadeOut() {
        MR::closeWipeFade(60);
    }

    void timeKeepDemoFadeIn() {
        MR::openWipeFade(60);
    }
}  // namespace MR
