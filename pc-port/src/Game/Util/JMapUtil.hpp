#pragma once

#include "Game/Util/JMapInfo.hpp"

struct TVec3f;

namespace MR {
    bool isValidInfo(const JMapInfoIter& rIter);
    bool isObjectName(const JMapInfoIter& rIter, const char* pName);
    bool isEqualObjectName(const JMapInfoIter& rIter, const char* pName);
    bool getObjectName(const char** pDest, const JMapInfoIter& rIter);
    bool isExistJMapArg(const JMapInfoIter& rIter);

    bool getJMapInfoArgNoInit(const JMapInfoIter& rIter, const char* pFieldName, s32* pOut);
    bool getJMapInfoArgNoInit(const JMapInfoIter& rIter, const char* pFieldName, f32* pOut);
    bool getJMapInfoArgNoInit(const JMapInfoIter& rIter, const char* pFieldName, bool* pOut);

    bool getJMapInfoArg0WithInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg0WithInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg0WithInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg1WithInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg1WithInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg1WithInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg2WithInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg2WithInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg2WithInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg3WithInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg3WithInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg3WithInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg4WithInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg4WithInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg5WithInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg6WithInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg7WithInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg7WithInit(const JMapInfoIter& rIter, bool* pOut);

    bool getJMapInfoArg0NoInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg0NoInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg0NoInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg1NoInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg1NoInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg1NoInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg2NoInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg2NoInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg2NoInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg3NoInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg3NoInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg3NoInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg4NoInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg4NoInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg4NoInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg5NoInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg5NoInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg5NoInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg6NoInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg6NoInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg6NoInit(const JMapInfoIter& rIter, bool* pOut);
    bool getJMapInfoArg7NoInit(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoArg7NoInit(const JMapInfoIter& rIter, f32* pOut);
    bool getJMapInfoArg7NoInit(const JMapInfoIter& rIter, bool* pOut);

    bool getJMapInfoTrans(const JMapInfoIter& rIter, TVec3f* pOut);
    bool getJMapInfoRotate(const JMapInfoIter& rIter, TVec3f* pOut);
    bool getJMapInfoScale(const JMapInfoIter& rIter, TVec3f* pOut);
    bool getJMapInfoTransLocal(const JMapInfoIter& rIter, TVec3f* pOut);
    bool getJMapInfoRotateLocal(const JMapInfoIter& rIter, TVec3f* pOut);
    bool getJMapInfoV3f(const JMapInfoIter& rIter, const char* pName, TVec3f* pOut);

    bool getJMapInfoFollowID(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoGroupID(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoClippingGroupID(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoDemoGroupID(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoLinkID(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoCameraSetID(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoViewGroupID(const JMapInfoIter& rIter, s32* pOut);
    bool getJMapInfoMessageID(const JMapInfoIter& rIter, s32* pOut);
    bool isConnectedWithRail(const JMapInfoIter& rIter);
    bool isExistStageSwitchA(const JMapInfoIter& rIter);
    bool isExistStageSwitchB(const JMapInfoIter& rIter);
    bool isExistStageSwitchAppear(const JMapInfoIter& rIter);
    bool isExistStageSwitchDead(const JMapInfoIter& rIter);
    bool isExistStageSwitchSleep(const JMapInfoIter& rIter);

    s32 getDemoGroupID(const JMapInfoIter& rIter);
    s32 getDemoGroupLinkID(const JMapInfoIter& rIter);
    s32 getDemoCastID(const JMapInfoIter& rIter);
    const char* getDemoName(const JMapInfoIter& rIter);
    const char* getDemoSheetName(const JMapInfoIter& rIter);

    template <typename T>
    inline bool getValue(const JMapInfoIter& rIter, const char* pName, T* pOut) {
        return rIter.getValue<T>(pName, pOut);
    }
}
