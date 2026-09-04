#include "Game/Util/JMapUtil.hpp"

namespace MR {
    bool getJMapInfoShapeIdWithInit(const JMapInfoIter& rIter, s32* pShapeID) {
        return rIter.getValue< s32 >("ShapeModelNo", pShapeID);
    }
}
