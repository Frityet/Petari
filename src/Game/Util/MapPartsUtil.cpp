#include "Game/Util/MapPartsUtil.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/MapObj/MapPartsRailGuideHolder.hpp"
#include "Game/MapObj/MapPartsRailMover.hpp"
#include "Game/MapObj/MapPartsRailPointPassChecker.hpp"
#include "Game/MapObj/MapPartsRailRotator.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/RailUtil.hpp"
#include <cstdio>

namespace {
    const char* cFollowJointName = "Move";

    bool getJMapInfoArgNoInit(const JMapInfoIter&, const char*, s32*);
    bool getJMapInfoArgNoInit(const JMapInfoIter&, const char*, f32*);
}  // namespace

namespace MR {
    const HitSensor* getBodySensor(LiveActor* pActor) {
        return pActor->getSensor("body");
    }

    void setBodySensorType(LiveActor* pActor, u32 type) {
        pActor->getSensor("body")->setType(type);
    }

    void setBodySensorTypeMapObj(LiveActor* pActor) {
        setBodySensorType(pActor, 0x46);
    }

    void setBodySensorTypePress(LiveActor* pActor) {
        setBodySensorType(pActor, 0x76);
    }

    void setBodySensorTypeMoveCollision(LiveActor* pActor) {
        setBodySensorType(pActor, 0x48);
    }

    bool isBodySensorTypeMapObj(const LiveActor* pActor) {
        return pActor->getSensor("body")->isType(0x46);
    }

    bool receiveMapPartsRotateMsg(LiveActor* pActor, u32 msg, MapPartsRailMover* pMover, MapPartsRailRotator* pRotator) {
        if (msg == 0xCB) {
            return tryStartMapPartsRotateAtPoint(pActor, pMover, pRotator);
        }
        if (msg == 0xCC) {
            return tryEndMapPartsRotateAtPoint(pActor, pMover, pRotator);
        }
        if (msg == 0xCD) {
            return tryStartMapPartsRotateBetweenPoints(pActor, pMover, pRotator);
        }
        return false;
    }

    bool tryStartMapPartsRotateAtPoint(LiveActor* pActor, MapPartsRailMover*, MapPartsRailRotator* pRotator) {
        if (pRotator == nullptr) {
            return false;
        }
        s32 point = getCurrentRailPointNo(pActor);
        if (!pRotator->hasRotation(point)) {
            return false;
        }
        if (pRotator->hasRotationBetweenPoints(point)) {
            return false;
        }
        pRotator->rotateAtPoint(point);
        return true;
    }

    bool tryEndMapPartsRotateAtPoint(LiveActor*, MapPartsRailMover* pMover, MapPartsRailRotator* pRotator) {
        if (pMover == nullptr) {
            return false;
        }
        if (pRotator == nullptr) {
            return false;
        }
        pMover->endRotateAtPoint();
        return true;
    }

    bool tryStartMapPartsRotateBetweenPoints(LiveActor* pActor, MapPartsRailMover* pMover, MapPartsRailRotator* pRotator) {
        if (pMover == nullptr) {
            return false;
        }
        if (pRotator == nullptr) {
            return false;
        }
        if (!pRotator->hasRotationBetweenPoints(getCurrentRailPointNo(pActor))) {
            return false;
        }
        f32 time = 0.0f;
        pMover->calcTimeToNextRailPoint(&time);
        pRotator->rotateBetweenPoints(getCurrentRailPointNo(pActor), time);
        return true;
    }

    bool isMapPartsRailMovePassedStartPointRepeat(const MapPartsRailMover* pMover) {
        if (pMover->mMoveStopType == 2) {
            return pMover->mRailPointPassChecker->isPassedStartPoint();
        }
        return false;
    }

    bool isMapPartsRailMovePassedEndPointRepeat(const MapPartsRailMover* pMover) {
        if (pMover->mMoveStopType == 2) {
            return pMover->mRailPointPassChecker->isPassedEndPoint();
        }
        return false;
    }

    void getMapPartsObjectName(char* pName, u32 size, const JMapInfoIter& rIter) {
        const char* objectName = "";
        getObjectName(&objectName, rIter);
        s32 shapeId = -1;
        getJMapInfoShapeIdWithInit(rIter, &shapeId);
        snprintf(pName, size, "%s%02d", objectName, shapeId);
    }

