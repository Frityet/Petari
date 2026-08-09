#include "Game/Util/EventUtil.hpp"

#include "Game/Screen/InformationObserver.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/StageSessionState.hpp"

#include <stdexcept>

namespace {
    void require_message_bit(s8 bit) {
        if (bit < 0 || bit >= 16) {
            throw std::out_of_range("MessageAlreadyRead bit must fit the retail u16 event value.");
        }
    }
}  // namespace

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

    s32 setupAlreadyDoneFlag(const char* name, const JMapInfoIter& iter, u32* value) {
        if (name == nullptr) {
            throw std::invalid_argument("Already-done setup requires a name.");
        }

        auto link_id = s32{-1};
        static_cast<void>(MR::getJMapInfoLinkID(iter, &link_id));
        const auto name_hash = static_cast<u16>(MR::getHashCode(name) & 0x7fffU);
        return smgpc::compat::require_active_stage_session().setup_already_done_flag(
            name_hash, MR::getPlacedZoneId(iter), link_id, value);
    }

    void updateAlreadyDoneFlag(int index, u32 value) {
        smgpc::compat::require_active_stage_session().update_already_done_flag(index, value);
    }

    void onMessageAlreadyRead(s8 bit) {
        require_message_bit(bit);
        const auto value = GameDataFunction::getGameEventValue("MessageAlreadyRead");
        GameDataFunction::setGameEventValue(
            "MessageAlreadyRead", static_cast<u16>(value | (1U << static_cast<u8>(bit))));
    }

    bool isOnMessageAlreadyRead(s8 bit) {
        require_message_bit(bit);
        const auto value = GameDataFunction::getGameEventValue("MessageAlreadyRead");
        return (value & (1U << static_cast<u8>(bit))) != 0U;
    }

    void offMsgLedPattern() {
        GameDataFunction::setGameEventValue("MsgLedPattern", 0U);
    }

    bool isMsgLedPattern() {
        return static_cast<u16>(GameDataFunction::getGameEventValue("MsgLedPattern")) != 0U;
    }

    void onMsgLedPattern() {
        GameDataFunction::setGameEventValue("MsgLedPattern", 1U);
    }
}  // namespace MR
