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
