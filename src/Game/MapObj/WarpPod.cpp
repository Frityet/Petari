#include "Game/MapObj/WarpPod.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActorGroup.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/DirectDrawUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <cstdio>

GXColor gGlowEffectEnvColor[] = {
    {0, 100, 200}, {44, 255, 42}, {255, 60, 60}, {196, 166, 0}, {0, 255, 0}, {255, 0, 255}, {255, 255, 0}, {255, 255, 255},
};

namespace {
    static f32 cSensorRadius0 = 120.0f;
    static f32 cSensorRadius1 = 15.0f;
};  // namespace

namespace MR {
    static u32 mDrawTimer;

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

    for (u32 i = 0; i < _10->getObjNum(); i++) {
        WarpPod* pWarpPod = static_cast< WarpPod* >(_10->getActor(i));

        if (pWarpPod == pParam1) {
            continue;
        }

        if (pWarpPod->_8C == nullptr) {
            continue;
        }

        if (*pWarpPod->_8C == *static_cast< const WarpPod* >(pParam1)->_8C) {
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
    for (u32 i = 0; i < _10->getObjNum(); i++) {
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
    f32 sensorRadius = mArg1 == 0 ? sensorRadiusCoef * ::cSensorRadius0 : sensorRadiusCoef * ::cSensorRadius1;

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

    // The retail getPairPod return leaves the matched zone ID in r4. The
    // original duplicated comparisons can leave that byte as the path flag.
    u8 isPath = static_cast< u8 >(mPairPod->_8C->mZoneID);
    if (mPairPod->mPosition.x > mPosition.x) {
        isPath = true;
    } else if (mPairPod->mPosition.x < mPosition.x) {
        isPath = false;
    } else if (mPairPod->mPosition.y < mPosition.y) {
        isPath = true;
    } else if (mPairPod->mPosition.y < mPosition.y) {
        isPath = false;
    } else if (mPairPod->mPosition.z < mPosition.z) {
        isPath = true;
    } else if (mPairPod->mPosition.z < mPosition.z) {
        isPath = false;
    }

    if (mPairPod->_CA != 1 && _CA != 1) {
        if (mArg3 == 0) {
            _CA = false;
        } else if (mPairPod->mArg3 == 0) {
            _CA = true;
        } else {
            _CA = isPath;
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

    // FIXME: Should not optimize to beqlr instruction.
    if (!mPairPod->_CC) {
        return;
    }

    MR::startEventCameraNoTarget(_94, mEventCameraName, -1);
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

    TVec3f axis;
    TVec3f centerDirection;
    TVec3f delta = mPairPod->mPosition - mPosition;
    f32 distance = delta.length();
    TVec3f up;
    MR::calcUpVec(&up, this);
    axis.cross(delta, -up);
    MR::normalizeOrZero(&axis);
    TVec3f midPoint = mPosition + delta * 0.5f;
    centerDirection.cross(axis, delta);
    MR::normalizeOrZero(&centerDirection);
    f32 halfAngle = PI / 4.0f;
    f32 radius = (0.5f * distance) / MR::sin(halfAngle);
    f32 centerDistance = MR::sqrt(radius * radius - 0.5f * (0.5f * distance * distance));
    TVec3f rotationAxis = axis;
    TVec3f center = midPoint + centerDirection * centerDistance;
    TVec3f start = -centerDirection * radius;
    f32 startAngle = -halfAngle;

    u16 remaining = 60;
    _C4 = new TVec3f[60];
    u32 pointCount = 60;
    _C8 = pointCount;
    for (u32 i = 0; i < 60; i++, remaining--) {
        f32 rate = (1.0f + MR::sin(((60 - remaining) - 0.5f * pointCount) / pointCount * PI)) * 0.5f;
        if (mArg1 == 2) {
            rate = 1.0f - static_cast< f32 >(remaining - 1) / pointCount;
        }
        Mtx rotation;
        PSMTXRotAxisRad(rotation, rotationAxis, startAngle * (1.0f - rate) + halfAngle * rate);
        TVec3f rotated;
        PSMTXMultVecSR(rotation, start, rotated);
        TVec3f position = center + rotated;
        _C4[i] = position + up * 200.0f;
    }

    _D4 = new JUTTexture(MR::getTexture(MR::getResourceHolder(this), "TestColor.bti"), 0);
    _D8 = new JUTTexture(MR::getTexture(MR::getResourceHolder(this), "TestMask.bti"), 0);
}

void WarpPod::drawCylinder(u32) const {
    f32 radius = 30.0f;
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
    GXSetTevColor(GX_TEVREG0, gGlowEffectEnvColor[mArg6]);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
    _D4->load(GX_TEXMAP0);
    _D8->load(GX_TEXMAP1);

    u32 pointCount = _C8;
    s32 timer = _A6;
    s32 duration = mArg5;
    if (timer == 0) {
        timer = mPairPod->_A6;
        duration = mPairPod->mArg5;
    }
    if (timer != 0) {
        pointCount = pointCount * (1.0f - static_cast< f32 >(timer) / duration);
    }

    TVec3f prevVertices[4];
    TVec3f nextVertices[4];
    TVec3f prevPosition;
    TVec2f nextTexCoords[2];
    TVec2f prevTexCoords[2];
    for (u32 i = 0; i < pointCount; i++) {
        if (i == 0) {
            u32 j = 1;
            for (; j < pointCount; j++) {
                TVec3f delta = _C4[j] - _C4[i];
                TVec3f direction;
                MR::vecKillElement(delta, MR::getCamZdir(), &direction);
                if (MR::normalizeOrZero(&direction)) {
                    continue;
                }
                TVec3f side;
                side.cross(direction, MR::getCamZdir());
                MR::normalizeOrZero(&side);
                TVec3f up;
                up.cross(side, direction);
                MR::normalizeOrZero(&up);
                side.setLength(radius);
                up.setLength(radius);
                prevVertices[0] = _C4[i] + side;
                prevVertices[1] = _C4[i] - side;
                prevVertices[2] = _C4[i] + up;
                prevVertices[3] = _C4[i] - up;
                break;
            }
            if (j >= pointCount) {
                return;
            }
            prevPosition = _C4[0];
            prevTexCoords[0].set(0.0f, 1.0f);
            prevTexCoords[1].set(1.0f, 1.0f);
        } else {
            f32 texCoord = 2.0f * (static_cast< f32 >(i + 1) / pointCount) - 1.0f;
            if (texCoord < 0.0f) {
                texCoord = -texCoord;
            }
            nextTexCoords[0].set(0.0f, texCoord);
            nextTexCoords[1].set(1.0f, texCoord);
            TVec3f delta = _C4[i] - prevPosition;
            TVec3f direction;
            MR::vecKillElement(delta, MR::getCamZdir(), &direction);
            if (MR::normalizeOrZero(&direction)) {
                continue;
            }
            TVec3f side;
            side.cross(direction, MR::getCamZdir());
            MR::normalizeOrZero(&side);
            TVec3f up;
            up.cross(side, direction);
            MR::normalizeOrZero(&up);
            side.setLength(radius);
            up.setLength(radius);
            nextVertices[0] = _C4[i] + side;
            nextVertices[1] = _C4[i] - side;
            nextVertices[2] = _C4[i] + up;
            nextVertices[3] = _C4[i] - up;
            GXBegin(GX_QUADS, GX_VTXFMT0, 8);
            MR::ddSendVtxData(prevVertices[0], prevTexCoords[0]);
            MR::ddSendVtxData(nextVertices[0], nextTexCoords[0]);
            MR::ddSendVtxData(nextVertices[1], nextTexCoords[1]);
            MR::ddSendVtxData(prevVertices[1], prevTexCoords[1]);
            MR::ddSendVtxData(prevVertices[2], prevTexCoords[0]);
            MR::ddSendVtxData(nextVertices[2], nextTexCoords[0]);
            MR::ddSendVtxData(nextVertices[3], nextTexCoords[1]);
            MR::ddSendVtxData(prevVertices[3], prevTexCoords[1]);
            GXEnd();
            prevVertices[0] = nextVertices[0];
            prevTexCoords[0] = nextTexCoords[0];
            prevVertices[1] = nextVertices[1];
            prevTexCoords[1] = nextTexCoords[1];
            prevVertices[2] = nextVertices[2];
            prevVertices[3] = nextVertices[3];
            prevPosition = _C4[i];
        }
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

        TDDraw::drawCylinder(_C4[i], v, 100.0f, 0x40406040, 0x40406040, 8);
    }
}
