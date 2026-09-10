from pathlib import Path
p=Path('decomp/src/Game/Util/NPCUtil.cpp');s=p.read_text()
s=s.replace('#include "Game/Util/ActorMovementUtil.hpp"','#include "Game/Util/ActorMovementUtil.hpp"\n#include "Game/Util/ActorShadowUtil.hpp"\n#include "Game/Util/CameraUtil.hpp"')
s=s.replace('#include "Game/Util/StringUtil.hpp"','#include "Game/Util/StringUtil.hpp"\n#include "Game/Util/TalkUtil.hpp"\n#include <JSystem/JMath/JMATrigonometric.hpp>')
pose='''    void setNPCActorPos(NPCActor* pActor, const char* pName) {
        TPos3f mtx;
        mtx.identity();
        findNamePos(pName, mtx.toMtxPtr());
        pActor->setBaseMtx(mtx);
        mtx.getTrans(pActor->mPosition);
        resetPosition(pActor);
        onCalcShadowOneTimeAll(pActor);
    }

'''
cam='''    void startNPCTalkCamera(const TalkMessageCtrl* pTalkCtrl, MtxPtr pActorMtx, f32 scale, s32 frame) {
        startNPCTalkCamera(pTalkCtrl, pActorMtx, getPlayerBaseMtx(), scale, frame);
    }

    void startNPCTalkCamera(const TalkMessageCtrl* pTalkCtrl, MtxPtr pActorMtx, MtxPtr pPlayerMtx, f32 scale, s32 frame) {
        TVec3f offset(getMessageBalloonFollowOffset(pTalkCtrl));
        if (getMessageBalloonFollowMatrix(pTalkCtrl) != nullptr) {
            pActorMtx = getMessageBalloonFollowMatrix(pTalkCtrl);
        }

        TVec3f up(pPlayerMtx[0][1], pPlayerMtx[1][1], pPlayerMtx[2][1]);
        TVec3f position(pActorMtx[0][3], pActorMtx[1][3], pActorMtx[2][3]);
        TVec3f playerPosition(pPlayerMtx[0][3], pPlayerMtx[1][3], pPlayerMtx[2][3]);
        if (normalizeOrZero(&up)) {
            up.set(0.0f, 1.0f, 0.0f);
        }

        f32 distance = PSVECDistance(&playerPosition, &position);
        f32 angle = JMAATan2(1.0f, JMACosDegree(67.5f));
        TVec3f horizontal;
        f32 height = vecKillElement(position - playerPosition, up, &horizontal);
        f32 distanceRate = max(((height + offset.y) / distance) / 0.75f, 1.0f);
        f32 axisX = height / 6.0f + offset.y;
        f32 axisY = max(2.0f * (distance * angle * distanceRate) * scale, 450.0f);
        startTalkCamera(position, up, axisX, axisY, frame);
    }

'''
turn='''    bool tryStartMoveTurnAction(NPCActor* pActor) {
        if (!isExistRail(pActor)) {
            return tryStartTurnAction(pActor);
        }

        startMoveAction(pActor);
        if (isNullOrEmptyString(pActor->_11C)) {
            return false;
        }

        return tryStartAction(pActor, pActor->_11C);
    }

'''
near='''    bool tryTalkNearPlayerAtEndAndStartTalkAction(NPCActor* pActor) {
        tryStartTalkAction(pActor);
        return tryTalkNearPlayerAtEnd(pActor->mMsgCtrl);
    }

'''
force='''    bool tryTalkForceAndStartMoveTalkAction(NPCActor* pActor) {
        tryStartMoveTalkAction(pActor);
        return tryTalkForce(pActor->mMsgCtrl);
    }

'''
for anchor,body in [('    void followRailPose(NPCActor*',pose),('    bool isActionLoopedOrStopped(',cam),('    bool tryStartReaction(NPCActor*',turn),('    bool tryTalkNearPlayerAtEndAndStartMoveTalkAction(',near),('    bool tryStartReactionAndPushNerve(',force)]:
 assert s.count(anchor)==1,anchor;s=s.replace(anchor,body+anchor)
p.write_text(s)
Path('notes/original-rabbit-actors-20260910/actors/providers/recovered-npc-functions.cpp').write_text(pose+cam+turn+near+force)
