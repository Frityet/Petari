#include "Game/Util/ObjUtil.hpp"

#include "Logger.hpp"
#include "compat/DecompIntegration.hpp"
#include "compat/RuntimeContext.hpp"

namespace {

// SMGPC_INTEGRATION_BEGIN
SMGPC_STUB(src/Game/Effect/EffectSystemUtil.cpp);
// SMGPC_INTEGRATION_END

}  // namespace

namespace MR {

void tryRumblePadMiddle(void *pActor, int intensity) {
    (void)pActor;

    auto *logger = smgpc::game::compat::runtime_context().logger;
    if (logger != nullptr) {
        logger->debug(__FILE__, __LINE__, smgpc::logging::Category::GAME, "tryRumblePadMiddle intensity={}", intensity);
    }
}

}  // namespace MR