    void getMapPartsObjectNameIfExistShapeID(char* pName, u32 size, const JMapInfoIter& rIter) {
        const char* objectName = "";
        getObjectName(&objectName, rIter);
        s32 shapeId = -1;
        getJMapInfoShapeIdWithInit(rIter, &shapeId);
        if (shapeId >= 0) {
            snprintf(pName, size, "%s%02d", objectName, shapeId);
        } else {
            snprintf(pName, size, "%s", objectName);
        }
    }

    void getMapPartsObjectName(char* pName, u32 size, const char* pObjectName, s32 modelNo) {
        snprintf(pName, size, "%s%02d", pObjectName, modelNo);
    }

    void initMapPartsClipping(LiveActor* pActor, const JMapInfoIter& rIter, TVec3f* pCenter, bool noRailClipping) {
        f32 radius = 0.0f;
        if (getJ3DModel(pActor) != nullptr) {
            calcModelBoundingRadius(&radius, pActor);
            if (isNearZero(radius)) {
                radius = getCollisionBoundingSphereRange(pActor);
            }
        } else {
            radius = getCollisionBoundingSphereRange(pActor);
        }
        if (isConnectedWithRail(rIter) && !noRailClipping) {
            initAndSetRailClipping(pCenter, pActor, 0.5f * radius, radius);
        } else {
            setClippingTypeSphere(pActor, radius);
        }
        setGroupClipping(pActor, rIter, 64);
        s32 farClip = -1;
        ::getJMapInfoArgNoInit(rIter, "FarClip", &farClip);
        if (farClip != -1) {
            if (farClip == 0) {
                setClippingFarMax(pActor);
            } else {
                setClippingFar(pActor, farClip);
            }
        }
    }

    bool isMapPartsPressOn(const JMapInfoIter& rIter) {
        s32 pressType = -1;
        ::getJMapInfoArgNoInit(rIter, "PressType", &pressType);
        return pressType == 1;
    }

    MapPartsRailGuideDrawer* createMapPartsRailGuideDrawer(LiveActor* pActor, const char* pName, const JMapInfoIter& rIter) {
        MapPartsRailGuideHolder* holder = static_cast< MapPartsRailGuideHolder* >(createSceneObj(SceneObj_MapPartsRailGuideHolder));
        return holder->createRailGuide(pActor, pName, rIter);
    }

    void initMapPartsShadow(LiveActor* pActor, const JMapInfoIter& rIter) {
        s32 shadowType = 0;
        ::getJMapInfoArgNoInit(rIter, "ShadowType", &shadowType);
        if (shadowType != 0) {
            if (shadowType == 2) {
                TBox3f box;
                calcModelBoundingBox(&box, pActor);
                TVec3f size;
                JMathInlineVEC::PSVECSubtract(&box.f, &box.i, &size);
                initShadowVolumeBox(pActor, size, pActor->getBaseMtx());
            } else {
                initShadowVolumeSphere(pActor, 0.70710677f * pActor->getSensor("body")->mRadius);
            }
            if (isExistJoint(pActor, cFollowJointName)) {
                TVec3f offset(0.0f, 0.0f, 0.0f);
                setShadowDropPositionAtJoint(pActor, nullptr, cFollowJointName, offset);
            }
        }
    }
}  // namespace MR

namespace {
    bool getJMapInfoArgNoInit(const JMapInfoIter& rIter, const char* pName, s32* pValue) {
        s32 value;
        if (!rIter.getValue(pName, &value)) {
            return false;
        }
        if (value != -1) {
            *pValue = value;
            return true;
        }
        return false;
    }

    bool getJMapInfoArgNoInit(const JMapInfoIter& rIter, const char* pName, f32* pValue) {
        s32 value;
        if (!getJMapInfoArgNoInit(rIter, pName, &value)) {
            return false;
        }
        *pValue = value;
        return true;
    }
}  // namespace

