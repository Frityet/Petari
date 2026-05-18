#include "Game/Util/DemoUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/compat/RuntimeContext.hpp"

#include <string>

namespace MR {
    bool tryStartDemoWithoutCinemaFrame(const LiveActor *pActor, const char *pDemoName) {
#ifndef NDEBUG
        if (auto *runtime = smgpc::game::RuntimeContext::try_instance(); runtime != nullptr && pDemoName != nullptr) {
            runtime->emit_semantic_trace_event("demo", "demo_started", "name=" + std::string(pDemoName));
        }
#endif
        static_cast<void>(pActor);
        return true;
    }

    bool tryStartDemoMarioPuppetable(const LiveActor *pActor, const char *pDemoName) {
        return tryStartDemoWithoutCinemaFrame(pActor, pDemoName);
    }

    void endDemo(const LiveActor *, const char *pDemoName) {
#ifndef NDEBUG
        if (auto *runtime = smgpc::game::RuntimeContext::try_instance(); runtime != nullptr && pDemoName != nullptr) {
            runtime->emit_semantic_trace_event("demo", "demo_ended", "name=" + std::string(pDemoName));
        }
#endif
    }
}  // namespace MR
