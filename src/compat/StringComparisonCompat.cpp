#include <cctype>
#include "Game/Util/StringUtil.hpp"

#include <cstring>
#include <strings.h>

namespace MR {

    int strcasecmp(const char *lhs, const char *rhs) {
        return ::strcasecmp(lhs, rhs);
    }

    bool isEqualStringCase(const char *lhs, const char *rhs) {
        return ::strcasecmp(lhs, rhs) == 0;
    }

}  // namespace MR

// Original methods from Game/Util/StringUtil.cpp.
namespace MR {
    bool isEqualSubString(const char* pStr, const char* pSubStr) {
        return strstr(pStr, pSubStr) != nullptr;
    }

    bool hasStringSpace(const char* pStr) {
        return strchr(pStr, ' ') != nullptr;
    }

    bool isDigitStringTail(const char* pStr, int digitNum) {
        for (int i = 1; i <= digitNum; i++) {
            int ch = pStr[strlen(pStr) - i];

            if (isdigit(ch) == 0) {
                return false;
            }
        }

        return true;
    }
} // namespace MR
