#include "Game/Util/DemoUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/Functor.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/DemoUtilCompat.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <optional>
#include <string>

namespace {
    bool sIsDemoActive = false;
    std::string sActiveDemoName;
    const NameObj *sActiveDemoOwner = nullptr;
    bool sPuppetableControlOwned = false;
    const NameObj *sPuppetableControlOwner = nullptr;
    bool sControlWasEnabledBeforePuppet = true;

    void clear_active_demo_state() {
        smgpc::compat::release_puppetable_demo_control(false);
        sIsDemoActive = false;
        sActiveDemoName.clear();
        sActiveDemoOwner = nullptr;
    }
}  // namespace

namespace smgpc::compat {
    void release_puppetable_demo_control(bool force_enable) {
        auto *player = active_player_system_for_player_util();
        if (player != nullptr && (force_enable || (sPuppetableControlOwned && sControlWasEnabledBeforePuppet))) {
            player->enable_control(false);
        }
        sPuppetableControlOwned = false;
        sPuppetableControlOwner = nullptr;
        sControlWasEnabledBeforePuppet = true;
    }

    void activate_demo_state(const NameObj *owner, std::string_view demo_name,
                             bool puppetable) {
        if (sIsDemoActive &&
            (sActiveDemoOwner != owner || sActiveDemoName != demo_name)) {
            clear_active_demo_state();
        }
#ifndef NDEBUG
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_semantic_trace_event("demo", "demo_started",
                                               "name=" + std::string(demo_name));
        }
#endif
        sIsDemoActive = true;
        sActiveDemoName = demo_name;
        sActiveDemoOwner = owner;

        if (puppetable && !sPuppetableControlOwned) {
            if (auto *player = active_player_system_for_player_util()) {
                sControlWasEnabledBeforePuppet = player->is_control_enabled();
                player->disable_control();
                sPuppetableControlOwned = true;
                sPuppetableControlOwner = owner;
            }
        }
    }

    void finish_demo_state(const NameObj *owner, std::string_view demo_name) {
        if (!sIsDemoActive || sActiveDemoOwner != owner ||
            sActiveDemoName != demo_name) {
            return;
        }
#ifndef NDEBUG
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_semantic_trace_event("demo", "demo_ended",
                                               "name=" + std::string(demo_name));
        }
#endif
        clear_active_demo_state();
    }

    void release_active_demo_for_owner(const LiveActor *owner) {
        if (owner == nullptr || sActiveDemoOwner != static_cast<const NameObj *>(owner)) {
            return;
        }
#ifndef NDEBUG
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_semantic_trace_event(
                "demo", "demo_owner_released",
                "name=" + sActiveDemoName + ";owner=" +
                    std::string(owner->getName() != nullptr ? owner->getName() : ""));
        }
#endif
        clear_active_demo_state();
    }
}  // namespace smgpc::compat

namespace MR {
    void registerDemoActionFunctor(const LiveActor *pActor, const MR::FunctorBase &rFunctor, const char *pActionName) {
        static_cast<void>(MR::tryRegisterDemoActionFunctor(pActor, rFunctor, pActionName));
    }

    bool tryStartDemoWithoutCinemaFrame(LiveActor *pActor, const char *pDemoName) {
        if (auto *runtime = smgpc::compat::active_demo_scene_runtime()) {
            (void)runtime->stop_active_demo(nullptr, std::nullopt);
        }
        smgpc::compat::activate_demo_state(pActor, pDemoName != nullptr ? pDemoName : "",
                                           false);
        return true;
    }

    bool tryStartDemoMarioPuppetable(LiveActor *pActor, const char *pDemoName) {
        if (auto *runtime = smgpc::compat::active_demo_scene_runtime()) {
            (void)runtime->stop_active_demo(nullptr, std::nullopt);
        }
        smgpc::compat::activate_demo_state(pActor, pDemoName != nullptr ? pDemoName : "",
                                           true);
        return true;
    }

    bool requestStartDemo(LiveActor *pActor, const char *pDemoName, const Nerve *pCanStartNerve, const Nerve *pCannotStartNerve) {
        const auto started = tryStartDemoWithoutCinemaFrame(pActor, pDemoName);
        if (pActor != nullptr) {
            pActor->setNerve(started ? pCanStartNerve : pCannotStartNerve);
        }
        return started;
    }

    void endDemo(NameObj *pOwner, const char *pDemoName) {
        static_cast<void>(pOwner);
        static_cast<void>(pDemoName);
        if (auto *runtime = smgpc::compat::active_demo_scene_runtime()) {
            (void)runtime->stop_active_demo(nullptr, std::nullopt);
        }
        if (!sIsDemoActive) {
            return;
        }
#ifndef NDEBUG
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            runtime->emit_semantic_trace_event("demo", "demo_ended",
                                               "name=" + sActiveDemoName);
        }
#endif
        clear_active_demo_state();
    }

    bool isDemoActive() {
        return sIsDemoActive;
    }

    bool isDemoActive(const char *pDemoName) {
        return sIsDemoActive && pDemoName != nullptr && sActiveDemoName == pDemoName;
    }
}  // namespace MR
