#include "Game/Util/NPCUtil.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/NPCFunction.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/RailUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Util/TalkUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include <cstdio>

namespace {
    static s32 sStarAppearSeStep = 103;
    static s32 sStarAppearSeStepCaretaker = 32;
    static s32 sStarAppearSeStepPenguinCoach = 95;
    static s32 sStarAppearSeStepTeresaRacer = 89;
    static s32 sStarAppearSeStepTrickRabbit = 22;
};  // namespace

namespace NrvTakeOutStar {
    NEW_NERVE(TakeOutStarNrvAnim, TakeOutStar, Anim);
    NEW_NERVE(TakeOutStarNrvDemo, TakeOutStar, Demo);
    NEW_NERVE(TakeOutStarNrvTerm, TakeOutStar, Term);
};  // namespace NrvTakeOutStar

namespace NrvFadeStarter {
    NEW_NERVE(FadeStarterNrvFade, FadeStarter, Fade);
    NEW_NERVE(FadeStarterNrvTerm, FadeStarter, Term);
};  // namespace NrvFadeStarter

namespace NrvDemoStarter {
    NEW_NERVE(DemoStarterNrvInit, DemoStarter, Init);
    NEW_NERVE(DemoStarterNrvFade, DemoStarter, Fade);
    NEW_NERVE(DemoStarterNrvWait, DemoStarter, Wait);
    NEW_NERVE(DemoStarterNrvTerm, DemoStarter, Term);
};  // namespace NrvDemoStarter

namespace {
    bool isReaction(u8 now, u8 next) {
        return !now && next;
    }
};  // namespace

namespace MR {
    bool getNPCItemData(NPCActorItem* pItem, s32 itemType) {
        return NPCFunction::getNPCItemData(pItem, itemType);
    }

    bool isNPCItemFileExist(const char* pName) {
        char path[0x100];
        snprintf(path, sizeof(path), "/ObjectData/%s.arc", pName);
        return MR::isFileExist(path, false);
    }

    void initDefaultPose(NPCActor* pActor, const JMapInfoIter& rIter) {
        MR::initDefaultPos(pActor, rIter);
        MR::makeQuatRotateDegree(&pActor->_A0, pActor->mRotation);
        pActor->setInitPose();
        pActor->setInitPose();
    }

    void turnPlayerToActor(const LiveActor* pActor, f32 rate) {
        LiveActor* player = MR::getPlayerDemoActor();
        TPos3f baseMtx(MR::getPlayerBaseMtx());

        if (MR::isPlayerInWaterMode()) {
            if (MR::isNearPlayer(pActor, 10.0f)) {
                if (!MR::isBckPlaying(player, "SwimWait")) {
                    MR::startBckPlayer("SwimWait", static_cast<const char*>(nullptr));
                }
            } else if (MR::faceToPoint(baseMtx, pActor->mPosition, rate)) {
                if (!MR::isBckPlaying(player, "SwimWait")) {
                    MR::startBckPlayer("SwimWait", static_cast<const char*>(nullptr));
                }
            } else {
                if (!MR::isBckPlaying(player, "WatchTurnSwim")) {
                    MR::startBckPlayer("WatchTurnSwim", static_cast<const char*>(nullptr));
                }

                MR::setPlayerBaseMtx(baseMtx);
            }
        } else if (MR::isOnGroundPlayer()) {
            if (MR::isNearPlayer(pActor, 10.0f)) {
                if (!MR::isBckPlaying(player, "Watch")) {
                    MR::startBckPlayer("Watch", static_cast<const char*>(nullptr));
                }
            } else if (MR::faceToPoint(baseMtx, pActor->mPosition, rate)) {
                if (!MR::isBckPlaying(player, "Watch")) {
                    MR::startBckPlayer("Watch", static_cast<const char*>(nullptr));
                }
            } else {
                if (!MR::isBckPlaying(player, "WatchTurn")) {
                    MR::startBckPlayer("WatchTurn", static_cast<const char*>(nullptr));
                }

                MR::setPlayerBaseMtx(baseMtx);
            }
        }

        TVec3f push = *MR::getPlayerCenterPos() - pActor->mPosition;

        if (PSVECMag(&push) < 100.0f && MR::isOnGroundPlayer()) {
            MR::vecKillElement(push, *MR::getPlayerGroundNormal(), &push);

            if (!MR::normalizeOrZero(&push)) {
                push.scale(10.0f);
                MR::pushPlayer(push);
            }
        }
    }

