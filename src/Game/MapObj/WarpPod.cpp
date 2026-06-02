#include "Game/MapObj/WarpPod.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActorGroup.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/DirectDrawUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include <cstdio>
#include <cstring>
#include <revolution/mtx.h>

struct ResTIMG;

class JUTTexture {
public:
    JUTTexture(const ResTIMG*, u8);
    void load(GXTexMapID);

private:
    u8 _0[0x40];
};

GXColor gGlowEffectEnvColor[] = {
    {0, 100, 200}, {44, 255, 42}, {255, 60, 60}, {196, 166, 0}, {0, 255, 0}, {255, 0, 255}, {255, 255, 0}, {255, 255, 255},
};

namespace {
    const f32 cSensorRadius0 = 120.0f;
    const f32 cSensorRadius1 = 15.0f;
    const u16 cPathPointCount = 60;
    const f32 cPathArcAngle = 0.7853982f;
    const f32 cPathHeight = 200.0f;
    const f32 cPathRibbonHalfWidth = 30.0f;
    const f32 cCylinderRadius = 100.0f;
    const u32 cCylinderColor = 0x40406040;

    bool isPairDrawSide(const WarpPod* pPod, const WarpPod* pPairPod) {
        if (pPairPod->mPosition.x > pPod->mPosition.x) {
            return true;
        }

        if (pPairPod->mPosition.x < pPod->mPosition.x) {
            return false;
        }

        if (pPairPod->mPosition.y < pPod->mPosition.y) {
            return true;
        }

        if (pPairPod->mPosition.y > pPod->mPosition.y) {
            return false;
        }

        return pPairPod->mPosition.z < pPod->mPosition.z;
    }
};  // namespace

namespace MR {
    static u32 mDrawTimer;

    f32 sin(f32);
    bool normalizeOrZero(TVec3f*);
    f32 vecKillElement(const TVec3f&, const TVec3f&, TVec3f*);
    const ResTIMG* getTexture(ResourceHolder*, const char*);

    WarpPodMgr* getWarpPodManager() {
        if (!isExistSceneObj(SceneObj_WarpPodMgr)) {
            return nullptr;
        }

        return getSceneObj< WarpPodMgr >(SceneObj_WarpPodMgr);
    }
};  // namespace MR

WarpPodMgr::WarpPodMgr(const char* pName) : NameObj(pName) {
    _10 = new LiveActorGroup("ワープポッド群", 128);
    _C = nullptr;
    _14 = 0;

    MR::connectToScene(this, -1, -1, -1, MR::DrawType_WarpPodPath);
}

WarpPod* WarpPodMgr::getPairPod(const LiveActor* pParam1) {
    if (static_cast< const WarpPod* >(pParam1)->_8C == nullptr) {
        return nullptr;
    }

    for (u32 i = 0; i < _10->getObjectCount(); i++) {
        WarpPod* pWarpPod = static_cast< WarpPod* >(_10->getActor(i));

        if (pWarpPod == pParam1) {
            continue;
        }

        if (pWarpPod->_8C == nullptr) {
            continue;
        }

        bool isSameJMapIdInfo = false;

        if (pWarpPod->_8C->_0 == static_cast< const WarpPod* >(pParam1)->_8C->_0
            && pWarpPod->_8C->mZoneID == static_cast< const WarpPod* >(pParam1)->_8C->mZoneID) {
            isSameJMapIdInfo = true;
        }

        if (isSameJMapIdInfo) {
            return pWarpPod;
        }
    }

    return nullptr;
}

void WarpPodMgr::startEventCamera(const LiveActor* pWarpPod) {
    static_cast< const WarpPod* >(pWarpPod)->startEventCamera();

    _C = pWarpPod;
}

void WarpPodMgr::endEventCamera() {
    WarpPod* pPairPod;

    if (_C == nullptr) {
        return;
    }

    const_cast< WarpPod* >(static_cast< const WarpPod* >(_C))->endEventCamera();

    pPairPod = getPairPod(_C);
    pPairPod->_A0 = 60;
    MR::startBck(pPairPod, "Wait", nullptr);
    MR::startBrk(pPairPod, "Wait");

    _C = nullptr;
}

void WarpPodMgr::notifyWarpEnd(WarpPod* pWarpPod) {
    WarpPod* pPairPod;

    if (pWarpPod == nullptr) {
        return;
    }

    pPairPod = getPairPod(pWarpPod);
    pPairPod->_A0 = 60;
    MR::startBck(pPairPod, "Wait", nullptr);
    MR::startBrk(pPairPod, "Wait");

    _C = nullptr;
}

