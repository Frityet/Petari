#include "Game/Util/JMapUtil.hpp"

#include "Game/Util/MtxUtil.hpp"

namespace MR {
    bool getJMapInfoMatrixFromRT(const JMapInfoIter& rIter, TPos3f* pOut) {
        TVec3f translation;
        if (!getJMapInfoTrans(rIter, &translation)) {
            return false;
        }

        TVec3f rotation;
        if (!getJMapInfoRotate(rIter, &rotation)) {
            return false;
        }

        makeMtxTR(pOut->toMtxPtr(), translation, rotation);
        return true;
    }
}  // namespace MR