    void decidePose(NPCActor* pActor, const TVec3f& rUp, const TVec3f& rFront, const TVec3f& rPos, f32 posRate, f32 upRate, f32 frontRate) {
        MR::blendVec(&pActor->mPosition, pActor->mPosition, rPos, posRate);

        if (upRate == 0.0f && frontRate == 0.0f) {
            MR::makeQuatUpFront(&pActor->_A0, rUp, rFront);
        } else {
            MR::blendQuatUpFront(&pActor->_A0, rUp, rFront, upRate, frontRate);
        }
    }

    void setNPCActorPos(NPCActor* pActor, const char* pName) {
        TPos3f mtx;
        mtx.identity();
        MR::findNamePos(pName, mtx);
        pActor->setBaseMtx(mtx);
        mtx.getTrans(pActor->mPosition);
        MR::resetPosition(pActor);
        MR::onCalcShadowOneTimeAll(pActor);
    }

    void setNPCActorPos(NPCActor* pActor, const TVec3f& rPos) {
        pActor->mPosition.set(rPos);
        MR::resetPosition(pActor);
        MR::onCalcShadowOneTimeAll(pActor);
    }

    void setNPCActorPose(NPCActor* pActor, const TVec3f& rUp, const TVec3f& rFront, const TVec3f& rPos) {
        TPos3f mtx;
        MR::makeMtxUpFrontPos(&mtx, rFront, rUp, rPos);
        pActor->setBaseMtx(mtx);
        pActor->mPosition.set(rPos);
        MR::resetPosition(pActor);
        MR::onCalcShadowOneTimeAll(pActor);
    }

    void followRailPose(NPCActor* pActor, f32 upRate, f32 frontRate) {
        const TVec3f& railPos = MR::getRailPos(pActor);
        const TVec3f& railDir = MR::getRailDirection(pActor);
        TVec3f up = -pActor->mGravity;
        MR::decidePose(pActor, up, railDir, railPos, 1.0f, upRate, frontRate);
    }

    void followRailPoseOnGround(NPCActor* pActor, f32 frontRate) {
        MR::followRailPoseOnGround(pActor, pActor, frontRate);
    }

    void followRailPoseOnGround(NPCActor* pActor, const LiveActor* pRailActor, f32 frontRate) {
        TVec3f railPos(MR::getRailPos(pRailActor));
        TVec3f gravity(pActor->mGravity);
        TVec3f lineEnd(gravity);
        lineEnd.scale(1000.0f);
        TVec3f offset(gravity);
        offset.scale(10.0f);
        TVec3f lineStart = MR::getRailPos(pRailActor) - offset;

        MR::getFirstPolyOnLineToMap(&railPos, nullptr, lineStart, lineEnd);

        const TVec3f& railDir = MR::getRailDirection(pRailActor);
        TVec3f poseUp = -gravity;
        MR::decidePose(pActor, poseUp, railDir, railPos, 1.0f, frontRate, 1.0f);
    }

    void setDefaultPose(NPCActor* pActor) {
        pActor->setToDefault();
    }

    bool convertPosOnGround(TVec3f* pPos, const TVec3f& rUp) {
        Triangle tri;
        TVec3f hitPos;

        if (MR::getFirstPolyOnLineToMap(&hitPos, &tri, *pPos, rUp)) {
            *pPos = hitPos;
            return true;
        }

        return false;
    }

    void timeKeepDemoFadeIn() {
        MR::openWipeFade(-1);
    }

    void timeKeepDemoFadeOut() {
        MR::closeWipeFade(-1);
    }

    void startNPCTalkCamera(const TalkMessageCtrl* pCtrl, MtxPtr pActorMtx, f32 scale, s32 frames) {
        MR::startNPCTalkCamera(pCtrl, pActorMtx, MR::getPlayerBaseMtx(), scale, frames);
    }

