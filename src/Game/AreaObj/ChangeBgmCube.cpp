#include "Game/AreaObj/ChangeBgmCube.hpp"
#include "Game/AudioLib/AudBgm.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "Game/GameAudio/AudStageBgmTable.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

ChangeBgmCube::ChangeBgmCube(int formType, const char* pName) : AreaObj(formType, pName) {
    _3C = 0;
}

ChangeBgmCube::~ChangeBgmCube() {}

void ChangeBgmCube::init(const JMapInfoIter& rIter) {
    AreaObj::init(rIter);
    MR::connectToSceneAreaObj(this);
}

void ChangeBgmCube::movement() {
    bool isValid = false;
    if (mIsValid && _15 && mIsAwake) {
        isValid = true;
    }

    if (!isValid) {
        return;
    }

    if (MR::isCubeBgmChangeInvalid()) {
        return;
    }

    if (MR::isPowerStarGetDemoActive()) {
        mIsValid = false;
        return;
    }

    if (MR::isPlayingStageBgmID(0x02000014)) {
        mIsValid = false;
        return;
    }

    if (MR::isPlayingStageBgmID(0x0200003E)) {
        mIsValid = false;
        return;
    }

    if (MR::isPlayerDead()) {
        mIsValid = false;
        return;
    }

    if (MR::isStageStateScenarioOpeningCamera()) {
        return;
    }

    if (MR::isPlayingStageBgmID(0x0200000A)) {
        return;
    }

    if (MR::isPlayingStageBgmID(0x02000039)) {
        return;
    }

    if (MR::isPlayingStageBgmID(0x02000003)) {
        return;
    }

    if (!isInVolume(*MR::getPlayerPos())) {
        _3C = false;
        return;
    }

    if (_3C) {
        return;
    }

    s32 arg1 = mObjArg1;
    s32 arg2 = mObjArg2;

    switch (mObjArg0) {
    case 0:
        if (arg1 < 0) {
            MR::startCurrentStageBGM();
        }
        else {
            u32 bgmID = AudStageBgmTable::getBgmId(MR::getCurrentStageName(), arg1);
            if (bgmID != 0xFFFFFFFF) {
                if (AudWrap::getBgmMgr()->_10[0] == bgmID && MR::isPlayingStageBgm()) {
                    break;
                }

                AudWrap::startStageBgm(bgmID, false);

                if (MR::isEqualStageName("ReverseKingdomGalaxy") && bgmID == 0x1010012) {
                    MR::setCubeBgmChangeInvalid();
                }

                if (MR::isEqualStageName("CannonFleetGalaxy") && bgmID == 0x1010002) {
                    MR::setCubeBgmChangeInvalid();
                }

                if (MR::isEqualStageName("BattleShipGalaxy") && bgmID == 0x1010002) {
                    MR::setCubeBgmChangeInvalid();
                }
            }

            if (arg2 >= 0) {
                s32 state = AudStageBgmTable::getBgmState(MR::getCurrentStageName(), arg2);
                if (state >= 0) {
                    AudBgm* bgm = AudWrap::getStageBgm();
                    if (bgm != 0) {
                        bgm->changeTrackMuteState(state, 0);
                    }
                }
            }
        }
        break;
    case 1:
        if (mObjArg3 != 1) {
            if (MR::isGalaxyRedCometAppearInCurrentStage()) {
                break;
            }

            if (MR::isGalaxyBlackCometAppearInCurrentStage()) {
                break;
            }
        }

        if (arg1 < 0) {
            arg1 = 0x5A;
        }

        if (arg2 >= 0) {
            u32 bgmID = AudStageBgmTable::getBgmId(MR::getCurrentStageName(), arg2);
            if (bgmID != 0xFFFFFFFF) {
                if (AudWrap::getBgmMgr()->_10[0] == bgmID && MR::isPlayingStageBgm()) {
                    break;
                }

                AudWrap::setNextIdStageBgm(bgmID);
            }
        }

        MR::stopStageBGM(arg1);
        break;
    case 2:
        if (arg1 < 0) {
            AudBgm* bgm = AudWrap::getStageBgm();
            if (bgm == 0) {
                break;
            }

            s32 fadeFrame = 0x1E;
            if (arg2 >= 0) {
                fadeFrame = arg2;
            }

            bgm->changeTrackMuteState(0, fadeFrame);
        }
        else {
            s32 state = AudStageBgmTable::getBgmState(MR::getCurrentStageName(), arg1);
            if (state < 0) {
                break;
            }

            AudBgm* bgm = AudWrap::getStageBgm();
            if (bgm == 0) {
                break;
            }

            s32 fadeFrame = 0x1E;
            if (arg2 >= 0) {
                fadeFrame = arg2;
            }

            bgm->changeTrackMuteState(state, fadeFrame);
        }
        break;
    default:
        break;
    }

    _3C = true;
}
