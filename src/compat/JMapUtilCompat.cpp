#include "Game/Util/JMapUtil.hpp"

#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/StringUtil.hpp"

namespace {
    bool getJMapInfoRailArg(const JMapInfoIter& rIter, const char* pName, s32* pOut) NO_INLINE {
        s32 arg;
        if (!rIter.getValue<s32>(pName, &arg)) return false;
        if (arg != -1) {
            *pOut = arg;
            return true;
        }
        return false;
    }
}

namespace MR {
    bool getJMapInfoRailArg0NoInit(const JMapInfoIter& rIter, s32* pOut) {
        return ::getJMapInfoRailArg(rIter, "path_arg0", pOut);
    }

    bool isEqualRailUsage(const JMapInfoIter& rIter, const char* pUsage) {
        const char* usage = nullptr;
        rIter.getValue<const char*>("usage", &usage);
        return isEqualStringCase(usage, pUsage);
    }

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