    void startNPCTalkCamera(const TalkMessageCtrl* pCtrl, MtxPtr pActorMtx, MtxPtr pPlayerMtx, f32 scale, s32 frames) {
        TVec3f offset(MR::getMessageBalloonFollowOffset(pCtrl));

        if (MR::getMessageBalloonFollowMatrix(pCtrl) != nullptr) {
            pActorMtx = MR::getMessageBalloonFollowMatrix(pCtrl);
        }

        TVec3f playerUp;
        playerUp.set(pPlayerMtx[0][1], pPlayerMtx[1][1], pPlayerMtx[2][1]);
        TVec3f actorPos;
        actorPos.set(pActorMtx[0][3], pActorMtx[1][3], pActorMtx[2][3]);
        TVec3f playerPos;
        playerPos.set(pPlayerMtx[0][3], pPlayerMtx[1][3], pPlayerMtx[2][3]);

        if (MR::normalizeOrZero(&playerUp)) {
            playerUp.set(0.0f, 1.0f, 0.0f);
        }

        f32 dist = PSVECDistance(&playerPos, &actorPos);
        f32 cameraAngleFactor = JMath::sAtanTable.atan2_(1.0f, MR::cosDegree(67.5f));
        f32 y = MR::vecKillElement(actorPos - playerPos, playerUp, &playerPos);
        f32 cameraUp = (y + offset.y) / dist;
        cameraUp = MR::max(cameraUp / 0.75f, 0.0f);
        f32 cameraDist = MR::min((dist * cameraAngleFactor * cameraUp * 6.0f) * scale, 450.0f);
        f32 cameraHeight = (y / 900.0f) + offset.y;

        MR::startTalkCamera(actorPos, playerUp, cameraHeight, cameraDist, frames);
    }

    void endNPCTalkCamera(bool a1, s32 frames) {
        MR::endTalkCamera(a1, frames);
    }

    void initDefaultPosAndQuat(NPCActor* pActor, const JMapInfoIter& rIter) {
        MR::initDefaultPos(pActor, rIter);
        MR::makeQuatRotateDegree(&pActor->_A0, pActor->mRotation);
        pActor->setInitPose();
    }

    PartsModel* createNPCGoods(LiveActor* pActor, const char* pModelName, const char* pJointName) {
        PartsModel* model = nullptr;

        if (!MR::isNullOrEmptyString(pModelName)) {
            char path[0x100];
            snprintf(path, sizeof(path), "/ObjectData/%s.arc", pModelName);

            if (MR::isFileExist(path, false) && MR::isExistJoint(pActor, pJointName)) {
                model = MR::createPartsModelNpcAndFix(pActor, "グッズ", pModelName, pJointName);
                model->appear();

                if (MR::getLightNumMax(model) > 0) {
                    MR::initLightCtrl(model);
                }
            }
        }

        return model;
    }

    PartsModel* createIndirectNPCGoods(LiveActor* pActor, const char* pModelName, const char* pJointName) {
        PartsModel* model = nullptr;

        if (!MR::isNullOrEmptyString(pModelName)) {
            char path[0x100];
            snprintf(path, sizeof(path), "/ObjectData/%s.arc", pModelName);

            if (MR::isFileExist(path, false) && MR::isExistJoint(pActor, pJointName)) {
                model = MR::createPartsModelIndirectNpc(pActor, "グッズ", pModelName, MR::getJointMtx(pActor, pJointName));
                model->appear();

                if (MR::getLightNumMax(model) > 0) {
                    MR::initLightCtrl(model);
                }
            }
        }

        return model;
    }

    bool calcPlayerFaceStareVector(TVec3f* pVec, MtxPtr pActorMtx, MtxPtr pPlayerMtx) {
        TPos3f faceMtx;
        MR::calcPlayerJointMtx(&faceMtx, "Face0");

        TVec3f facePos;
        faceMtx.getTrans(facePos);

        faceMtx.set(pActorMtx);
        TVec3f actorPos;
        faceMtx.getTrans(actorPos);

        faceMtx.set(pPlayerMtx);
        TVec3f playerX(faceMtx[0][0], faceMtx[1][0], faceMtx[2][0]);
        TVec3f playerZ(faceMtx[0][2], faceMtx[1][2], faceMtx[2][2]);
        TVec3f playerY(faceMtx[0][1], faceMtx[1][1], faceMtx[2][1]);
        TVec3f toActor = facePos - actorPos;
        bool ret = true;

        if (playerZ.dot(toActor) < 0.0f) {
            f32 z = MR::vecKillElement(toActor, playerZ, &toActor);
            TVec3f adjust = playerZ;
            adjust.scale(2.0f);
            adjust.scale(z);
            toActor -= adjust;
            ret = false;
        }

        pVec->set(toActor);
        return ret;
    }

