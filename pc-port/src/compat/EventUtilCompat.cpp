#include "Game/Util/EventUtil.hpp"

#include "Game/Screen/InformationObserver.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/Util/PlayerUtil.hpp"

namespace MR {
    bool isPlayerLuigi() {
        return !GameDataFunction::isDataMario();
    }

    void explainEnableToSpin(LiveActor* actor) {
        InformationObserverFunction::explainSpin(actor);
    }

    void onGameEventFlagBeeMarioAtFirst() {
        GameDataFunction::onGameEventFlag("ハチマリオ初変身");
    }

    void onGameEventFlagTeresaMarioAtFirst() {
        GameDataFunction::onGameEventFlag("テレサマリオ初変身");
    }

    void onGameEventFlagHopperMarioAtFirst() {
        GameDataFunction::onGameEventFlag("ホッパーマリオ初変身");
    }

    void onGameEventFlagFireMarioAtFirst() {
        GameDataFunction::onGameEventFlag("ファイアマリオ初変身");
    }

    void onGameEventFlagIceMarioAtFirst() {
        GameDataFunction::onGameEventFlag("アイスマリオ初変身");
    }

    void onGameEventFlagFlyingMarioAtFirst() {
        GameDataFunction::onGameEventFlag("フライングマリオ初変身");
    }

    void onGameEventFlagInvincibleMarioAtFirst() {
        GameDataFunction::onGameEventFlag("無敵マリオ初変身");
    }

    void onGameEventFlagLifeUpAtFirst() {
        GameDataFunction::onGameEventFlag("ライフアップキノコ解説");
    }

    void onGameEventFlagOneUpAtFirst() {
        GameDataFunction::onGameEventFlag("１ＵＰキノコ解説");
    }

    s32 getPictureBookChapterCanRead() {
        return GameDataFunction::getPictureBookChapterCanRead();
    }

    s32 getPictureBookChapterAlreadyRead() {
        return GameDataFunction::getPictureBookChapterAlreadyRead();
    }

    void setPictureBookChapterAlreadyRead(int chapter_already_read) {
        GameDataFunction::setPictureBookChapterAlreadyRead(chapter_already_read);
    }

    bool isOnGameEventFlagEndTicoGuideDemo() {
        return GameDataFunction::isPassedStoryEvent("チコガイドデモ終了");
    }

    void onGameEventFlagEndTicoGuideDemo() {
        GameDataFunction::followStoryEventByName("チコガイドデモ終了");
    }

    void onGameEventFlagEnableToSpinAndStarPointer() {
        GameDataFunction::followStoryEventByName("スピン権利");
        MR::setPlayerSwingPermission(true);
    }
}  // namespace MR
