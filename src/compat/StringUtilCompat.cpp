#include "Game/Util/StringUtil.hpp"

#include <cstring>

namespace MR {
    bool isEqualString(const char* pStr1, const char* pStr2) {
        return std::strcmp(pStr1, pStr2) == 0;
    }

    bool isNullOrEmptyString(const char* pStr) {
        return pStr == nullptr || MR::isEqualString(pStr, "");
    }
}  // namespace MR