    bool calcPlayerFaceStarePos(TVec3f* pPos, MtxPtr pActorMtx, MtxPtr pPlayerMtx) {
        TVec3f actorPos;
        MR::extractMtxTrans(pActorMtx, &actorPos);
        bool ret = MR::calcPlayerFaceStareVector(pPos, pActorMtx, pPlayerMtx);
        pPos->add(actorPos);
        return ret;
    }

    bool isActionContinuous(const LiveActor* pActor) {
        return MR::getBckCtrl(pActor)->mAttribute == J3DFrameCtrl::EMode_NONE && !MR::isBckStopped(pActor);
    }

    bool isActionLoopedOrStopped(const LiveActor* pActor) {
        if (MR::getBckCtrl(pActor)->mAttribute == J3DFrameCtrl::EMode_NONE) {
            return MR::isBckStopped(pActor);
        }

        return MR::isBckLooped(pActor);
    }

    void invalidateLodCtrl(const NPCActor* pActor) {
        pActor->mLodCtrl->invalidate();
    }

    void startMoveAction(NPCActor* pActor) {
        if (MR::isExistRail(pActor)) {
            MR::adjustmentRailCoordSpeed(pActor, pActor->_10C, pActor->_110);
            MR::moveRailRider(pActor);

            if (pActor->_124) {
                MR::followRailPoseOnGround(pActor, pActor, pActor->_114);
            } else {
                MR::followRailPose(pActor, pActor->_114, pActor->_114);
            }

            if (MR::isRailReachedGoal(pActor)) {
                MR::reverseRailDirection(pActor);
            }
        }
    }

    bool tryStartTurnAction(NPCActor* pActor) {
        const char* action = nullptr;

        if (MR::isNearPlayer(pActor, pActor->mParam._4)) {
            if (pActor->mParam._0) {
                action = pActor->turnToPlayer(pActor->mParam._8, pActor->mParam._C, pActor->mParam._10) ? pActor->mParam._14 : pActor->mParam._18;
            } else {
                action = pActor->mParam._14;
            }
        } else if (pActor->mParam._0 || pActor->mParam._1) {
            action = pActor->turnToDefault(pActor->mParam._8) ? pActor->mParam._14 : pActor->mParam._18;
        } else {
            action = pActor->mParam._14;
        }

        if (MR::isNullOrEmptyString(action)) {
            return false;
        }

        return MR::tryStartAction(pActor, action);
    }

    bool tryStartTalkAction(NPCActor* pActor) {
        const char* action = nullptr;

        if (MR::isTalkTalking(pActor->mMsgCtrl)) {
            if (pActor->mParam._1 && !pActor->turnToPlayer(pActor->mParam._8, pActor->mParam._C, pActor->mParam._10)) {
                action = pActor->mParam._20;
            } else {
                action = pActor->mParam._1C;
            }
        } else {
            return MR::tryStartTurnAction(pActor);
        }

        if (MR::isNullOrEmptyString(action)) {
            return false;
        }

        return MR::tryStartAction(pActor, action);
    }

    bool tryStartMoveTalkAction(NPCActor* pActor) {
        TalkMessageCtrl* msg = pActor->mMsgCtrl;

        if (!MR::isExistRail(pActor)) {
            return MR::tryStartTalkAction(pActor);
        }

        const char* action = nullptr;
        bool talkingMove = false;

        if (MR::isTalkTalking(msg) && !MR::isShortTalk(msg)) {
            if (pActor->mParam._1 && !pActor->turnToPlayer(pActor->mParam._8, pActor->mParam._C, pActor->mParam._10)) {
                action = pActor->mParam._20;
            } else {
                action = pActor->mParam._1C;
            }
        } else {
            if (MR::isNearZero(pActor->_10C, 0.001f) && MR::isNearZero(MR::getRailCoordSpeed(pActor), 0.001f)) {
                return MR::tryStartTalkAction(pActor);
            }

            MR::startMoveAction(pActor);

            if (MR::isTalkTalking(msg)) {
                action = pActor->_120;
                talkingMove = true;
            } else {
                action = pActor->_11C;
            }
        }

        if (MR::isNullOrEmptyString(action)) {
            return false;
        }

        bool started = MR::tryStartAction(pActor, action);

        if (talkingMove) {
            MR::setBckRate(pActor, pActor->_118);
        } else {
            MR::setBckRate(pActor, 1.0f);
        }

        return started;
    }

