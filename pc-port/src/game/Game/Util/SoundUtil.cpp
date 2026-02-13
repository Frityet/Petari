#include "Game/Util/SoundUtil.hpp"

#include "Logger.hpp"
#include "compat/DecompIntegration.hpp"
#include "compat/RuntimeContext.hpp"

namespace {

// SMGPC_INTEGRATION_BEGIN
SMGPC_STUB(src/Game/Util/SoundUtil.cpp);
// SMGPC_INTEGRATION_END
bool sPreparedStageBgm {};

}  // namespace

namespace MR {

void startStageBGM(const char *name, bool prepare) {
    sPreparedStageBgm = prepare;

    auto *logger = smgpc::game::compat::runtime_context().logger;
    if (logger != nullptr) {
        logger->debug(__FILE__, __LINE__, smgpc::logging::Category::GAME, "startStageBGM name={} prepare={}", name != nullptr ? name : "<null>", prepare);
    }
}

bool isPreparedStageBgm() {
    return sPreparedStageBgm;
}

void unlockStageBGM() {
    auto *logger = smgpc::game::compat::runtime_context().logger;
    if (logger != nullptr) {
        logger->debug(__FILE__, __LINE__, smgpc::logging::Category::GAME, "unlockStageBGM");
    }
}

void stopStageBGM(int fadeFrames) {
    auto *logger = smgpc::game::compat::runtime_context().logger;
    if (logger != nullptr) {
        logger->debug(__FILE__, __LINE__, smgpc::logging::Category::GAME, "stopStageBGM fadeFrames={}", fadeFrames);
    }
}

void startSystemSE(const char *name, int, int) {
    auto *logger = smgpc::game::compat::runtime_context().logger;
    if (logger != nullptr) {
        logger->debug(__FILE__, __LINE__, smgpc::logging::Category::GAME, "startSystemSE {}", name != nullptr ? name : "<null>");
    }
}

void startCSSound(const char *name, int, int) {
    auto *logger = smgpc::game::compat::runtime_context().logger;
    if (logger != nullptr) {
        logger->debug(__FILE__, __LINE__, smgpc::logging::Category::GAME, "startCSSound {}", name != nullptr ? name : "<null>");
    }
}

}  // namespace MR
