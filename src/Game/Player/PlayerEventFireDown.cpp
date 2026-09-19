#include "compat/Cp932Literal.hpp"
#include "Game/Player/PlayerEventFireDown.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

EventFireDown::EventFireDown() : EventSequence(16) {
    addEventOnTime(CP932("炎匀ｻ"), static_cast< EventFunc1 >(&EventFireDown::init), 0);
    addEventOnTime(CP932("通常レイアウト消去"), static_cast< EventFunc1 >(&EventFireDown::closeDefaultLayout), 100);
    addEventOnTime(CP932("サエンドA"), static_cast< EventFunc1 >(&EventFireDown::sound), 30);
    addEventOnTime(CP932("サエンドB"), static_cast< EventFunc1 >(&EventFireDown::sound2), 50);
    addEventInStatus(CP932("ワイプ開始"), static_cast< EventFunc1 >(&EventFireDown::doCloseWipe),
                     static_cast< EventFunc2 >(&EventSequence::isMissLayoutClosed));
    addEventOnTime(CP932("演出を引き"), static_cast< EventFunc1 >(&EventFireDown::decLeft), 120);
    addEventInPhase(CP932("ワイプ終了後"), static_cast< EventFunc1 >(&EventFireDown::doWaitAfterWipe), 2);
}

void EventFireDown::init(u16 eventFrame, u16 sequenceFrame) {
    MR::setCubeBgmChangeInvalid();
    MR::clearBgmQueue();
    MR::stopStageBGM(10);
    MR::stopSubBGM(10);
    MarioAccess::getPlayerActor()->changeGameOverAnimation();
    playSound(CP932("声最終ダメージ"));
    playSound(CP932("最後の一撃"));
    _28 = 20.0f;
    MR::startPlayerDownWipe();
    MR::startMissLayout();
}
void EventFireDown::sound(u16 eventFrame, u16 sequenceFrame) {
    MR::setSoundVolumeSetting(2, 20);
    MR::startSubBGM("BGM_MISS", false);
}

void EventFireDown::updateAfter() {
    setSpot(_28, _24);
}

void EventFireDown::sound2(u16 eventFrame, u16 sequenceFrame) {
    MR::setSoundVolumeSetting(7, 70);
}