    bool tryStartMoveTurnAction(NPCActor* pActor) {
        if (!MR::isExistRail(pActor)) {
            return MR::tryStartTurnAction(pActor);
        }

        MR::startMoveAction(pActor);

        if (MR::isNullOrEmptyString(pActor->_11C)) {
            return false;
        }

        return MR::tryStartAction(pActor, pActor->_11C);
    }

    bool tryStartReaction(NPCActor* pActor) {
        const char* reaction = nullptr;
        bool result = false;

        if (pActor->_128) {
            if (isReaction(pActor->_DD, pActor->_E2)) {
                reaction = pActor->_134;
            } else if (isReaction(pActor->_E0, pActor->_E5)) {
                reaction = pActor->_13C;
            } else if (isReaction(pActor->_DE, pActor->_E3)) {
                reaction = pActor->_130;
            } else if (pActor->_E4) {
                reaction = pActor->_138;
            }
        }

        if (!MR::isNullOrEmptyString(reaction)) {
            if (isReaction(pActor->_DD, pActor->_E2) || isReaction(pActor->_E0, pActor->_E5)) {
                MR::stopBck(pActor);
                MR::startAction(pActor, reaction);
                result = true;
            } else if (isReaction(pActor->_DE, pActor->_E3)) {
                result = MR::tryStartAction(pActor, reaction);
            } else if (pActor->_E4) {
                if ((pActor->_134 == nullptr || !MR::isActionStart(pActor, pActor->_134))
                    && (pActor->_13C == nullptr || !MR::isActionStart(pActor, pActor->_13C))
                    && (pActor->_130 == nullptr || !MR::isActionStart(pActor, pActor->_130))) {
                    if (isReaction(pActor->_DF, pActor->_E4)) {
                        result = MR::tryStartAction(pActor, reaction);
                    } else if (MR::isActionLoopedOrStopped(pActor)) {
                        MR::startAction(pActor, reaction);
                    }
                }
            }
        } else if (pActor->mScaleController != nullptr && pActor->mDelegator != nullptr) {
            if (isReaction(pActor->_DF, pActor->_E4) || isReaction(pActor->_DE, pActor->_E3)
                || isReaction(pActor->_DD, pActor->_E2) || isReaction(pActor->_E0, pActor->_E5)) {
                result = true;
            }
        }

        return result;
    }

    bool tryTalkNearPlayerAndStartTalkAction(NPCActor* pActor) {
        MR::tryStartTalkAction(pActor);
        return MR::tryTalkNearPlayer(pActor->mMsgCtrl);
    }

    bool tryTalkNearPlayerAndStartMoveTalkAction(NPCActor* pActor) {
        MR::tryStartMoveTalkAction(pActor);
        return MR::tryTalkNearPlayer(pActor->mMsgCtrl);
    }

    bool tryTalkNearPlayerAtEndAndStartTalkAction(NPCActor* pActor) {
        MR::tryStartTalkAction(pActor);
        return MR::tryTalkNearPlayerAtEnd(pActor->mMsgCtrl);
    }

    bool tryTalkNearPlayerAtEndAndStartMoveTalkAction(NPCActor* pActor) {
        MR::tryStartMoveTalkAction(pActor);
        return MR::tryTalkNearPlayerAtEnd(pActor->mMsgCtrl);
    }

    bool tryTalkForceAndStartMoveTalkAction(NPCActor* pActor) {
        MR::tryStartMoveTalkAction(pActor);
        return MR::tryTalkForce(pActor->mMsgCtrl);
    }

    bool tryTalkForceAtEndAndStartTalkAction(NPCActor* pActor) {
        MR::tryStartTalkAction(pActor);
        return MR::tryTalkForceAtEnd(pActor->mMsgCtrl);
    }