void WarpPodMgr::draw() const {
    for (u32 i = 0; i < _10->getObjectCount(); i++) {
        static_cast< WarpPod* >(_10->getActor(i))->drawCylinder(MR::mDrawTimer);
    }

    MR::mDrawTimer++;
}

void WarpPod::init(const JMapInfoIter& rIter) {
    MR::createSceneObj(SceneObj_WarpPodMgr);
    MR::joinToGroup(this, "ワープポッド群");
    LiveActor::init(rIter);
    MR::initDefaultPos(this, rIter);
    initModelManagerWithAnm("WarpPod", nullptr, false);

    s32 groupID = -1;
    MR::getJMapInfoGroupID(rIter, &groupID);

    if (groupID >= 0) {
        _8C = new JMapIdInfo(groupID, rIter);
    }

    _90 = groupID;
    mArg1 = 1;
    mArg2 = -1;
    mArg3 = -1;
    mArg4 = -1;
    mArg5 = 120;
    mArg6 = 0;
    s32 arg7 = -1;

    if (MR::isValidInfo(rIter)) {
        MR::getJMapInfoArg1NoInit(rIter, &mArg1);
        MR::getJMapInfoArg2NoInit(rIter, &mArg2);
        MR::getJMapInfoArg3NoInit(rIter, &mArg3);
        MR::getJMapInfoArg4NoInit(rIter, &mArg4);
        MR::getJMapInfoArg5NoInit(rIter, &mArg5);
        MR::getJMapInfoArg6NoInit(rIter, &mArg6);
        MR::getJMapInfoArg7NoInit(rIter, &arg7);
    }

    if (arg7 == 1) {
        _CA = true;
    } else {
        _CA = false;
    }

    _94 = new ActorCameraInfo(rIter);

    s32 arg0;
    MR::getJMapInfoArg0WithInit(rIter, &arg0);

    char eventCameraName[256];
    sprintf(eventCameraName, "ワープカメラ %d-%c", groupID, arg0 + 65);
    MR::declareEventCamera(_94, eventCameraName);

    mEventCameraName = new char[strlen(eventCameraName) + 1];
    strcpy(mEventCameraName, eventCameraName);

    if (mArg1 == 0) {
        MR::connectToScene(this, MR::MovementType_MapObj, -1, -1, -1);
    } else {
        MR::connectToScene(this, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_MapObj, -1);
    }

    initSound(4, false);
    initHitSensor(1);

    f32 sensorRadiusCoef = mScale.x;
    f32 sensorRadius = mArg1 == 0 ? sensorRadiusCoef * cSensorRadius0 : sensorRadiusCoef * cSensorRadius1;

    MR::addHitSensorEye(this, "eye", 8, sensorRadius, TVec3f(0.0f, 0.0f, 0.0f));

    _A0 = 0;
    _A2 = 0;
    _A6 = 0;
    _CD = false;

    initEffectKeeper(1, nullptr, false);
    MR::validateClipping(this);
    MR::setClippingFarMax(this);
    makeActorAppeared();

    _A4 = 0;

    if (mArg1 != 0) {
        MR::startBck(this, "Active", nullptr);
        MR::startBrk(this, "Active");
    }

    bool isNonActive = mArg4 > MR::calcOpenedAstroDomeNum();

    if (mArg3 == 0) {
        mPathFlagIndex = MR::getWarpPodManager()->_14++;

        if (MR::isOnWarpPodPathFlag(mPathFlagIndex)) {
            isNonActive = false;
        } else {
            isNonActive = true;
        }
    } else if (!isNonActive) {
        glowEffect();
    }

    if (isNonActive) {
        MR::startBck(this, "Wait", nullptr);
        MR::startBrk(this, "Wait");

        _CB = true;
    } else {
        _CB = false;

        glowEffect();
    }

    _CC = false;

    mPairPod = MR::getWarpPodManager()->getPairPod(this);

    if (mPairPod != nullptr) {
        initPair();
        mPairPod->initPair();
    }

    MR::tryRegisterDemoCast(this, rIter);
}

void WarpPod::glowEffect() {
    if (mArg1 == 0) {
        return;
    }

    MR::emitEffect(this, "EndGlow");
    MR::setEffectEnvColor(this, "EndGlow", gGlowEffectEnvColor[mArg6].r, gGlowEffectEnvColor[mArg6].g, gGlowEffectEnvColor[mArg6].b);
}

