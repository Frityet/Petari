#include "compat/Cp932Literal.hpp"
#include "Game/Player/PlayerEventGameOver.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

EventGameOver::EventGameOver() : EventSequence(16) {
    addEventOnTime(CP932("初期化"), static_cast< EventFunc1 >(&EventGameOver::init), 0);
    addEventOnTime(CP932("通常レイアウト消去"), static_cast< EventFunc1 >(&EventGameOver::closeDefaultLayout), 60);
    addEventOnTime(CP932("サウンドA"), static_cast< EventFunc1 >(&EventGameOver::sound), 90);
    addEventOnTime(CP932("レイアウト開始"), static_cast< EventFunc1 >(&EventGameOver::reqLayout), 160);
    addEventOnTime(CP932("終了"), static_cast< EventFunc1 >(&EventGameOver::finish), 420);
}

void EventGameOver::init(u16 eventFrame, u16 sequenceFrame) {
    MR::requestStartGameOverDemo();
    MR::setCubeBgmChangeInvalid();
    MR::clearBgmQueue();
    MR::stopStageBGM(10);
    MR::stopSubBGM(10);
    playSound(CP932("声最終ダメージ"));
    playSound(CP932("最後の一撃"));
    _28 = 20.0f;
}

void EventGameOver::sound(u16 eventFrame, u16 sequenceFrame) {
    switch (sequenceFrame) {
    case 90:
        MR::setSoundVolumeSetting(2, 30);
        MR::startSubBGM("BGM_GAMEOVER", false);
        break;
    }
}

void EventGameOver::updateAfter() {
}

void EventGameOver::reqLayout(u16 eventFrame, u16 sequenceFrame) {
    MR::startGameOverWipe();
}

void EventGameOver::finish(u16 eventFrame, u16 sequenceFrame) {
    MR::requestEndGameOverDemo();
    stopSequence();
}