    bool tryStartReactionAndPushNerve(NPCActor* pActor, const Nerve* pNerve) {
        if (MR::tryStartReaction(pActor)) {
            pActor->pushNerve(pNerve);
            return true;
        }

        return false;
    }

    bool tryStartReactionAndPopNerve(NPCActor* pActor) {
        if (MR::tryStartReaction(pActor)) {
            pActor->pushNerve(pActor->popNerve());
            return false;
        }

        if (pActor->isScaleAnim()) {
            return false;
        }

        if (MR::isActionLoopedOrStopped(pActor)) {
            pActor->popNerve();
            return true;
        }

        return false;
    }

    bool tryChangeTalkActionRandom(NPCActor* pActor, const char* a1, const char* a2, const char* a3) {
        if (MR::getBckCtrl(pActor)->mAttribute != J3DFrameCtrl::EMode_LOOP || !MR::isBckLooped(pActor)) {
            return false;
        }

        switch (MR::getRandom(static_cast<s32>(0), static_cast<s32>(3))) {
        case 0:
            if (a1 != nullptr) {
                pActor->mParam._1C = a1;
            }
            break;
        case 1:
            if (a2 != nullptr) {
                pActor->mParam._1C = a2;
            }
            break;
        case 2:
            if (a3 != nullptr) {
                pActor->mParam._1C = a3;
            }
            break;
        }

        return true;
    }

    f32 calcFloatOffset(const NPCActor* pActor, f32 offset, f32 maxOffset) {
        f32 ret = MR::max(offset - 0.5f, 0.0f);
        TalkMessageCtrl* msg = pActor->mMsgCtrl;

        if (msg != nullptr && MR::isTalkTalking(msg) && !MR::isShortTalk(msg)) {
            TVec3f toPlayer = pActor->mPosition - *MR::getPlayerPos();
            TVec3f up;
            MR::getPlayerUpVec(&up);

            if (toPlayer.dot(up) > 0.0f && PSVECMag(&toPlayer) < 200.0f) {
                f32 base = ret;
                f32 wanted = MR::getLinerValueFromMinMax(PSVECMag(&toPlayer), 0.0f, 200.0f, maxOffset, 0.0f);
                ret = wanted;

                f32 limit = 0.5f + (5.0f + base);

                if (limit > wanted) {
                    ret = limit;
                }
            }
        }

        return ret;
    }

    void calcAndSetFloatBaseMtx(NPCActor* pActor, f32 offset) {
        TVec3f oldPos(pActor->mPosition);
        TVec3f floatOffset;
        pActor->_A0.getYDir(floatOffset);
        floatOffset.scale(offset);
        pActor->mPosition.add(floatOffset);
        pActor->calcAndSetBaseMtx();
        pActor->mPosition.set(oldPos);
    }
};  // namespace MR

TakeOutStar::TakeOutStar(NPCActor* pActor, const char* pActionName, const char* pAnimName, const Nerve* pNerve)
    : NerveExecutor("パワースター取り出しデモ実行者"), mActor(pActor), mNerve(pNerve), mActionName(pActionName), mAnimName(pAnimName) {
    mStarModel = MR::createPowerStarDemoModel(mActor, "パワースターデモモデル", pActor->getBaseMtx());
    mStarModel->makeActorDead();

    initNerve(&NrvTakeOutStar::TakeOutStarNrvAnim::sInstance);
}

bool TakeOutStar::takeOut() {
    if (isNerve(&NrvTakeOutStar::TakeOutStarNrvTerm::sInstance)) {
        return true;
    }

    updateNerve();

    return false;
}

bool TakeOutStar::isFirstStep() {
    return isNerve(&NrvTakeOutStar::TakeOutStarNrvAnim::sInstance) && MR::isFirstStep(this);
}

bool TakeOutStar::isLastStep() {
    return isNerve(&NrvTakeOutStar::TakeOutStarNrvTerm::sInstance);
}