void WarpPod::initPair() {
    mPairPod = MR::getWarpPodManager()->getPairPod(this);

    bool isDrawSide = isPairDrawSide(this, mPairPod);

    if (mPairPod->_CA != 1 && _CA != 1) {
        if (mArg3 == 0) {
            _CA = false;
        } else if (mPairPod->mArg3 == 0) {
            _CA = true;
        } else {
            _CA = isDrawSide;
        }
    }

    initDraw();

    if (!_CB) {
        return;
    }

    if (!_CA && mPairPod->_CB) {
        return;
    }

    char buf[256];
    sprintf(buf, "wPod出現カメラ %d", _90);

    _9C = new char[strlen(buf) + 1];
    strcpy(_9C, buf);

    MR::declareEventCamera(_94, _9C);
}

void WarpPod::appear() {
    if (_CB && mPairPod->_CB && !_CA) {
        mPairPod->appear();
        _CB = false;
    } else {
        _A6 = mArg5;

        MR::invalidateClipping(this);

        _CB = false;

        MR::startSound(this, "SE_OJ_WARP_POD_PATH_APPEAR");
        MR::startBck(this, "Active", nullptr);
        MR::startBrk(this, "Active");
        glowEffect();
    }
}

void WarpPod::appearWithDemo() {
    if (_CB) {
        if (mArg3 == 0) {
            MR::setWarpPodPathFlag(mPathFlagIndex, true);
        }

        if (mPairPod->_CB && !_CA) {
            mPairPod->appearWithDemo();
            _CB = false;
            return;
        }
    }

    _CD = true;

    MR::invalidateClipping(this);
}

void WarpPod::control() {
    if (!_CD) {
        return;
    }

    if (!MR::tryStartDemoWithoutCinemaFrame(this, "出現")) {
        return;
    }

    _CD = false;
    _A6 = mArg5;

    MR::startEventCameraNoTarget(_94, _9C, -1);
    MR::startSound(this, "SE_OJ_WARP_POD_PATH_APPEAR");
    MR::requestMovementOn(this);
    MR::pauseOffCameraDirector();

    _CB = false;
    _CC = true;

    MR::startBck(this, "Active", nullptr);
    MR::startBrk(this, "Active");
    glowEffect();
}

void WarpPod::movement() {
    if (_A6 != 0) {
        _A6--;

        if (_A6 == 0) {
            MR::validateClipping(this);

            if (_CC) {
                MR::endDemo(this, "出現");
            }

            mPairPod->glowEffect();
            MR::startBck(mPairPod, "Active", nullptr);
            MR::startBrk(mPairPod, "Active");
        }
    } else {
        if (_A0 != 0) {
            if (_A0 == 1 && MR::calcDistanceToPlayer(mPosition) < 200.0f) {
                return;
            }

            _A0--;

            if (_A0 == 0) {
                MR::validateClipping(this);
                MR::startBck(this, "Active", nullptr);
                MR::startBrk(this, "Active");

                if (mArg1 != 0) {
                    MR::startSound(this, "SE_OJ_WARP_POD_ACTIVE");
                }
            }
        }

        LiveActor::movement();

        _A4++;
    }
}

void WarpPod::startEventCamera() const {
    if (_CC) {
        return;
    }

    u8 isPairCameraStarted = mPairPod->_CC;

    if (isPairCameraStarted == 0) {
        MR::startEventCameraNoTarget(_94, mEventCameraName, -1);
    }
}

void WarpPod::endEventCamera() {
    if (_CC) {
        MR::endEventCamera(_94, _9C, true, -1);

        _CC = false;
    } else if (mPairPod->_CC) {
        mPairPod->endEventCamera();
    } else {
        MR::endEventCamera(_94, mEventCameraName, true, -1);
    }
}

void WarpPod::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (_CB) {
        return;
    }

    if (!MR::isSensorPlayer(pReceiver)) {
        return;
    }

    if (mPairPod->_CB) {
        mPairPod->appearWithDemo();
        MR::invalidateClipping(this);
    } else if (_A0 != 0) {
        if (_A0 < 30) {
            _A0 = 30;
        }
    } else if (MR::sendArbitraryMsg(ACTMES_WARP, pReceiver, pSender)) {
        _A2 = 60;
    }
}

