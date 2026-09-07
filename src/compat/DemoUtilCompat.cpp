#include "Game/Util/DemoUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/Functor.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/DemoUtilCompat.hpp"
#include "compat/StageSessionState.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    [[noreturn]] void throw_programmable_demo_unavailable(
        std::string_view operation) {
        throw std::logic_error(
            std::string(operation) +
            " requires the real programmable DemoDirector movement/cinema-frame closure; "
            "a DemoSheet executor is not a substitute.");
    }
}  // namespace

namespace smgpc::compat {
    void release_puppetable_demo_control(bool force_enable) {
        require_active_demo_scene_runtime("Puppetable demo teardown")
            .release_puppetable_control(force_enable);
    }
}  // namespace smgpc::compat

namespace MR {
    void registerDemoActionFunctor(const LiveActor *pActor, const MR::FunctorBase &rFunctor, const char *pActionName) {
        if (!MR::tryRegisterDemoActionFunctor(pActor, rFunctor, pActionName)) {
            throw std::logic_error(
                "Required demo functor registration has no matching real Action row.");
        }
    }

    bool tryStartDemoWithoutCinemaFrame(LiveActor *pActor, const char *pDemoName) {
        (void)smgpc::compat::require_active_demo_scene_runtime(
            "Programmable demo start");
        if (pActor == nullptr || pDemoName == nullptr) {
            throw std::invalid_argument(
                "A programmable demo requires a real starter and demo name.");
        }
        throw_programmable_demo_unavailable("MR::tryStartDemoWithoutCinemaFrame");
    }

    bool tryStartDemoMarioPuppetable(LiveActor *pActor, const char *pDemoName) {
        (void)smgpc::compat::require_active_demo_scene_runtime(
            "Programmable puppetable demo start");
        if (pActor == nullptr || pDemoName == nullptr) {
            throw std::invalid_argument(
                "A programmable puppetable demo requires a real starter and demo name.");
        }
        throw_programmable_demo_unavailable("MR::tryStartDemoMarioPuppetable");
    }

    bool requestStartDemo(LiveActor *pActor, const char *pDemoName, const Nerve *pCanStartNerve, const Nerve *pCannotStartNerve) {
        static_cast<void>(pCanStartNerve);
        static_cast<void>(pCannotStartNerve);
        (void)smgpc::compat::require_active_demo_scene_runtime(
            "Programmable demo request");
        if (pActor == nullptr || pDemoName == nullptr) {
            throw std::invalid_argument(
                "A programmable demo request requires a real starter and demo name.");
        }
        throw_programmable_demo_unavailable("MR::requestStartDemo");
    }

    bool requestStartDemoWithoutCinemaFrame(
        LiveActor *pActor, const char *pDemoName,
        const Nerve *pCanStartNerve, const Nerve *pCannotStartNerve) {
        static_cast<void>(pCanStartNerve);
        static_cast<void>(pCannotStartNerve);
        (void)smgpc::compat::require_active_demo_scene_runtime(
            "Programmable demo request without cinema frame");
        if (pActor == nullptr || pDemoName == nullptr) {
            throw std::invalid_argument(
                "A programmable demo request requires a real starter and demo name.");
        }
        throw_programmable_demo_unavailable(
            "MR::requestStartDemoWithoutCinemaFrame");
    }

    void endDemo(NameObj *pOwner, const char *pDemoName) {
        static_cast<void>(pOwner);
        static_cast<void>(pDemoName);
        auto &runtime = smgpc::compat::require_active_demo_scene_runtime(
            "Demo end");
        if (!runtime.stop_active_demo(nullptr, std::nullopt)) {
            throw std::logic_error(
                "Cannot end a demo without a real active DemoDirector state.");
        }
    }

    bool isDemoActive() {
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Demo active query")
            .is_active();
    }

    bool isDemoActive(const char *pDemoName) {
        if (pDemoName == nullptr) {
            throw std::invalid_argument(
                "A named demo-active query requires a real demo name.");
        }
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Named demo active query")
            .is_active(pDemoName);
    }

    bool isPowerStarGetDemoActive() {
        return smgpc::compat::require_active_stage_session().is_power_star_get_demo_active();
    }
}  // namespace MR
