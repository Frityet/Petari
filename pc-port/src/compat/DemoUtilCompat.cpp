#include "Game/Util/DemoUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/Functor.hpp"
#include "compat/DemoUtilCompat.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <string>

namespace {
    bool sIsDemoActive = false;
    std::string sActiveDemoName;
    const LiveActor *sActiveDemoOwner = nullptr;
    bool sPuppetableControlOwned = false;
    const LiveActor *sPuppetableControlOwner = nullptr;
    bool sControlWasEnabledBeforePuppet = true;
}

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

    void release_active_demo_for_owner(const LiveActor *owner) {
        if (owner == nullptr || sActiveDemoOwner != owner) {
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
        if (sPuppetableControlOwner == owner) {
            release_puppetable_demo_control(false);
        }
        sIsDemoActive = false;
        sActiveDemoName.clear();
        sActiveDemoOwner = nullptr;
    }
}  // namespace smgpc::compat

namespace MR {
    void registerDemoActionFunctor(const LiveActor* pActor, const MR::FunctorBase& rFunctor, const char* pActionName) {
        static_cast< void >(MR::tryRegisterDemoActionFunctor(pActor, rFunctor, pActionName));
    }

    bool tryStartDemoWithoutCinemaFrame(LiveActor* pActor, const char* pDemoName) {
        const auto requested_name = std::string(pDemoName != nullptr ? pDemoName : "");
        if (sIsDemoActive && (sActiveDemoOwner != pActor || sActiveDemoName != requested_name)) {
            smgpc::compat::release_active_demo_for_owner(sActiveDemoOwner);
        }
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pDemoName != nullptr) {
            runtime->emit_semantic_trace_event("demo", "demo_started", "name=" + std::string(pDemoName));
        }
#endif
        sIsDemoActive = true;
        sActiveDemoName = requested_name;
        sActiveDemoOwner = pActor;
        return true;
    }

    bool tryStartDemoMarioPuppetable(LiveActor* pActor, const char* pDemoName) {
        const auto started = tryStartDemoWithoutCinemaFrame(pActor, pDemoName);
        if (started && !sPuppetableControlOwned) {
            if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
                sControlWasEnabledBeforePuppet = player->is_control_enabled();
                player->disable_control();
                sPuppetableControlOwned = true;
                sPuppetableControlOwner = pActor;
            }
        }
        return started;
    }

    bool requestStartDemo(LiveActor* pActor, const char* pDemoName, const Nerve* pCanStartNerve, const Nerve* pCannotStartNerve) {
        const auto started = tryStartDemoWithoutCinemaFrame(pActor, pDemoName);
        if (pActor != nullptr) {
            pActor->setNerve(started ? pCanStartNerve : pCannotStartNerve);
        }
        return started;
    }

    void endDemo(NameObj* pOwner, const char* pDemoName) {
        const auto owner_matches = pOwner == nullptr ||
                                   (sActiveDemoOwner != nullptr && static_cast<const NameObj *>(sActiveDemoOwner) == pOwner);
        const auto name_matches = pDemoName == nullptr || sActiveDemoName == pDemoName;
        if (!owner_matches || !name_matches) {
            return;
        }
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pDemoName != nullptr) {
            runtime->emit_semantic_trace_event("demo", "demo_ended", "name=" + std::string(pDemoName));
        }
#endif
        smgpc::compat::release_puppetable_demo_control(false);
        sIsDemoActive = false;
        sActiveDemoName.clear();
        sActiveDemoOwner = nullptr;
    }

    bool isDemoActive() {
        return sIsDemoActive;
    }

    bool isDemoActive(const char* pDemoName) {
        return sIsDemoActive && pDemoName != nullptr && sActiveDemoName == pDemoName;
    }
}  // namespace MR
