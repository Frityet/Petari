#pragma once

#include "Game/Util/JMapInfo.hpp"
#include <revolution.h>

class JMapIdInfo {
public:
    inline JMapIdInfo() {}

    JMapIdInfo(s32, s32);
    JMapIdInfo(s32, const JMapInfoIter&);
    JMapIdInfo(const JMapIdInfo& rInf) {
        _0 = rInf._0;
        mZoneID = rInf.mZoneID;
    }

    void initialize(s32, const JMapInfoIter&);

    void operator=(const JMapIdInfo& rhs) NO_INLINE {
        _0 = rhs._0;
        mZoneID = rhs.mZoneID;
    }

    bool operator==(const JMapIdInfo&) const;

    s32 _0;
    s32 mZoneID;
};

namespace MR {
    JMapIdInfo& createJMapIdInfoFromClippingGroupId(const JMapInfoIter&);
};
