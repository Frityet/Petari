#include "compat/Cp932Literal.hpp"
#include "Game/Player/PlayerEventAbyss.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

EventAbyss::EventAbyss() : EventSequence(12) {
    addEventOnTime(CP932("初期化"), static_cast< EventFunc1 >(&EventAbyss::init), 0);
    addEventOnTime(CP932("通常レイアウト消去"), static_cast< EventFunc1 >(&EventAbyss::closeDefaultLayout), 25);
    addEventOnTime(CP932("サウンドA"), static_cast< EventFunc1 >(&EventAbyss::sound), 30);
    addEventInStatus(CP932("ワイプ開始"), static_cast< EventFunc1 >(&EventAbyss::doCloseWipe), static_cast< EventFunc2 >(&EventAbyss::isMissLayoutClosed));
    addEventOnTime(CP932("残機を引く"), static_cast< EventFunc1 >(&EventAbyss::decLeft), 120);
    addEventInPhase(CP932("ワイプ終了後"), static_cast< EventFunc1 >(&EventAbyss::doWaitAfterWipe), 2);
}

void EventAbyss::init(u16 eventFrame, u16 sequenceFrame) {
    MR::requestStartGameOverDemo();
    playSound(CP932("声落下死亡"));
    MR::setCubeBgmChangeInvalid();
    MR::clearBgmQueue();
    MR::stopStageBGM(10);
    MR::stopSubBGM(10);
    MR::startMissLayout();
}

void EventAbyss::sound(u16 eventFrame, u16 sequenceFrame) {
    MR::setSoundVolumeSetting(2, 20);
    MR::startSubBGM("BGM_MISS", false);
}

void EventAbyss::updateAfter() {
    setSpot(_28, _24);
}
