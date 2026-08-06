#pragma once

#include "JSystem/JGeometry/TVec.hpp"

namespace MR {
    bool isInAreaObj(const char* pAreaName, const TVec3f& rPosition);
    bool isInWater(const TVec3f& rPosition);
}  // namespace MR