void WarpPod::initDraw() {
    if (!_CA) {
        return;
    }

    TVec3f pairOffset = mPairPod->mPosition - mPosition;
    f32 distance = PSVECMag(&pairOffset);

    TVec3f upVec;
    MR::calcUpVec(&upVec, this);

    TVec3f reverseUp = -upVec;
    TVec3f sideVec;
    PSVECCrossProduct(&pairOffset, &reverseUp, &sideVec);
    MR::normalizeOrZero(&sideVec);

    TVec3f halfOffset(pairOffset);
    halfOffset.scale(0.5f);
    TVec3f centerBase = mPosition + halfOffset;

    TVec3f bendVec;
    PSVECCrossProduct(&sideVec, &pairOffset, &bendVec);
    MR::normalizeOrZero(&bendVec);

    f32 arcAngle = cPathArcAngle;
    f32 halfDistance = 0.5f * distance;
    f32 radius = halfDistance / MR::sin(arcAngle);
    f32 centerDistanceSqr = radius * radius - halfDistance * distance * 0.5f;
    f32 centerDistance = centerDistanceSqr;

    if (centerDistanceSqr > 0.0f) {
        f32 recip = __frsqrte(centerDistanceSqr);
        f32 value = recip * centerDistanceSqr;
        centerDistance = -((value * recip) - 3.0f) * value * 0.5f;
    }

    TVec3f arcAxis(sideVec);
    TVec3f centerOffset(bendVec);
    centerOffset.scale(centerDistance);
    TVec3f arcCenter = centerBase + centerOffset;

    TVec3f startOffset = -bendVec;
    startOffset.scale(radius);
    f32 negArcAngle = -arcAngle;

    u16 pathPointCount = cPathPointCount;
    _C4 = new TVec3f[pathPointCount];
    _C8 = pathPointCount;

    u16 remaining = pathPointCount;
    TVec3f* pathPoint = _C4;

    for (u16 i = 0; i < pathPointCount; i++, remaining--, pathPoint++) {
        f32 ratio = (static_cast< f32 >(pathPointCount) - 0.5f * static_cast< f32 >(pathPointCount - remaining)) / static_cast< f32 >(pathPointCount);
        ratio = (1.0f + MR::sin(ratio * 3.1415927f)) * 0.5f;

        if (mArg1 == 2) {
            ratio = 1.0f - static_cast< f32 >(remaining - 1) / static_cast< f32 >(pathPointCount);
        }

        f32 angle = negArcAngle * (1.0f - ratio) + arcAngle * ratio;

        Mtx rotation;
        PSMTXRotAxisRad(rotation, &arcAxis, angle);

        TVec3f rotatedOffset;
        PSMTXMultVecSR(rotation, &startOffset, &rotatedOffset);

        TVec3f point = arcCenter + rotatedOffset;
        TVec3f upOffset(upVec);
        upOffset.scale(cPathHeight);

        *pathPoint = point + upOffset;
    }

    mColorTexture = new JUTTexture(MR::getTexture(MR::getResourceHolder(this), "TestColor.bti"), 0);
    mMaskTexture = new JUTTexture(MR::getTexture(MR::getResourceHolder(this), "TestMask.bti"), 0);
}

