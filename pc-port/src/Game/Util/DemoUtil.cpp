#include "Game/Util/DemoUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/Functor.hpp"
#include "runtime/RuntimeContext.hpp"

#include <string>

namespace {
    bool sIsDemoActive = false;
    std::string sActiveDemoName;
}

namespace MR {
    void registerDemoActionFunctor(const LiveActor* pActor, const MR::FunctorBase& rFunctor, const char* pActionName) {
        static_cast< void >(MR::tryRegisterDemoActionFunctor(pActor, rFunctor, pActionName));
    }

    bool tryStartDemoWithoutCinemaFrame(const LiveActor* pActor, const char* pDemoName) {
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pDemoName != nullptr) {
            runtime->emit_semantic_trace_event("demo", "demo_started", "name=" + std::string(pDemoName));
        }
#endif
        static_cast< void >(pActor);
        sIsDemoActive = true;
        sActiveDemoName = pDemoName != nullptr ? pDemoName : "";
        return true;
    }

    bool tryStartDemoMarioPuppetable(const LiveActor* pActor, const char* pDemoName) {
        return tryStartDemoWithoutCinemaFrame(pActor, pDemoName);
    }

    bool requestStartDemo(LiveActor* pActor, const char* pDemoName, const Nerve* pCanStartNerve, const Nerve* pCannotStartNerve) {
        const auto started = tryStartDemoWithoutCinemaFrame(pActor, pDemoName);
        if (pActor != nullptr) {
            pActor->setNerve(started ? pCanStartNerve : pCannotStartNerve);
        }
        return started;
    }

    void endDemo(const LiveActor*, const char* pDemoName) {
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pDemoName != nullptr) {
            runtime->emit_semantic_trace_event("demo", "demo_ended", "name=" + std::string(pDemoName));
        }
#endif
        if (pDemoName == nullptr || sActiveDemoName == pDemoName) {
            sIsDemoActive = false;
            sActiveDemoName.clear();
        }
    }

    bool isDemoActive() {
        return sIsDemoActive;
    }

    bool isDemoActive(const char* pDemoName) {
        return sIsDemoActive && pDemoName != nullptr && sActiveDemoName == pDemoName;
    }
}  // namespace MR