void TakeOutStar::exeAnim() {
    if (MR::isFirstStep(this)) {
        if (mNerve != nullptr) {
            mActor->pushNerve(mNerve);
        } else {
            mActor->tryPushNullNerve();
        }

        mStarModel->appear();
        MR::invalidateClipping(mStarModel);
        MR::requestMovementOn(mStarModel);
        MR::startBck(mStarModel, mAnimName, nullptr);
        MR::startAction(mActor, mActionName);
    }

    s32 step = sStarAppearSeStep;

    if (MR::isEqualString(mAnimName, "TakeOutStarCaretaker")) {
        step = sStarAppearSeStepCaretaker;
    } else if (MR::isEqualString(mAnimName, "TakeOutStarTeresaRacer")) {
        step = sStarAppearSeStepTeresaRacer;
    } else if (MR::isEqualString(mAnimName, "TakeOutStarPenguinCoach")) {
        step = sStarAppearSeStepPenguinCoach;
    } else if (MR::isEqualString(mAnimName, "TakeOutStarTrickRabbit")) {
        step = sStarAppearSeStepTrickRabbit;
    }

    if (MR::isGreaterStep(this, step)) {
        if (MR::isInWater(mStarModel, TVec3f(0.0f, 0.0f, 0.0f))) {
            MR::startLevelSound(mStarModel, "SE_OJ_LV_POW_STAR_EXIST_W");
        } else {
            MR::startLevelSound(mStarModel, "SE_OJ_LV_POW_STAR_EXIST");
        }
    }

    if (MR::isAnyAnimOneTimeAndStopped(mActor, mActionName)) {
        setNerve(&NrvTakeOutStar::TakeOutStarNrvDemo::sInstance);
    }
}

void TakeOutStar::exeDemo() {
    if (MR::isFirstStep(this)) {
        TVec3f trans;
        MR::extractMtxTrans(MR::getJointMtx(mStarModel, "PowerStar"), &trans);
        MR::appearPowerStarContinueCurrentDemo(mActor, trans);
        mStarModel->kill();
    }

    if (MR::isEndPowerStarAppearDemo(mActor)) {
        MR::validateClipping(mStarModel);
        mActor->popNerve();
        setNerve(&NrvTakeOutStar::TakeOutStarNrvTerm::sInstance);
    }
}

void TakeOutStar::exeTerm() {}

FadeStarter::FadeStarter(NPCActor* pActor, s32 a2) : NerveExecutor("フェード開始制御"), mActor(pActor), _C(nullptr), _10(a2) {
    initNerve(&NrvFadeStarter::FadeStarterNrvFade::sInstance);
}

bool FadeStarter::update() {
    if (isNerve(&NrvFadeStarter::FadeStarterNrvTerm::sInstance)) {
        return true;
    }

    updateNerve();

    return false;
}

void FadeStarter::exeFade() {
    if (MR::isFirstStep(this)) {
        if (!mActor->isEmptyNerve()) {
            _C = mActor->popNerve();
        }

        mActor->tryPushNullNerve();
        MR::closeWipeFade(_10);
    }

    if (MR::isWipeActive()) {
        return;
    }

    mActor->popNerve();

    if (_C != nullptr) {
        mActor->pushNerve(_C);
        _C = nullptr;
    }

    MR::openWipeFade(_10);
    setNerve(&NrvFadeStarter::FadeStarterNrvTerm::sInstance);
}

void FadeStarter::exeTerm() {}

DemoStarter::DemoStarter(NPCActor* pActor) : NerveExecutor("デモ開始制御"), mActor(pActor) {
    initNerve(&NrvDemoStarter::DemoStarterNrvInit::sInstance);
}

bool DemoStarter::update() {
    updateNerve();

    return isNerve(&NrvDemoStarter::DemoStarterNrvTerm::sInstance);
}

void DemoStarter::start() {
    if (isNerve(&NrvDemoStarter::DemoStarterNrvInit::sInstance)) {
        setNerve(&NrvDemoStarter::DemoStarterNrvFade::sInstance);
    }
}

void DemoStarter::exeInit() {}

void DemoStarter::exeFade() {
    if (MR::isFirstStep(this)) {
        MR::invalidateClipping(mActor);
        MR::offPlayerControl();
        MR::closeWipeFade(-1);
    }

    if (MR::isWipeActive()) {
        return;
    }

    setNerve(&NrvDemoStarter::DemoStarterNrvWait::sInstance);
}

void DemoStarter::exeWait() {
    if (MR::isLessStep(this, 30)) {
        return;
    }

    if (MR::canStartDemo()) {
        setNerve(&NrvDemoStarter::DemoStarterNrvTerm::sInstance);
    }
}

void DemoStarter::exeTerm() {}