void WarpPod::drawCylinder(u32 drawTimer) const {
    if (!_CA) {
        return;
    }

    if (mPairPod->_CB) {
        return;
    }

    if (_CB) {
        return;
    }

    if (mArg1 != 1) {
        return;
    }

    TDDraw::setup(0, 1, 0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    MR::ddSetVtxFormat(2);
    MR::ddLightingOff();

    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_ONE, GX_CC_TEXA, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_KONST, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);

    GXColor color = gGlowEffectEnvColor[mArg6];
    GXSetTevColor(GX_TEVREG0, color);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);

    mColorTexture->load(GX_TEXMAP0);
    mMaskTexture->load(GX_TEXMAP1);

    f32 ribbonHalfWidth = cPathRibbonHalfWidth;
    u32 pointCount = _C8;
    u16 timer = _A6;
    s32 appearTime = mArg5;

    if (timer == 0) {
        timer = mPairPod->_A6;
        appearTime = mPairPod->mArg5;
    }

    if (timer != 0) {
        f32 rate = 1.0f - static_cast< f32 >(timer) / static_cast< f32 >(appearTime);
        pointCount = static_cast< u32 >(static_cast< f32 >(pointCount) * rate);
    }

    TVec3f prevPoint;
    TVec3f prevSideAPlus;
    TVec3f prevSideAMinus;
    TVec3f prevSideBPlus;
    TVec3f prevSideBMinus;
    TVec2f prevTexLeft;
    TVec2f prevTexRight;

    for (u32 i = 0; i < pointCount; i++) {
        if (i == 0) {
            u32 startIndex = 1;

            for (; startIndex < pointCount; startIndex++) {
                TVec3f segment = _C4[startIndex] - _C4[0];
                TVec3f camZ = MR::getCamZdir();
                TVec3f forward;
                MR::vecKillElement(segment, camZ, &forward);

                if (MR::normalizeOrZero(&forward)) {
                    continue;
                }

                TVec3f camZCross = MR::getCamZdir();
                TVec3f sideA;
                PSVECCrossProduct(&forward, &camZCross, &sideA);
                MR::normalizeOrZero(&sideA);

                TVec3f sideB;
                PSVECCrossProduct(&sideA, &forward, &sideB);
                MR::normalizeOrZero(&sideB);

                sideA.setLength(ribbonHalfWidth);
                sideB.setLength(ribbonHalfWidth);

                prevSideAPlus = _C4[0] + sideA;
                prevSideAMinus = _C4[0] - sideA;
                prevSideBPlus = _C4[0] + sideB;
                prevSideBMinus = _C4[0] - sideB;
                break;
            }

            if (startIndex >= pointCount) {
                return;
            }

            prevPoint = _C4[0];
            prevTexLeft.x = 0.0f;
            prevTexLeft.y = 1.0f;
            prevTexRight.x = 1.0f;
            prevTexRight.y = 1.0f;
            continue;
        }

        f32 texY = 2.0f * static_cast< f32 >(i + 1) / static_cast< f32 >(pointCount) - 1.0f;

        if (texY < 0.0f) {
            texY = -texY;
        }

        TVec2f texLeft(0.0f, texY);
        TVec2f texRight(1.0f, texY);

        TVec3f segment = _C4[i] - prevPoint;
        TVec3f camZ = MR::getCamZdir();
        TVec3f forward;
        MR::vecKillElement(segment, camZ, &forward);

        if (MR::normalizeOrZero(&forward)) {
            continue;
        }

        TVec3f camZCross = MR::getCamZdir();
        TVec3f sideA;
        PSVECCrossProduct(&forward, &camZCross, &sideA);
        MR::normalizeOrZero(&sideA);

        TVec3f sideB;
        PSVECCrossProduct(&sideA, &forward, &sideB);
        MR::normalizeOrZero(&sideB);

        sideA.setLength(ribbonHalfWidth);
        sideB.setLength(ribbonHalfWidth);

        TVec3f sideAPlus = _C4[i] + sideA;
        TVec3f sideAMinus = _C4[i] - sideA;
        TVec3f sideBPlus = _C4[i] + sideB;
        TVec3f sideBMinus = _C4[i] - sideB;

        GXBegin(GX_QUADS, GX_VTXFMT0, 8);
        MR::ddSendVtxData(prevSideAPlus, prevTexLeft);
        MR::ddSendVtxData(sideAPlus, texLeft);
        MR::ddSendVtxData(sideAMinus, texRight);
        MR::ddSendVtxData(prevSideAMinus, prevTexRight);
        MR::ddSendVtxData(prevSideBPlus, prevTexLeft);
        MR::ddSendVtxData(sideBPlus, texLeft);
        MR::ddSendVtxData(sideBMinus, texRight);
        MR::ddSendVtxData(prevSideBMinus, prevTexRight);

        prevSideAPlus = sideAPlus;
        prevTexLeft.x = texLeft.x;
        prevTexLeft.y = texLeft.y;
        prevSideAMinus = sideAMinus;
        prevTexRight.x = texRight.x;
        prevTexRight.y = texRight.y;
        prevSideBPlus = sideBPlus;
        prevSideBMinus = sideBMinus;
        prevPoint = _C4[i];
    }
}

void WarpPod::draw() const {
    if (mArg1 == 0) {
        return;
    }

    if (!_CA) {
        return;
    }

    if (mPairPod->_CB) {
        return;
    }

    if (_CB) {
        return;
    }

    TDDraw::setup(0, 1, 0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);

    for (u32 i = 0; i < _C8; i++) {
        TVec3f v = _C4[i + 1] - _C4[i];

        TDDraw::drawCylinder(_C4[i], v, cCylinderRadius, cCylinderColor, cCylinderColor, 8);
    }
}
