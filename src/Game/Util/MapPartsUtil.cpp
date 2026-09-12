#include "Game/Util/MapPartsUtil.hpp"
#include <cstdio>

namespace MR {
    void getMapPartsObjectName(char* pName, u32 size, const char* pObjectName, s32 modelNo) {
        snprintf(pName, size, "%s%02d", pObjectName, modelNo);
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
};  // namespace MR