namespace MR {
    bool getMapPartsArgMoveConditionType(s32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "MoveConditionType", pValue);
    }

    bool getMapPartsArgRotateSpeed(f32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "RotateSpeed", pValue);
    }

    bool getMapPartsArgRotateTime(s32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "RotateSpeed", pValue);
    }

    bool getMapPartsArgRotateAngle(f32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "RotateAngle", pValue);
    }

    bool getMapPartsArgRotateAxis(s32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "RotateAxis", pValue);
    }

    bool getMapPartsArgRotateAccelType(s32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "RotateAccelType", pValue);
    }

    bool getMapPartsArgRotateStopTime(s32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "RotateStopTime", pValue);
    }

    bool getMapPartsArgRotateType(s32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "RotateType", pValue);
    }

    bool getMapPartsArgShadowType(s32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "ShadowType", pValue);
    }

    bool getMapPartsArgSignMotionType(s32* pValue, const JMapInfoIter& rIter) {
        return ::getJMapInfoArgNoInit(rIter, "SignMotionType", pValue);
    }

    bool getMapPartsArgMovePosture(s32* pValue, const LiveActor* pActor) {
        return getRailArg0NoInit(pActor, pValue);
    }

    bool getMapPartsArgMoveStopType(s32* pValue, const LiveActor* pActor) {
        return getRailArg1NoInit(pActor, pValue);
    }

    bool getMapPartsArgRailGuideType(s32* pValue, const LiveActor* pActor) {
        return getRailArg2NoInit(pActor, pValue);
    }

    bool getMapPartsArgRailInitPosType(s32* pValue, const LiveActor* pActor) {
        return getRailArg4NoInit(pActor, pValue);
    }

    bool getMapPartsArgMoveSpeed(f32* pValue, const LiveActor* pActor) {
        return getCurrentRailPointArg0NoInit(pActor, pValue);
    }

    bool getMapPartsArgStopTime(s32* pValue, const LiveActor* pActor) {
        return getCurrentRailPointArg5NoInit(pActor, pValue);
    }

    bool getMapPartsArgAccelTime(s32* pValue, const LiveActor* pActor) {
        return getCurrentRailPointArg1NoInit(pActor, pValue);
    }

    bool getMapPartsArgSpeedCalcType(s32* pValue, const LiveActor* pActor) {
        return getCurrentRailPointArg7NoInit(pActor, pValue);
    }

    bool getMapPartsArgMoveTimeToNextPoint(s32* pValue, const LiveActor* pActor) {
        return getCurrentRailPointArg0NoInit(pActor, pValue);
    }

    bool isMapPartsSignMotionTypeMoveStart(s32 signMotion) {
        if (signMotion == 1) {
            return true;
        }
        return signMotion == 4;
    }

    bool isMapPartsSignMotionTypeMoveWait(s32 signMotion) {
        if (signMotion == 2) {
            return true;
        }
        return signMotion == 5;
    }

    bool hasMapPartsMoveStartSignMotion(s32 signMotion) {
        if (isMapPartsSignMotionTypeMoveStart(signMotion)) {
            return true;
        }
        return isMapPartsSignMotionTypeMoveWait(signMotion);
    }

    bool hasMapPartsVanishSignMotion(s32 signMotion) {
        if (signMotion == 3) {
            return true;
        }

        if (signMotion == 4) {
            return true;
        }

        return signMotion == 5;
    }

    bool isMapPartsShadowTypeNone(s32 shadowType) {
        return shadowType == 0;
    }

    bool hasMapPartsShadow(s32 flag) {
        return flag != 0;
    }

    bool isMoveStartTypeUnconditional(s32 startType) {
        return startType == 0;
    }

    bool isMoveStartTypePlayerOnStopEnd(s32 startType) {
        return startType == 1;
    }

    bool isMapPartsRailInitPosTypeRailPos(s32 posType) {
        return posType == 0;
    }

    bool isMapPartsRailInitPosTypeRailPoint(s32 posType) {
        return posType == 1;
    }

    bool isMapPartsRailInitPosTypePoint0(s32 posType) {
        return posType == 2;
    }

    bool isMapPartsRailSpeedCalcTypeTime(s32 calcType) {
        return calcType == 1;
    }
    bool getMapPartsArgRailRotateSpeed(f32* pValue, const LiveActor* pActor, s32 point) {
        return getRailPointArg2WithInit(pActor, point, pValue);
    }

    bool getMapPartsArgRailRotateTime(s32* pValue, const LiveActor* pActor, s32 point) {
        return getRailPointArg2WithInit(pActor, point, pValue);
    }

    bool getMapPartsArgRailRotateAngle(f32* pValue, const LiveActor* pActor, s32 point) {
        return getRailPointArg3WithInit(pActor, point, pValue);
    }

    bool getMapPartsArgRailRotateAxis(s32* pValue, const LiveActor* pActor, s32 point) {
        return getRailPointArg4NoInit(pActor, point, pValue);
    }

    bool getMapPartsArgRailRotateType(s32* pValue, const LiveActor* pActor, s32 point) {
        return getRailPointArg6NoInit(pActor, point, pValue);
    }

    bool getMapPartsArgSpeedCalcType(s32* pValue, const LiveActor* pActor, s32 point) {
        return getRailPointArg7NoInit(pActor, point, pValue);
    }

};  // namespace MR
