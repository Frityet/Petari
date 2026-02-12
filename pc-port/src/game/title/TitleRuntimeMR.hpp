#pragma once

#include <cstdint>

namespace smgpc::game::title {

class TitleLayoutActor;

namespace MR {

void beginFrame();
void setInputSource(void *pRendererService, void *pLogger);

[[nodiscard]] bool isDisplayEncouragePal60Window();

void startAnim(TitleLayoutActor *pActor, const char *pAnimationName, std::uint32_t layer);
[[nodiscard]] bool isAnimStopped(const TitleLayoutActor *pActor, std::uint32_t layer);
void setAnimFrameAndStop(TitleLayoutActor *pActor, float frame, std::uint32_t layer);
void emitEffect(TitleLayoutActor *pActor, const char *pEffectName);
void deleteEffectAll(TitleLayoutActor *pActor);

[[nodiscard]] bool isDead(const TitleLayoutActor *pActor);

[[nodiscard]] bool testCorePadButtonA(int channel);
[[nodiscard]] bool testCorePadButtonB(int channel);

void startStageBGM(const char *name, bool prepare);
[[nodiscard]] bool isPreparedStageBgm();
void unlockStageBGM();
void stopStageBGM(int fadeFrames);
void startSystemSE(const char *name, int, int);
void startCSSound(const char *name, int, int);
void tryRumblePadMiddle(void *, int);

}  // namespace MR

}  // namespace smgpc::game::title
