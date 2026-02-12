#include "TitleRuntimeMR.hpp"

#include <cstdint>

#include "Logger.hpp"
#include "RenderWindow.hpp"
#include "TitleLayoutActor.hpp"

namespace smgpc::game::title::MR {
namespace {

constexpr int ENTER_KEY = 257;
constexpr int A_KEY = 90;
constexpr int B_KEY = 88;

render::IRendererService *sRendererService {};
logging::ILogger *sLogger {};
bool sIsStageBgmPrepared {};

}  // namespace

void beginFrame() {
}

void setInputSource(void *pRendererService, void *pLogger) {
    sRendererService = static_cast<render::IRendererService *>(pRendererService);
    sLogger = static_cast<logging::ILogger *>(pLogger);
}

bool isDisplayEncouragePal60Window() {
    return false;
}

void startAnim(TitleLayoutActor *pActor, const char *pAnimationName, std::uint32_t layer) {
    if (pActor == nullptr || pAnimationName == nullptr) {
        return;
    }
    pActor->startAnim(pAnimationName, layer);
}

bool isAnimStopped(const TitleLayoutActor *pActor, std::uint32_t layer) {
    if (pActor == nullptr) {
        return true;
    }
    return pActor->isAnimStopped(layer);
}

void setAnimFrameAndStop(TitleLayoutActor *pActor, float frame, std::uint32_t layer) {
    if (pActor == nullptr) {
        return;
    }
    pActor->setAnimFrameAndStop(frame, layer);
}

void emitEffect(TitleLayoutActor *pActor, const char *pEffectName) {
    if (pActor == nullptr) {
        return;
    }
    pActor->emitEffect(pEffectName);
}

void deleteEffectAll(TitleLayoutActor *pActor) {
    if (pActor == nullptr) {
        return;
    }
    pActor->deleteEffectAll();
}

bool isDead(const TitleLayoutActor *pActor) {
    if (pActor == nullptr) {
        return true;
    }
    return pActor->isDead();
}

bool testCorePadButtonA(int channel) {
    (void)channel;
    return sRendererService != nullptr && (sRendererService->is_key_down(ENTER_KEY) || sRendererService->is_key_down(A_KEY));
}

bool testCorePadButtonB(int channel) {
    (void)channel;
    return sRendererService != nullptr && (sRendererService->is_key_down(ENTER_KEY) || sRendererService->is_key_down(B_KEY));
}

void startStageBGM(const char *name, bool prepare) {
    sIsStageBgmPrepared = prepare;
    if (sLogger != nullptr) {
        sLogger->debug(__FILE__, __LINE__, logging::Category::GAME, "startStageBGM name={} prepare={}", name != nullptr ? name : "<null>", prepare);
    }
}

bool isPreparedStageBgm() {
    return sIsStageBgmPrepared;
}

void unlockStageBGM() {
    if (sLogger != nullptr) {
        sLogger->debug(__FILE__, __LINE__, logging::Category::GAME, "unlockStageBGM");
    }
}

void stopStageBGM(int fadeFrames) {
    if (sLogger != nullptr) {
        sLogger->debug(__FILE__, __LINE__, logging::Category::GAME, "stopStageBGM fadeFrames={}", fadeFrames);
    }
}

void startSystemSE(const char *name, int, int) {
    if (sLogger != nullptr) {
        sLogger->debug(__FILE__, __LINE__, logging::Category::GAME, "startSystemSE {}", name != nullptr ? name : "<null>");
    }
}

void startCSSound(const char *name, int, int) {
    if (sLogger != nullptr) {
        sLogger->debug(__FILE__, __LINE__, logging::Category::GAME, "startCSSound {}", name != nullptr ? name : "<null>");
    }
}

void tryRumblePadMiddle(void *, int) {
    if (sLogger != nullptr) {
        sLogger->debug(__FILE__, __LINE__, logging::Category::GAME, "tryRumblePadMiddle");
    }
}

}  // namespace smgpc::game::title::MR
