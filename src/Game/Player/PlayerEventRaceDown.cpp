#include "compat/Cp932Literal.hpp"
#include "Game/Player/PlayerEventRaceDown.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

EventRaceDown::EventRaceDown() : EventSequence(16) {
    addEventOnTime(CP932("初期化"), static_cast< EventFunc1 >(&EventRaceDown::init), 0);
    addEventOnTime(CP932("MISSレイアウト開始"), static_cast< EventFunc1 >(&EventRaceDown::missLayoutOpen), 45);
    addEventOnTime(CP932("通常レイアウト消去"), (&EventRaceDown::closeDefaultLayout), 30);
    addEventOnTime(CP932("サウンドA"), static_cast< EventFunc1 >(&EventRaceDown::sound), 30);
    addEventOnTime(CP932("サウンドB"), static_cast< EventFunc1 >(&EventRaceDown::sound), 100);
    addEventOnTime(CP932("サウンドC"), static_cast< EventFunc1 >(&EventRaceDown::sound), 120);
    addEventInStatus(CP932("ワイプ開始"), static_cast< EventFunc1 >(&EventRaceDown::doCloseWipe),
                     static_cast< EventFunc2 >(&EventRaceDown::checkCloseWipeStart));
    addEventOnTime(CP932("残機を引く"), static_cast< EventFunc1 >(&EventRaceDown::decLeft), 120);
    addEventInPhase(CP932("ワイプ終了後"), static_cast< EventFunc1 >(&EventRaceDown::doWaitAfterWipe), 2);
}

void EventRaceDown::init(u16, u16) {
    MR::setCubeBgmChangeInvalid();
    MR::clearBgmQueue();
    MR::stopStageBGM(10);
    MR::stopSubBGM(10);
    playAnimation(CP932("レース負け"));
    playSound(CP932("声最終ダメージ"));
    playSound(CP932("最後の一撃"));
    MR::startPlayerDownWipe();
}

void EventRaceDown::sound(u16 eventFrame, u16 sequenceFrame) {
    switch (sequenceFrame) {
    case 30:
        MR::setSoundVolumeSetting(2, 20);
        MR::startSubBGM("BGM_MISS", false);
        break;
    case 100:
    case 120:
        break;
    }
}

bool EventRaceDown::checkCloseWipeStart(u16 sequenceFrame) {
    if (isMissLayoutClosed(sequenceFrame) && getPhase() == 1) {
        return true;
    } else {
        return false;
    }
}

void EventRaceDown::missLayoutOpen(u16 eventFrame, u16 sequenceFrame) {
    MR::startMissLayout();
    nextPhase();
}
